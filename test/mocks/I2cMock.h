#pragma once

#include <cstddef>
#include <cstdint>

namespace I2cMock {

void reset();
void setDevicePresent(uint8_t addr, bool present);
void setRegister(uint8_t addr, uint8_t reg, uint8_t value);
void setRegisters(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
uint8_t getRegister(uint8_t addr, uint8_t reg);

} // namespace I2cMock
