#pragma once

#include "esp_sntp.h"

namespace SntpMock {

void reset();
void setSyncStatus(sntp_sync_status_t status);
bool isEnabled();
bool wasConfigCalled();
const char *lastTimezone();
const char *lastServer1();
const char *lastServer2();
const char *lastServer3();

} // namespace SntpMock
