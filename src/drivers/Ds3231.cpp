// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "drivers/Ds3231.h"

#include <driver/i2c.h>
#include <string.h>

#include "config/AppConfig.h"

namespace {

uint8_t bcd2bin(uint8_t val) {
    return val - 6 * (val >> 4);
}

bool isBcdByte(uint8_t raw) {
    return ((raw >> 4) & 0x0F) <= 9 && (raw & 0x0F) <= 9;
}

bool isBcdWithin(uint8_t raw, uint8_t mask, uint8_t max_value, bool allow_zero) {
    raw &= mask;
    if (!isBcdByte(raw)) {
        return false;
    }
    const uint8_t value = bcd2bin(raw);
    if (!allow_zero && value == 0) {
        return false;
    }
    return value <= max_value;
}

bool hasValidHourLayout(uint8_t raw) {
    if ((raw & 0x80) != 0) {
        return false;
    }
    if ((raw & 0x40) != 0) {
        return isBcdWithin(raw, 0x1F, 12, false);
    }
    return isBcdWithin(raw, 0x3F, 23, true);
}

bool hasValidCalendarLayout(const uint8_t *regs) {
    return (regs[0] & 0x80) == 0 &&
           isBcdWithin(regs[0], 0x7F, 59, true) &&
           (regs[1] & 0x80) == 0 &&
           isBcdWithin(regs[1], 0x7F, 59, true) &&
           hasValidHourLayout(regs[2]) &&
           (regs[3] & 0xF8) == 0 &&
           (regs[3] & 0x07) <= 7 &&
           (regs[4] & 0xC0) == 0 &&
           isBcdWithin(regs[4], 0x3F, 31, false) &&
           (regs[5] & 0x60) == 0 &&
           isBcdWithin(regs[5], 0x1F, 12, false) &&
           isBcdByte(regs[6]);
}

} // namespace

bool Ds3231::probe() {
    uint8_t meta_regs[4] = { 0 };
    uint8_t time_regs[7] = { 0 };
    if (!read(Config::DS3231_REG_STATUS, meta_regs, sizeof(meta_regs)) ||
        !read(Config::DS3231_REG_SECONDS, time_regs, sizeof(time_regs))) {
        return false;
    }

    // Keep probe read-only and avoid depending on mutable CONTROL bits.
    return (meta_regs[0] & Config::DS3231_STATUS_RESERVED_MASK) == 0 &&
           (meta_regs[3] & Config::DS3231_TEMP_LSB_UNUSED_MASK) == 0 &&
           hasValidCalendarLayout(time_regs);
}

bool Ds3231::begin() {
    return true;
}

uint8_t Ds3231::bcd2bin(uint8_t val) {
    return val - 6 * (val >> 4);
}

uint8_t Ds3231::bin2bcd(uint8_t val) {
    return val + 6 * (val / 10);
}

bool Ds3231::read(uint8_t reg, uint8_t *buf, size_t len) {
    if (!buf || len == 0) {
        return false;
    }
    const esp_err_t err = i2c_master_write_read_device(
        Config::I2C_PORT,
        Config::DS3231_ADDR,
        &reg,
        1,
        buf,
        len,
        pdMS_TO_TICKS(Config::I2C_TIMEOUT_MS)
    );
    return err == ESP_OK;
}

bool Ds3231::write(uint8_t reg, const uint8_t *buf, size_t len) {
    if (!buf || len == 0 || len > 18) {
        return false;
    }
    uint8_t data[19] = { 0 };
    data[0] = reg;
    memcpy(&data[1], buf, len);
    const esp_err_t err = i2c_master_write_to_device(
        Config::I2C_PORT,
        Config::DS3231_ADDR,
        data,
        len + 1,
        pdMS_TO_TICKS(Config::I2C_TIMEOUT_MS)
    );
    return err == ESP_OK;
}

bool Ds3231::readTime(tm &out, bool &osc_stop, bool &valid) {
    uint8_t buf[7] = { 0 };
    uint8_t status = 0;
    if (!read(Config::DS3231_REG_SECONDS, buf, sizeof(buf)) ||
        !read(Config::DS3231_REG_STATUS, &status, 1)) {
        return false;
    }

    osc_stop = (status & Config::DS3231_STATUS_OSF) != 0;

    const int sec = bcd2bin(buf[0] & 0x7F);
    const int min = bcd2bin(buf[1] & 0x7F);
    int hour = 0;
    if ((buf[2] & 0x40) != 0) {
        hour = bcd2bin(buf[2] & 0x1F);
        const bool pm = (buf[2] & 0x20) != 0;
        if (hour == 12) {
            hour = pm ? 12 : 0;
        } else if (pm) {
            hour += 12;
        }
    } else {
        hour = bcd2bin(buf[2] & 0x3F);
    }

    const int day = bcd2bin(buf[4] & 0x3F);
    const int month = bcd2bin(buf[5] & 0x1F);
    const int year = bcd2bin(buf[6]) + 2000;
    const int weekday = buf[3] & 0x07;
    valid = !(sec > 59 || min > 59 || hour > 23 || day < 1 || day > 31 ||
              month < 1 || month > 12 || year < 2000 || year > 2099 ||
              weekday < 1 || weekday > 7);

    memset(&out, 0, sizeof(out));
    if (valid) {
        out.tm_sec = sec;
        out.tm_min = min;
        out.tm_hour = hour;
        out.tm_mday = day;
        out.tm_mon = month - 1;
        out.tm_year = year - 1900;
        out.tm_wday = weekday % 7;
    }
    out.tm_isdst = 0;
    return true;
}

bool Ds3231::writeTime(const tm &utc_tm) {
    const uint8_t weekday = (utc_tm.tm_wday == 0) ? 7 : utc_tm.tm_wday;
    uint8_t buf[7] = { 0 };
    buf[0] = bin2bcd(static_cast<uint8_t>(utc_tm.tm_sec)) & 0x7F;
    buf[1] = bin2bcd(static_cast<uint8_t>(utc_tm.tm_min)) & 0x7F;
    buf[2] = bin2bcd(static_cast<uint8_t>(utc_tm.tm_hour)) & 0x3F;
    buf[3] = bin2bcd(weekday) & 0x07;
    buf[4] = bin2bcd(static_cast<uint8_t>(utc_tm.tm_mday)) & 0x3F;
    buf[5] = bin2bcd(static_cast<uint8_t>(utc_tm.tm_mon + 1)) & 0x1F;
    buf[6] = bin2bcd(static_cast<uint8_t>(utc_tm.tm_year + 1900 - 2000));
    if (!write(Config::DS3231_REG_SECONDS, buf, sizeof(buf))) {
        return false;
    }
    return clearOscillatorStop();
}

bool Ds3231::clearOscillatorStop() {
    uint8_t status = 0;
    if (!read(Config::DS3231_REG_STATUS, &status, 1)) {
        return false;
    }
    status &= static_cast<uint8_t>(~Config::DS3231_STATUS_OSF);
    return write(Config::DS3231_REG_STATUS, &status, 1);
}

bool Ds3231::isBatteryLow(bool &low) {
    low = false;
    return false;
}
