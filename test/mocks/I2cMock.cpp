#include "I2cMock.h"

#include <array>

#include "driver/i2c.h"

namespace {

struct DeviceState {
    bool present = false;
    std::array<uint8_t, 256> regs{};
};

std::array<DeviceState, 256> g_devices{};

DeviceState &device(uint8_t addr) {
    return g_devices[addr];
}

} // namespace

namespace I2cMock {

void reset() {
    g_devices = {};
}

void setDevicePresent(uint8_t addr, bool present) {
    device(addr).present = present;
}

void setRegister(uint8_t addr, uint8_t reg, uint8_t value) {
    device(addr).regs[reg] = value;
}

void setRegisters(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
    if (!data) {
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        device(addr).regs[static_cast<uint8_t>(reg + i)] = data[i];
    }
}

uint8_t getRegister(uint8_t addr, uint8_t reg) {
    return device(addr).regs[reg];
}

} // namespace I2cMock

esp_err_t i2c_master_write_read_device(i2c_port_t,
                                       uint8_t addr,
                                       const uint8_t *write_buffer,
                                       size_t write_size,
                                       uint8_t *read_buffer,
                                       size_t read_size,
                                       TickType_t) {
    if (!device(addr).present || !write_buffer || write_size == 0 ||
        !read_buffer || read_size == 0) {
        return ESP_FAIL;
    }
    const uint8_t reg = write_buffer[0];
    for (size_t i = 0; i < read_size; ++i) {
        read_buffer[i] = device(addr).regs[static_cast<uint8_t>(reg + i)];
    }
    return ESP_OK;
}

esp_err_t i2c_master_write_to_device(i2c_port_t,
                                     uint8_t addr,
                                     const uint8_t *write_buffer,
                                     size_t write_size,
                                     TickType_t) {
    if (!device(addr).present || !write_buffer || write_size < 2) {
        return ESP_FAIL;
    }
    const uint8_t reg = write_buffer[0];
    for (size_t i = 1; i < write_size; ++i) {
        device(addr).regs[static_cast<uint8_t>(reg + i - 1)] = write_buffer[i];
    }
    return ESP_OK;
}
