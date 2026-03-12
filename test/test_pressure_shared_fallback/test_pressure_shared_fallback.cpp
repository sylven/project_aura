#include <unity.h>

#include "ArduinoMock.h"
#include "I2cMock.h"
#include "config/AppConfig.h"
#include "core/Logger.h"
#include "drivers/Bmp3xxProbe.h"

// Pull in the production DPS310 implementation under an alias so this suite can
// coexist with the mock-based SensorManager tests inside the normal native_test env.
#define Dps310 RealDps310
#include "../../src/drivers/Dps310.cpp"
#undef Dps310

namespace {

void seedSharedAddressDps310(uint8_t addr) {
    I2cMock::setDevicePresent(addr, true);

    // First byte collides with BMP390 chip id, but the rest of the shared-address
    // register map is intentionally invalid for BMP3xx reserved-bit checks.
    I2cMock::setRegister(addr, Config::BMP3XX_REG_CHIP_ID, Config::BMP3XX_CHIP_ID_BMP390);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_ERR, 0xA5);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_PWR_CTRL, 0xFF);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_OSR, 0xC1);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_ODR, 0xE0);

    I2cMock::setRegister(addr, Config::DPS310_PRODREVID, 0x10);
    I2cMock::setRegister(addr, Config::DPS310_MEASCFG, 0xC0);
    I2cMock::setRegister(addr, Config::DPS310_CFGREG, 0x00);
    I2cMock::setRegister(addr, Config::DPS310_TMPCOEFSRCE, 0x00);

    const uint8_t coeffs[18] = {
        0x64, 0x10, 0x22, 0x10, 0x20, 0x30, 0x40, 0x50, 0x06,
        0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15
    };
    I2cMock::setRegisters(addr, 0x10, coeffs, sizeof(coeffs));
}

} // namespace

void setUp() {
    setMillis(0);
    I2cMock::reset();
    Logger::begin(Serial, Logger::Debug);
    Logger::setSerialOutputEnabled(false);
    Logger::setSensorsSerialOutputEnabled(false);
}

void tearDown() {}

void test_real_dps310_start_succeeds_after_bmp3xx_probe_rejects_shared_address() {
    seedSharedAddressDps310(Config::DPS310_ADDR_PRIMARY);

    Bmp3xxProbe::Variant variant = Bmp3xxProbe::Variant::Unknown;
    TEST_ASSERT_FALSE(Bmp3xxProbe::detect(Config::DPS310_ADDR_PRIMARY, variant));

    RealDps310 dps310;
    TEST_ASSERT_TRUE(dps310.begin());
    TEST_ASSERT_TRUE(dps310.start());
    TEST_ASSERT_TRUE(dps310.isOk());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_real_dps310_start_succeeds_after_bmp3xx_probe_rejects_shared_address);
    return UNITY_END();
}
