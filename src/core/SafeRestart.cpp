// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "core/SafeRestart.h"

#include <Arduino.h>
#include <esp_cpu.h>
#include <esp_ipc.h>
#include <esp_system.h>

#include "core/Logger.h"

namespace {

void IRAM_ATTR restart_from_core0(void *) {
    esp_restart();
}

} // namespace

void safe_restart_via_core0() {
    if (esp_cpu_get_core_id() == 0) {
        esp_restart();
    }

    const esp_err_t err = esp_ipc_call_blocking(0, restart_from_core0, nullptr);
    if (err != ESP_OK) {
        LOGE("Restart", "esp_ipc_call_blocking failed: 0x%x", static_cast<unsigned>(err));
        esp_restart();
    }

    // Should never return. Keep system idle if restart is delayed.
    for (;;) {
        delay(100);
    }
}

