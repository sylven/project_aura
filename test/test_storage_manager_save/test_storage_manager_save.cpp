#include <unity.h>

#include "modules/StorageManager.h"

void setUp() {
    StorageManager::setTestForceSaveFailure(false);
}

void tearDown() {
    StorageManager::setTestForceSaveFailure(false);
}

void test_save_wifi_settings_preserves_spaces() {
    StorageManager storage;
    storage.begin();

    TEST_ASSERT_TRUE(storage.saveWiFiSettings("  My SSID  ", "  My Pass  ", true));
    TEST_ASSERT_EQUAL_STRING("  My SSID  ", storage.config().wifi_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("  My Pass  ", storage.config().wifi_pass.c_str());
    TEST_ASSERT_TRUE(storage.config().wifi_enabled);
}

void test_save_wifi_settings_rolls_back_on_failure() {
    StorageManager storage;
    storage.begin();
    TEST_ASSERT_TRUE(storage.saveWiFiSettings("old-ssid", "old-pass", true));

    StorageManager::setTestForceSaveFailure(true);
    TEST_ASSERT_FALSE(storage.saveWiFiSettings("new-ssid", "new-pass", true));

    TEST_ASSERT_EQUAL_STRING("old-ssid", storage.config().wifi_ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("old-pass", storage.config().wifi_pass.c_str());
    TEST_ASSERT_TRUE(storage.config().wifi_enabled);
}

void test_save_mqtt_settings_preserves_spaces() {
    StorageManager storage;
    storage.begin();

    TEST_ASSERT_TRUE(storage.saveMqttSettings("broker.local", 1883, "  user  ", "  pass  ",
                                              "base/topic", "Device", true, false));
    TEST_ASSERT_EQUAL_STRING("broker.local", storage.config().mqtt_host.c_str());
    TEST_ASSERT_EQUAL_UINT16(1883, storage.config().mqtt_port);
    TEST_ASSERT_EQUAL_STRING("  user  ", storage.config().mqtt_user.c_str());
    TEST_ASSERT_EQUAL_STRING("  pass  ", storage.config().mqtt_pass.c_str());
    TEST_ASSERT_EQUAL_STRING("base/topic", storage.config().mqtt_base_topic.c_str());
}

void test_save_mqtt_settings_rolls_back_on_failure() {
    StorageManager storage;
    storage.begin();
    TEST_ASSERT_TRUE(storage.saveMqttSettings("old-host", 1883, "old-user", "old-pass",
                                              "old/topic", "Old Device", false, true));

    StorageManager::setTestForceSaveFailure(true);
    TEST_ASSERT_FALSE(storage.saveMqttSettings("new-host", 1884, "new-user", "new-pass",
                                               "new/topic", "New Device", true, false));

    TEST_ASSERT_EQUAL_STRING("old-host", storage.config().mqtt_host.c_str());
    TEST_ASSERT_EQUAL_UINT16(1883, storage.config().mqtt_port);
    TEST_ASSERT_EQUAL_STRING("old-user", storage.config().mqtt_user.c_str());
    TEST_ASSERT_EQUAL_STRING("old-pass", storage.config().mqtt_pass.c_str());
    TEST_ASSERT_EQUAL_STRING("old/topic", storage.config().mqtt_base_topic.c_str());
    TEST_ASSERT_EQUAL_STRING("Old Device", storage.config().mqtt_device_name.c_str());
    TEST_ASSERT_FALSE(storage.config().mqtt_discovery);
    TEST_ASSERT_TRUE(storage.config().mqtt_anonymous);
}

void test_save_dac_auto_state_persists_mode_and_armed() {
    StorageManager storage;
    storage.begin();

    TEST_ASSERT_TRUE(storage.saveDacAutoState(true, true));
    TEST_ASSERT_TRUE(storage.config().dac_auto_mode);
    TEST_ASSERT_TRUE(storage.config().dac_auto_armed);

    TEST_ASSERT_TRUE(storage.saveDacAutoState(true, false));
    TEST_ASSERT_TRUE(storage.config().dac_auto_mode);
    TEST_ASSERT_FALSE(storage.config().dac_auto_armed);
}

void test_save_dac_auto_state_rolls_back_on_failure() {
    StorageManager storage;
    storage.begin();
    TEST_ASSERT_TRUE(storage.saveDacAutoState(true, true));

    StorageManager::setTestForceSaveFailure(true);
    TEST_ASSERT_FALSE(storage.saveDacAutoState(false, false));

    TEST_ASSERT_TRUE(storage.config().dac_auto_mode);
    TEST_ASSERT_TRUE(storage.config().dac_auto_armed);
}

void test_save_rtc_mode_persists_selection() {
    StorageManager storage;
    storage.begin();

    TEST_ASSERT_TRUE(storage.saveRtcMode(Config::RtcMode::Ds3231));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Config::RtcMode::Ds3231),
                          static_cast<int>(storage.config().rtc_mode));

    TEST_ASSERT_TRUE(storage.saveRtcMode(Config::RtcMode::Pcf8523));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Config::RtcMode::Pcf8523),
                          static_cast<int>(storage.config().rtc_mode));
}

void test_save_rtc_mode_rolls_back_on_failure() {
    StorageManager storage;
    storage.begin();
    TEST_ASSERT_TRUE(storage.saveRtcMode(Config::RtcMode::Pcf8523));

    StorageManager::setTestForceSaveFailure(true);
    TEST_ASSERT_FALSE(storage.saveRtcMode(Config::RtcMode::Ds3231));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Config::RtcMode::Pcf8523),
                          static_cast<int>(storage.config().rtc_mode));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_save_wifi_settings_preserves_spaces);
    RUN_TEST(test_save_wifi_settings_rolls_back_on_failure);
    RUN_TEST(test_save_mqtt_settings_preserves_spaces);
    RUN_TEST(test_save_mqtt_settings_rolls_back_on_failure);
    RUN_TEST(test_save_dac_auto_state_persists_mode_and_armed);
    RUN_TEST(test_save_dac_auto_state_rolls_back_on_failure);
    RUN_TEST(test_save_rtc_mode_persists_selection);
    RUN_TEST(test_save_rtc_mode_rolls_back_on_failure);
    return UNITY_END();
}
