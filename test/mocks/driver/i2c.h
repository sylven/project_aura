#pragma once

#include <cstddef>
#include <cstdint>

typedef int i2c_port_t;
typedef int esp_err_t;
typedef uint32_t TickType_t;

#ifndef I2C_NUM_0
#define I2C_NUM_0 0
#endif

#ifndef ESP_OK
#define ESP_OK 0
#endif

#ifndef ESP_FAIL
#define ESP_FAIL -1
#endif

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (ms)
#endif

esp_err_t i2c_master_write_read_device(i2c_port_t port,
                                       uint8_t addr,
                                       const uint8_t *write_buffer,
                                       size_t write_size,
                                       uint8_t *read_buffer,
                                       size_t read_size,
                                       TickType_t ticks_to_wait);

esp_err_t i2c_master_write_to_device(i2c_port_t port,
                                     uint8_t addr,
                                     const uint8_t *write_buffer,
                                     size_t write_size,
                                     TickType_t ticks_to_wait);
