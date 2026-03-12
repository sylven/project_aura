#include <unity.h>

#include "I2cMock.h"
#include "config/AppConfig.h"
#include "drivers/Bmp3xxProbe.h"

namespace {

void seedBmp3xxRegisters(uint8_t addr, uint8_t chip_id) {
    I2cMock::setDevicePresent(addr, true);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_CHIP_ID, chip_id);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_ERR, 0x00);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_PWR_CTRL, 0x33);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_OSR, 0x3F);
    I2cMock::setRegister(addr, Config::BMP3XX_REG_ODR, 0x1F);
}

} // namespace

void setUp() {
    I2cMock::reset();
}

void tearDown() {}

void test_bmp3xx_probe_accepts_bmp388_with_non_default_valid_config_bits() {
    seedBmp3xxRegisters(Config::BMP3XX_ADDR_PRIMARY, Config::BMP3XX_CHIP_ID_BMP388);

    Bmp3xxProbe::Variant variant = Bmp3xxProbe::Variant::Unknown;
    TEST_ASSERT_TRUE(Bmp3xxProbe::detect(Config::BMP3XX_ADDR_PRIMARY, variant));
    TEST_ASSERT_EQUAL(static_cast<int>(Bmp3xxProbe::Variant::BMP388),
                      static_cast<int>(variant));
}

void test_bmp3xx_probe_accepts_bmp390_with_non_default_valid_config_bits() {
    seedBmp3xxRegisters(Config::BMP3XX_ADDR_ALT, Config::BMP3XX_CHIP_ID_BMP390);

    Bmp3xxProbe::Variant variant = Bmp3xxProbe::Variant::Unknown;
    TEST_ASSERT_TRUE(Bmp3xxProbe::detect(Config::BMP3XX_ADDR_ALT, variant));
    TEST_ASSERT_EQUAL(static_cast<int>(Bmp3xxProbe::Variant::BMP390),
                      static_cast<int>(variant));
}

void test_bmp3xx_probe_rejects_shared_address_device_with_matching_first_byte_only() {
    I2cMock::setDevicePresent(Config::BMP3XX_ADDR_PRIMARY, true);
    I2cMock::setRegister(Config::BMP3XX_ADDR_PRIMARY, Config::BMP3XX_REG_CHIP_ID,
                         Config::BMP3XX_CHIP_ID_BMP390);
    I2cMock::setRegister(Config::BMP3XX_ADDR_PRIMARY, Config::BMP3XX_REG_ERR, 0xA5);
    I2cMock::setRegister(Config::BMP3XX_ADDR_PRIMARY, Config::BMP3XX_REG_PWR_CTRL, 0xFF);
    I2cMock::setRegister(Config::BMP3XX_ADDR_PRIMARY, Config::BMP3XX_REG_OSR, 0xC1);
    I2cMock::setRegister(Config::BMP3XX_ADDR_PRIMARY, Config::BMP3XX_REG_ODR, 0xE0);

    Bmp3xxProbe::Variant variant = Bmp3xxProbe::Variant::BMP388;
    TEST_ASSERT_FALSE(Bmp3xxProbe::detect(Config::BMP3XX_ADDR_PRIMARY, variant));
    TEST_ASSERT_EQUAL(static_cast<int>(Bmp3xxProbe::Variant::Unknown),
                      static_cast<int>(variant));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_bmp3xx_probe_accepts_bmp388_with_non_default_valid_config_bits);
    RUN_TEST(test_bmp3xx_probe_accepts_bmp390_with_non_default_valid_config_bits);
    RUN_TEST(test_bmp3xx_probe_rejects_shared_address_device_with_matching_first_byte_only);
    return UNITY_END();
}
