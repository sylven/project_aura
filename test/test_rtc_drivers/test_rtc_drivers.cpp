#include <unity.h>

#include "ArduinoMock.h"
#include "I2cMock.h"
#include "config/AppConfig.h"
#include "drivers/Ds3231.h"
#include "drivers/Pcf8523.h"

namespace {

uint8_t toBcd(uint8_t value) {
    return static_cast<uint8_t>(value + 6 * (value / 10));
}

void seedPcf8523Signature() {
    I2cMock::setDevicePresent(Config::PCF8523_ADDR, true);
    const uint8_t signature[] = {0x00, 0x00, 0x07, 0x00, 0x07};
    I2cMock::setRegisters(Config::PCF8523_ADDR, Config::PCF8523_REG_OFFSET,
                          signature, sizeof(signature));
}

void seedDs3231Signature(uint8_t control = 0x1C) {
    I2cMock::setDevicePresent(Config::DS3231_ADDR, true);
    const uint8_t signature[] = {control, 0x88, 0x00, 0x19, 0x40};
    I2cMock::setRegisters(Config::DS3231_ADDR, Config::DS3231_REG_CONTROL,
                          signature, sizeof(signature));
    const uint8_t time_regs[] = {
        toBcd(42), toBcd(35), toBcd(14), 4, toBcd(12), toBcd(3), toBcd(26)
    };
    I2cMock::setRegisters(Config::DS3231_ADDR, Config::DS3231_REG_SECONDS,
                          time_regs, sizeof(time_regs));
}

void seedDs3231ThatMatchesPcf8523Fallback() {
    I2cMock::setDevicePresent(Config::DS3231_ADDR, true);
    const uint8_t meta_regs[] = {0x00, 0x00, 0x19, 0x40};
    I2cMock::setRegisters(Config::DS3231_ADDR, Config::DS3231_REG_STATUS,
                          meta_regs, sizeof(meta_regs));
    const uint8_t time_regs[] = {
        toBcd(8), toBcd(34), toBcd(9), 4, toBcd(12), toBcd(3), toBcd(19)
    };
    I2cMock::setRegisters(Config::DS3231_ADDR, Config::DS3231_REG_SECONDS,
                          time_regs, sizeof(time_regs));
    const uint8_t alarm1_regs[] = {3, 4, 5};
    I2cMock::setRegisters(Config::DS3231_ADDR, 0x07, alarm1_regs, sizeof(alarm1_regs));
}

} // namespace

void setUp() {
    setMillis(0);
    I2cMock::reset();
}

void tearDown() {}

void test_pcf8523_probe_matches_project_signature() {
    seedPcf8523Signature();

    Pcf8523 pcf8523;
    Ds3231 ds3231;

    TEST_ASSERT_TRUE(pcf8523.probe());
    TEST_ASSERT_FALSE(ds3231.probe());
}

void test_pcf8523_probe_falls_back_to_calendar_layout() {
    I2cMock::setDevicePresent(Config::PCF8523_ADDR, true);
    const uint8_t control[] = {0x00, 0x00, 0x00};
    const uint8_t time_regs[] = {
        toBcd(12), toBcd(34), toBcd(9), toBcd(15), toBcd(3), toBcd(4), toBcd(26)
    };
    I2cMock::setRegisters(Config::PCF8523_ADDR, Config::PCF8523_REG_CONTROL_1,
                          control, sizeof(control));
    I2cMock::setRegisters(Config::PCF8523_ADDR, Config::PCF8523_REG_SECONDS,
                          time_regs, sizeof(time_regs));
    I2cMock::setRegister(Config::PCF8523_ADDR, Config::PCF8523_REG_OFFSET, 0x01);
    I2cMock::setRegister(Config::PCF8523_ADDR, Config::PCF8523_REG_TMR_A_FREQ_CTRL, 0x05);
    I2cMock::setRegister(Config::PCF8523_ADDR, Config::PCF8523_REG_TMR_B_FREQ_CTRL, 0x05);

    Pcf8523 pcf8523;
    TEST_ASSERT_TRUE(pcf8523.probeFallback());
}

void test_pcf8523_probe_fallback_accepts_dirty_pcf8523_layout_with_invalid_calendar() {
    I2cMock::setDevicePresent(Config::PCF8523_ADDR, true);
    const uint8_t control[] = {0x00, 0x00, 0x00};
    const uint8_t time_regs[] = {
        toBcd(12), toBcd(34), toBcd(9), 0x1A, toBcd(3), toBcd(4), toBcd(26)
    };
    const uint8_t timer_regs[] = {0x35, 0x80, 0x02, 0x10, 0x07};
    I2cMock::setRegisters(Config::PCF8523_ADDR, Config::PCF8523_REG_CONTROL_1,
                          control, sizeof(control));
    I2cMock::setRegisters(Config::PCF8523_ADDR, Config::PCF8523_REG_SECONDS,
                          time_regs, sizeof(time_regs));
    I2cMock::setRegisters(Config::PCF8523_ADDR, Config::PCF8523_REG_OFFSET,
                          timer_regs, sizeof(timer_regs));

    Pcf8523 pcf8523;
    Ds3231 ds3231;
    TEST_ASSERT_FALSE(pcf8523.probe());
    TEST_ASSERT_FALSE(ds3231.probe());
    TEST_ASSERT_TRUE(pcf8523.probeFallback());
}

void test_ds3231_probe_matches_signature() {
    seedDs3231Signature();

    Ds3231 ds3231;
    Pcf8523 pcf8523;

    TEST_ASSERT_TRUE(ds3231.probe());
    TEST_ASSERT_FALSE(pcf8523.probe());
}

void test_ds3231_probe_accepts_nondefault_control_bits() {
    seedDs3231Signature(0x03);

    Ds3231 ds3231;
    Pcf8523 pcf8523;

    TEST_ASSERT_TRUE(ds3231.probe());
    TEST_ASSERT_FALSE(pcf8523.probe());
}

void test_ds3231_probe_rejects_zero_day_or_month() {
    seedDs3231Signature();

    Ds3231 ds3231;

    I2cMock::setRegister(Config::DS3231_ADDR, 0x04, 0x00);
    TEST_ASSERT_FALSE(ds3231.probe());

    seedDs3231Signature();
    I2cMock::setRegister(Config::DS3231_ADDR, 0x05, 0x00);
    TEST_ASSERT_FALSE(ds3231.probe());
}

void test_ds3231_no_longer_matches_pcf8523_fallback_shape() {
    seedDs3231ThatMatchesPcf8523Fallback();

    Ds3231 ds3231;
    Pcf8523 pcf8523;

    TEST_ASSERT_FALSE(pcf8523.probe());
    TEST_ASSERT_FALSE(pcf8523.probeFallback());
    TEST_ASSERT_TRUE(ds3231.probe());
}

void test_ds3231_read_time_reports_osf_and_valid_time() {
    seedDs3231Signature();
    const uint8_t time_regs[] = {
        toBcd(42), toBcd(35), toBcd(14), toBcd(4), toBcd(12), toBcd(3), toBcd(26)
    };
    I2cMock::setRegisters(Config::DS3231_ADDR, Config::DS3231_REG_SECONDS,
                          time_regs, sizeof(time_regs));
    I2cMock::setRegister(Config::DS3231_ADDR, Config::DS3231_REG_STATUS, 0x80);

    Ds3231 ds3231;
    tm out = {};
    bool osc_stop = false;
    bool valid = false;

    TEST_ASSERT_TRUE(ds3231.readTime(out, osc_stop, valid));
    TEST_ASSERT_TRUE(osc_stop);
    TEST_ASSERT_TRUE(valid);
    TEST_ASSERT_EQUAL_INT(42, out.tm_sec);
    TEST_ASSERT_EQUAL_INT(35, out.tm_min);
    TEST_ASSERT_EQUAL_INT(14, out.tm_hour);
    TEST_ASSERT_EQUAL_INT(12, out.tm_mday);
    TEST_ASSERT_EQUAL_INT(2, out.tm_mon);
    TEST_ASSERT_EQUAL_INT(126, out.tm_year);
}

void test_pcf8523_begin_enables_battery_switching() {
    seedPcf8523Signature();
    I2cMock::setRegister(Config::PCF8523_ADDR, Config::PCF8523_REG_CONTROL_3, 0xE0);

    Pcf8523 pcf8523;
    TEST_ASSERT_TRUE(pcf8523.begin());
    TEST_ASSERT_EQUAL_HEX8(0x00,
                           I2cMock::getRegister(Config::PCF8523_ADDR,
                                                Config::PCF8523_REG_CONTROL_3));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_pcf8523_probe_matches_project_signature);
    RUN_TEST(test_pcf8523_probe_falls_back_to_calendar_layout);
    RUN_TEST(test_pcf8523_probe_fallback_accepts_dirty_pcf8523_layout_with_invalid_calendar);
    RUN_TEST(test_ds3231_probe_matches_signature);
    RUN_TEST(test_ds3231_probe_accepts_nondefault_control_bits);
    RUN_TEST(test_ds3231_probe_rejects_zero_day_or_month);
    RUN_TEST(test_ds3231_no_longer_matches_pcf8523_fallback_shape);
    RUN_TEST(test_ds3231_read_time_reports_osf_and_valid_time);
    RUN_TEST(test_pcf8523_begin_enables_battery_switching);
    return UNITY_END();
}
