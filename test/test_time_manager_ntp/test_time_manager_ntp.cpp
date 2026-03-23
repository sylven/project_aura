#include <unity.h>

#include "ArduinoMock.h"
#include "SntpMock.h"
#include "modules/StorageManager.h"
#include "modules/TimeManager.h"

void setUp() {
    setMillis(0);
    SntpMock::reset();
}

void tearDown() {}

void test_time_manager_ntp_uses_default_public_servers_when_custom_server_is_empty() {
    StorageManager storage;
    storage.begin();

    TimeManager manager;
    manager.begin(storage);

    TEST_ASSERT_TRUE(manager.updateWifiState(true, true));
    TEST_ASSERT_TRUE(SntpMock::wasConfigCalled());
    TEST_ASSERT_TRUE(SntpMock::isEnabled());
    TEST_ASSERT_EQUAL_STRING("pool.ntp.org", SntpMock::lastServer1());
    TEST_ASSERT_EQUAL_STRING("time.nist.gov", SntpMock::lastServer2());
    TEST_ASSERT_EQUAL_STRING("time.google.com", SntpMock::lastServer3());
}

void test_time_manager_ntp_uses_custom_server_when_configured() {
    StorageManager storage;
    storage.begin();
    storage.config().ntp_server = "192.168.1.1";

    TimeManager manager;
    manager.begin(storage);

    TEST_ASSERT_TRUE(manager.updateWifiState(true, true));
    TEST_ASSERT_TRUE(SntpMock::wasConfigCalled());
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", SntpMock::lastServer1());
    TEST_ASSERT_EQUAL_STRING("", SntpMock::lastServer2());
    TEST_ASSERT_EQUAL_STRING("", SntpMock::lastServer3());
}

void test_time_manager_set_ntp_server_pref_restarts_sync_with_new_server() {
    StorageManager storage;
    storage.begin();

    TimeManager manager;
    manager.begin(storage);
    TEST_ASSERT_TRUE(manager.updateWifiState(true, true));
    TEST_ASSERT_EQUAL_STRING("pool.ntp.org", SntpMock::lastServer1());

    advanceMillis(100);
    TEST_ASSERT_TRUE(manager.setNtpServerPref(" router.local "));
    TEST_ASSERT_EQUAL_STRING("router.local", manager.ntpServerPref().c_str());
    TEST_ASSERT_EQUAL_STRING("router.local", SntpMock::lastServer1());
    TEST_ASSERT_EQUAL_STRING("", SntpMock::lastServer2());
    TEST_ASSERT_EQUAL_STRING("", SntpMock::lastServer3());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_time_manager_ntp_uses_default_public_servers_when_custom_server_is_empty);
    RUN_TEST(test_time_manager_ntp_uses_custom_server_when_configured);
    RUN_TEST(test_time_manager_set_ntp_server_pref_restarts_sync_with_new_server);
    return UNITY_END();
}
