// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "ui/UiController.h"

#include <float.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include "config/AppConfig.h"
#include "modules/ChartsHistory.h"
#include "ui/UiText.h"
#include "ui/ui.h"

namespace {

float temperature_to_display(float celsius, bool units_c) {
    if (units_c) {
        return celsius;
    }
    return (celsius * 9.0f / 5.0f) + 32.0f;
}

float graph_nice_step(float value) {
    if (!isfinite(value) || value <= 0.0f) {
        return 1.0f;
    }
    const float exponent = floorf(log10f(value));
    const float base = powf(10.0f, exponent);
    const float normalized = value / base;
    float nice = 1.0f;
    if (normalized <= 1.0f) {
        nice = 1.0f;
    } else if (normalized <= 2.0f) {
        nice = 2.0f;
    } else if (normalized <= 5.0f) {
        nice = 5.0f;
    } else {
        nice = 10.0f;
    }
    return nice * base;
}

bool format_epoch_hhmm(time_t epoch, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0 || epoch <= Config::TIME_VALID_EPOCH) {
        return false;
    }
    tm local_tm = {};
    if (!localtime_r(&epoch, &local_tm)) {
        return false;
    }
    snprintf(buf, buf_size, "%02d:%02d", local_tm.tm_hour, local_tm.tm_min);
    return true;
}

void format_relative_time_label(uint32_t offset_s, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) {
        return;
    }
    if (offset_s == 0) {
        snprintf(buf, buf_size, "now");
        return;
    }
    const uint32_t hours = offset_s / 3600U;
    const uint32_t minutes = (offset_s % 3600U) / 60U;
    if (hours > 0 && minutes == 0) {
        snprintf(buf, buf_size, "-%luh", static_cast<unsigned long>(hours));
        return;
    }
    if (hours > 0) {
        snprintf(buf, buf_size, "-%luh%02lum", static_cast<unsigned long>(hours), static_cast<unsigned long>(minutes));
        return;
    }
    snprintf(buf, buf_size, "-%lum", static_cast<unsigned long>(minutes > 0 ? minutes : 1U));
}

constexpr uint8_t kTempGraphTimeTickCount = 7;

} // namespace

uint16_t UiController::temperature_graph_points() const {
    switch (temp_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

uint16_t UiController::humidity_graph_points() const {
    switch (rh_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

uint16_t UiController::voc_graph_points() const {
    switch (voc_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

uint16_t UiController::nox_graph_points() const {
    switch (nox_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

uint16_t UiController::hcho_graph_points() const {
    switch (hcho_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

uint16_t UiController::co2_graph_points() const {
    switch (co2_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

uint16_t UiController::co_graph_points() const {
    switch (co_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

uint16_t UiController::pressure_graph_points() const {
    switch (pressure_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            return Config::CHART_HISTORY_1H_STEPS;
        case TEMP_GRAPH_RANGE_24H:
            return Config::CHART_HISTORY_24H_SAMPLES;
        case TEMP_GRAPH_RANGE_3H:
        default:
            return Config::CHART_HISTORY_3H_STEPS;
    }
}

UiController::SensorGraphProfile UiController::build_temperature_graph_profile() const {
    SensorGraphProfile profile{};
    profile.min_span = temp_units_c ? 2.0f : 3.5f;
    profile.fallback_value = temp_units_c ? 22.0f : 71.6f;
    switch (temp_graph_range_) {
        case TEMP_GRAPH_RANGE_1H:
            // 0..60 min with 5 min step => 13 vertical marks
            profile.vertical_divisions = 13;
            break;
        case TEMP_GRAPH_RANGE_24H:
            // 0..24 h with 1 h step => 25 vertical marks
            profile.vertical_divisions = 25;
            break;
        case TEMP_GRAPH_RANGE_3H:
        default:
            // 0..180 min with 15 min step => 13 vertical marks
            profile.vertical_divisions = 13;
            break;
    }
    profile.horizontal_divisions_min = 3;
    profile.horizontal_divisions_max = 12;
    profile.unit = temp_units_c ? UiText::UnitC() : UiText::UnitF();
    profile.label_min = "MIN";
    profile.label_now = "NOW";
    profile.label_max = "MAX";
    profile.zone_count = 7;

    const float bounds_c[kMaxGraphZoneBounds] = {-1000.0f, 16.0f, 18.0f, 20.0f, 25.0f, 26.0f, 28.0f, 1000.0f};
    for (uint8_t i = 0; i < kMaxGraphZoneBounds; ++i) {
        const bool edge = (i == 0) || (i == (kMaxGraphZoneBounds - 1));
        profile.zone_bounds[i] = edge ? bounds_c[i] : temperature_to_display(bounds_c[i], temp_units_c);
    }

    profile.zone_tones[0] = GRAPH_ZONE_RED;
    profile.zone_tones[1] = GRAPH_ZONE_ORANGE;
    profile.zone_tones[2] = GRAPH_ZONE_YELLOW;
    profile.zone_tones[3] = GRAPH_ZONE_GREEN;
    profile.zone_tones[4] = GRAPH_ZONE_YELLOW;
    profile.zone_tones[5] = GRAPH_ZONE_ORANGE;
    profile.zone_tones[6] = GRAPH_ZONE_RED;

    return profile;
}

lv_color_t UiController::resolve_graph_zone_color(GraphZoneTone tone, lv_color_t chart_bg) {
    switch (tone) {
        case GRAPH_ZONE_RED:
            return lv_color_mix(color_red(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_ORANGE:
            return lv_color_mix(color_orange(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_YELLOW:
            return lv_color_mix(color_yellow(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_GREEN:
            return lv_color_mix(color_green(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_BLUE:
            return lv_color_mix(color_blue(), chart_bg, LV_OPA_40);
        case GRAPH_ZONE_NONE:
        default:
            return lv_color_mix(color_card_border(), chart_bg, LV_OPA_40);
    }
}

void UiController::sync_info_graph_button_state() {
    if (!objects.btn_info_graph) {
        return;
    }

    const bool voc_selected = (info_sensor == INFO_VOC);
    const bool nox_selected = (info_sensor == INFO_NOX);
    const bool hcho_selected = (info_sensor == INFO_HCHO);
    const bool co2_selected = (info_sensor == INFO_CO2);
    const bool co_selected = (info_sensor == INFO_CO);
    const bool pressure_selected = (info_sensor == INFO_PRESSURE_3H) || (info_sensor == INFO_PRESSURE_24H);
    const bool graph_supported = (info_sensor == INFO_TEMP) || (info_sensor == INFO_RH) ||
                                 voc_selected || nox_selected || hcho_selected || co2_selected ||
                                 co_selected || pressure_selected;
    const bool graph_checked =
        ((info_sensor == INFO_TEMP) && temp_graph_mode_) ||
        ((info_sensor == INFO_RH) && rh_graph_mode_) ||
        (voc_selected && voc_graph_mode_) ||
        (nox_selected && nox_graph_mode_) ||
        (hcho_selected && hcho_graph_mode_) ||
        (co2_selected && co2_graph_mode_) ||
        (co_selected && co_graph_mode_) ||
        (pressure_selected && pressure_graph_mode_);

    if (graph_checked) {
        lv_obj_add_state(objects.btn_info_graph, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(objects.btn_info_graph, LV_STATE_CHECKED);
    }

    if (graph_supported) {
        set_visible(objects.btn_info_graph, true);
        lv_obj_clear_state(objects.btn_info_graph, LV_STATE_DISABLED);
        lv_obj_add_flag(objects.btn_info_graph, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_move_foreground(objects.btn_info_graph);
    } else {
        lv_obj_clear_state(objects.btn_info_graph, LV_STATE_CHECKED);
        lv_obj_add_state(objects.btn_info_graph, LV_STATE_DISABLED);
        set_visible(objects.btn_info_graph, false);
    }

    lv_obj_set_ext_click_area(objects.btn_info_graph, 18);
}

bool UiController::should_show_threshold_dots() const {
    if (info_sensor == INFO_NONE) {
        return false;
    }
    if (info_sensor == INFO_TEMP) {
        return !temp_graph_mode_;
    }
    if (info_sensor == INFO_RH) {
        return !rh_graph_mode_;
    }
    if (info_sensor == INFO_VOC) {
        return !voc_graph_mode_;
    }
    if (info_sensor == INFO_NOX) {
        return !nox_graph_mode_;
    }
    if (info_sensor == INFO_HCHO) {
        return !hcho_graph_mode_;
    }
    if (info_sensor == INFO_CO2) {
        return !co2_graph_mode_;
    }
    if (info_sensor == INFO_CO) {
        return !co_graph_mode_;
    }
    if (info_sensor == INFO_PRESSURE_3H || info_sensor == INFO_PRESSURE_24H) {
        return !pressure_graph_mode_;
    }
    return true;
}

void UiController::sync_threshold_dots_visibility() {
    set_visible(objects.container_thresholds_dots, should_show_threshold_dots());
}

void UiController::set_temperature_info_mode(bool graph_mode) {
    temp_graph_mode_ = graph_mode;
    if (objects.btn_info_graph) {
        lv_obj_add_flag(objects.btn_info_graph, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_info_graph, 18);
    }
    if (objects.btn_temp_range_1h) {
        lv_obj_add_flag(objects.btn_temp_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_temp_range_1h, 12);
    }
    if (objects.btn_temp_range_3h) {
        lv_obj_add_flag(objects.btn_temp_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_temp_range_3h, 12);
    }
    if (objects.btn_temp_range_24h) {
        lv_obj_add_flag(objects.btn_temp_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_temp_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.temperature_info_thresholds, !graph_mode);
    set_visible(objects.temperature_info_graph, graph_mode);
    sync_threshold_dots_visibility();

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };

    sync_info_graph_button_state();
    set_checked(objects.btn_temp_range_1h, temp_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_temp_range_3h, temp_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_temp_range_24h, temp_graph_range_ == TEMP_GRAPH_RANGE_24H);
}

void UiController::set_rh_info_mode(bool graph_mode) {
    rh_graph_mode_ = graph_mode;
    if (objects.btn_rh_range_1h) {
        lv_obj_add_flag(objects.btn_rh_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_rh_range_1h, 12);
    }
    if (objects.btn_rh_range_3h) {
        lv_obj_add_flag(objects.btn_rh_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_rh_range_3h, 12);
    }
    if (objects.btn_rh_range_24h) {
        lv_obj_add_flag(objects.btn_rh_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_rh_range_24h, 12);
    }
    set_visible(objects.rh_info_thresholds, !graph_mode);
    set_visible(objects.rh_info_graph, graph_mode);
    sync_threshold_dots_visibility();

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };
    set_checked(objects.btn_rh_range_1h, rh_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_rh_range_3h, rh_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_rh_range_24h, rh_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_voc_info_mode(bool graph_mode) {
    voc_graph_mode_ = graph_mode;
    if (objects.btn_voc_range_1h) {
        lv_obj_add_flag(objects.btn_voc_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_voc_range_1h, 12);
    }
    if (objects.btn_voc_range_3h) {
        lv_obj_add_flag(objects.btn_voc_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_voc_range_3h, 12);
    }
    if (objects.btn_voc_range_24h) {
        lv_obj_add_flag(objects.btn_voc_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_voc_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.voc_info_thresholds, !graph_mode);
    set_visible(objects.voc_info_graph, graph_mode);
    sync_threshold_dots_visibility();

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };
    set_checked(objects.btn_voc_range_1h, voc_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_voc_range_3h, voc_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_voc_range_24h, voc_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_nox_info_mode(bool graph_mode) {
    nox_graph_mode_ = graph_mode;
    if (objects.btn_nox_range_1h) {
        lv_obj_add_flag(objects.btn_nox_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_nox_range_1h, 12);
    }
    if (objects.btn_nox_range_3h) {
        lv_obj_add_flag(objects.btn_nox_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_nox_range_3h, 12);
    }
    if (objects.btn_nox_range_24h) {
        lv_obj_add_flag(objects.btn_nox_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_nox_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.nox_info_thresholds, !graph_mode);
    set_visible(objects.nox_info_graph, graph_mode);
    sync_threshold_dots_visibility();

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };
    set_checked(objects.btn_nox_range_1h, nox_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_nox_range_3h, nox_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_nox_range_24h, nox_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_hcho_info_mode(bool graph_mode) {
    hcho_graph_mode_ = graph_mode;
    if (objects.btn_hcho_range_1h) {
        lv_obj_add_flag(objects.btn_hcho_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_hcho_range_1h, 12);
    }
    if (objects.btn_hcho_range_3h) {
        lv_obj_add_flag(objects.btn_hcho_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_hcho_range_3h, 12);
    }
    if (objects.btn_hcho_range_24h) {
        lv_obj_add_flag(objects.btn_hcho_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_hcho_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.hcho_info_thresholds, !graph_mode);
    set_visible(objects.hcho_info_graph, graph_mode);
    sync_threshold_dots_visibility();

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };
    set_checked(objects.btn_hcho_range_1h, hcho_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_hcho_range_3h, hcho_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_hcho_range_24h, hcho_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_co2_info_mode(bool graph_mode) {
    co2_graph_mode_ = graph_mode;
    if (objects.btn_co2_range_1h) {
        lv_obj_add_flag(objects.btn_co2_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co2_range_1h, 12);
    }
    if (objects.btn_co2_range_3h) {
        lv_obj_add_flag(objects.btn_co2_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co2_range_3h, 12);
    }
    if (objects.btn_co2_range_24h) {
        lv_obj_add_flag(objects.btn_co2_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co2_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.co2_info_thresholds, !graph_mode);
    set_visible(objects.co2_info_graph, graph_mode);
    sync_threshold_dots_visibility();

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };
    set_checked(objects.btn_co2_range_1h, co2_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_co2_range_3h, co2_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_co2_range_24h, co2_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_co_info_mode(bool graph_mode) {
    co_graph_mode_ = graph_mode;
    if (objects.btn_co_range_1h) {
        lv_obj_add_flag(objects.btn_co_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co_range_1h, 12);
    }
    if (objects.btn_co_range_3h) {
        lv_obj_add_flag(objects.btn_co_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co_range_3h, 12);
    }
    if (objects.btn_co_range_24h) {
        lv_obj_add_flag(objects.btn_co_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_co_range_24h, 12);
    }
    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }
    set_visible(objects.co_info_thresholds, !graph_mode);
    set_visible(objects.co_info_graph, graph_mode);
    sync_threshold_dots_visibility();

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };
    set_checked(objects.btn_co_range_1h, co_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_co_range_3h, co_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_co_range_24h, co_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::set_pressure_info_mode(bool graph_mode) {
    pressure_graph_mode_ = graph_mode;

    if (objects.btn_pressure_range_1h) {
        lv_obj_add_flag(objects.btn_pressure_range_1h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pressure_range_1h, 12);
    }
    if (objects.btn_pressure_range_3h) {
        lv_obj_add_flag(objects.btn_pressure_range_3h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pressure_range_3h, 12);
    }
    if (objects.btn_pressure_range_24h) {
        lv_obj_add_flag(objects.btn_pressure_range_24h, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_ext_click_area(objects.btn_pressure_range_24h, 12);
    }

    if (objects.btn_info_graph) {
        lv_obj_move_foreground(objects.btn_info_graph);
    }
    if (objects.btn_back_1) {
        lv_obj_move_foreground(objects.btn_back_1);
    }

    const bool show_3h = info_sensor == INFO_PRESSURE_3H;
    const bool show_24h = info_sensor == INFO_PRESSURE_24H;
    set_visible(objects.pressure_3h_pressure_thresholds, !graph_mode && show_3h);
    set_visible(objects.pressure_24h_pressure_thresholds, !graph_mode && show_24h);
    set_visible(objects.pressure_info_graph, graph_mode && (show_3h || show_24h));
    set_visible(objects.btn_3h_pressure_info, !graph_mode);
    set_visible(objects.btn_24h_pressure_info, !graph_mode);
    sync_threshold_dots_visibility();

    auto set_checked = [](lv_obj_t *btn, bool checked) {
        if (!btn) {
            return;
        }
        if (checked) {
            lv_obj_add_state(btn, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(btn, LV_STATE_CHECKED);
        }
    };
    set_checked(objects.btn_pressure_range_1h, pressure_graph_range_ == TEMP_GRAPH_RANGE_1H);
    set_checked(objects.btn_pressure_range_3h, pressure_graph_range_ == TEMP_GRAPH_RANGE_3H);
    set_checked(objects.btn_pressure_range_24h, pressure_graph_range_ == TEMP_GRAPH_RANGE_24H);

    sync_info_graph_button_state();
}

void UiController::apply_temperature_graph_theme(const SensorGraphProfile &profile) {
    if (!objects.chart_temp_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_temp_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_temp_info, LV_CHART_UPDATE_MODE_SHIFT);
    const uint8_t initial_horizontal = (profile.horizontal_divisions_min > 0) ? profile.horizontal_divisions_min : 3;
    const uint8_t initial_vertical = (profile.vertical_divisions > 0) ? profile.vertical_divisions : 15;
    lv_chart_set_div_line_count(objects.chart_temp_info, initial_horizontal, initial_vertical);

    lv_obj_set_style_bg_color(objects.chart_temp_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_temp_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_temp_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_temp_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_temp_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_temp_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_temp_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_temp_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_temp_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_temp_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_temp_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_temp_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

void UiController::ensure_temperature_graph_overlays() {
    if (!objects.chart_temp_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_temp_info) {
            label = lv_label_create(objects.chart_temp_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(temp_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(temp_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(temp_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_temperature_graph_overlays(const SensorGraphProfile &profile,
                                                     bool has_values,
                                                     float min_temp,
                                                     float max_temp,
                                                     float latest_temp) {
    if (!objects.chart_temp_info) {
        return;
    }

    ensure_temperature_graph_overlays();
    if (!temp_graph_label_min_ || !temp_graph_label_now_ || !temp_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_temp_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_temp_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {temp_graph_label_min_, temp_graph_label_now_, temp_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        char min_empty[24];
        char now_empty[24];
        char max_empty[24];
        snprintf(min_empty, sizeof(min_empty), "%s --", profile.label_min ? profile.label_min : "MIN");
        snprintf(now_empty, sizeof(now_empty), "%s --", profile.label_now ? profile.label_now : "NOW");
        snprintf(max_empty, sizeof(max_empty), "%s --", profile.label_max ? profile.label_max : "MAX");
        safe_label_set_text(temp_graph_label_min_, min_empty);
        safe_label_set_text(temp_graph_label_now_, now_empty);
        safe_label_set_text(temp_graph_label_max_, max_empty);
        return;
    }

    const char *unit = profile.unit;
    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "%s %.1f%s", profile.label_min ? profile.label_min : "MIN", min_temp, unit ? unit : "");
    snprintf(now_buf, sizeof(now_buf), "%s %.1f%s", profile.label_now ? profile.label_now : "NOW", latest_temp, unit ? unit : "");
    snprintf(max_buf, sizeof(max_buf), "%s %.1f%s", profile.label_max ? profile.label_max : "MAX", max_temp, unit ? unit : "");
    safe_label_set_text(temp_graph_label_min_, min_buf);
    safe_label_set_text(temp_graph_label_now_, now_buf);
    safe_label_set_text(temp_graph_label_max_, max_buf);
}

void UiController::ensure_temperature_zone_overlay() {
    if (!objects.temperature_info_graph || !objects.chart_temp_info) {
        return;
    }

    if (!temp_graph_zone_overlay_ || !lv_obj_is_valid(temp_graph_zone_overlay_) ||
        lv_obj_get_parent(temp_graph_zone_overlay_) != objects.temperature_info_graph) {
        temp_graph_zone_overlay_ = lv_obj_create(objects.temperature_info_graph);
        lv_obj_clear_flag(temp_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(temp_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(temp_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_temp_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_temp_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_temp_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_temp_info);

    lv_obj_set_pos(temp_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(temp_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(temp_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_temp_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = temp_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != temp_graph_zone_overlay_) {
            band = lv_obj_create(temp_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(temp_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_temp_info);
}

void UiController::update_temperature_zone_overlay(const SensorGraphProfile &profile,
                                                   float y_min_display,
                                                   float y_max_display) {
    ensure_temperature_zone_overlay();
    if (!temp_graph_zone_overlay_ || !lv_obj_is_valid(temp_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(temp_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(temp_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (temp_graph_zone_bands_[i]) {
                lv_obj_add_flag(temp_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_temp_info, LV_PART_MAIN);
    uint8_t zone_count = profile.zone_count;
    if (zone_count > kMaxGraphZoneBands) {
        zone_count = kMaxGraphZoneBands;
    }
    if (zone_count == 0) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (temp_graph_zone_bands_[i]) {
                lv_obj_add_flag(temp_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    const float denom = y_max_display - y_min_display;
    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = temp_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= zone_count) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = profile.zone_bounds[i];
        const float zone_high = profile.zone_bounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        lv_color_t zone_color = resolve_graph_zone_color(profile.zone_tones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_temperature_time_labels() {
    if (!objects.temperature_info_graph || !objects.chart_temp_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.temperature_info_graph) {
            label = lv_label_create(objects.temperature_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(temp_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_temp_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = temp_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_temperature_time_labels() {
    if (!objects.chart_temp_info || !objects.temperature_info_graph) {
        return;
    }

    ensure_temperature_time_labels();
    if (!temp_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = temperature_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = temp_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_temp_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_temp_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_temp_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_temp_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = temp_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::ensure_humidity_graph_overlays() {
    if (!objects.chart_rh_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_rh_info) {
            label = lv_label_create(objects.chart_rh_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(rh_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(rh_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(rh_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_humidity_graph_overlays(bool has_values,
                                                  float min_humidity,
                                                  float max_humidity,
                                                  float latest_humidity) {
    if (!objects.chart_rh_info) {
        return;
    }

    ensure_humidity_graph_overlays();
    if (!rh_graph_label_min_ || !rh_graph_label_now_ || !rh_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_rh_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_rh_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {rh_graph_label_min_, rh_graph_label_now_, rh_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        safe_label_set_text(rh_graph_label_min_, "MIN --");
        safe_label_set_text(rh_graph_label_now_, "NOW --");
        safe_label_set_text(rh_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f%%", min_humidity);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f%%", latest_humidity);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f%%", max_humidity);
    safe_label_set_text(rh_graph_label_min_, min_buf);
    safe_label_set_text(rh_graph_label_now_, now_buf);
    safe_label_set_text(rh_graph_label_max_, max_buf);
}

void UiController::ensure_humidity_zone_overlay() {
    if (!objects.rh_info_graph || !objects.chart_rh_info) {
        return;
    }

    if (!rh_graph_zone_overlay_ || !lv_obj_is_valid(rh_graph_zone_overlay_) ||
        lv_obj_get_parent(rh_graph_zone_overlay_) != objects.rh_info_graph) {
        rh_graph_zone_overlay_ = lv_obj_create(objects.rh_info_graph);
        lv_obj_clear_flag(rh_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(rh_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(rh_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_rh_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_rh_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_rh_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_rh_info);

    lv_obj_set_pos(rh_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(rh_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(rh_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_rh_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = rh_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != rh_graph_zone_overlay_) {
            band = lv_obj_create(rh_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(rh_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_rh_info);
}

void UiController::update_humidity_zone_overlay(float y_min_display, float y_max_display) {
    ensure_humidity_zone_overlay();
    if (!rh_graph_zone_overlay_ || !lv_obj_is_valid(rh_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(rh_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(rh_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (rh_graph_zone_bands_[i]) {
                lv_obj_add_flag(rh_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kRhZoneBounds[kMaxGraphZoneBounds] = {-1000.0f, 20.0f, 30.0f, 40.0f, 60.0f, 65.0f, 70.0f, 1000.0f};
    static const GraphZoneTone kRhZoneTones[kMaxGraphZoneBands] = {
        GRAPH_ZONE_RED,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };

    const float denom = y_max_display - y_min_display;
    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_rh_info, LV_PART_MAIN);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = rh_graph_zone_bands_[i];
        if (!band) {
            continue;
        }

        const float zone_low = kRhZoneBounds[i];
        const float zone_high = kRhZoneBounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        lv_color_t zone_color = resolve_graph_zone_color(kRhZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_humidity_time_labels() {
    if (!objects.rh_info_graph || !objects.chart_rh_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.rh_info_graph) {
            label = lv_label_create(objects.rh_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(rh_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_rh_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = rh_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_humidity_time_labels() {
    if (!objects.chart_rh_info || !objects.rh_info_graph) {
        return;
    }

    ensure_humidity_time_labels();
    if (!rh_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = humidity_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = rh_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_rh_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_rh_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_rh_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_rh_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = rh_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_temperature_info_graph() {
    if (!objects.chart_temp_info) {
        return;
    }

    const SensorGraphProfile profile = build_temperature_graph_profile();
    apply_temperature_graph_theme(profile);

    const uint16_t points = temperature_graph_points();
    lv_chart_set_point_count(objects.chart_temp_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_temp_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_temp_info,
                                     lv_obj_get_style_line_color(objects.chart_temp_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_temp_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_temp_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_temp = FLT_MAX;
    float max_temp = -FLT_MAX;
    float latest_temp = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value_c = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_TEMPERATURE, value_c, valid) &&
                valid && isfinite(value_c)) {
                const float display_value = temperature_to_display(value_c, temp_units_c);
                if (!has_values) {
                    min_temp = display_value;
                    max_temp = display_value;
                    has_values = true;
                } else {
                    if (display_value < min_temp) {
                        min_temp = display_value;
                    }
                    if (display_value > max_temp) {
                        max_temp = display_value;
                    }
                }
                latest_temp = display_value;
                point_value = static_cast<lv_coord_t>(lroundf(display_value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_temp_info, series, i, point_value);
    }

    const float fallback = profile.fallback_value;

    float scale_min = min_temp;
    float scale_max = max_temp;
    if (has_values) {
        scale_min = min_temp;
        scale_max = max_temp;
    } else {
        scale_min = fallback;
        scale_max = fallback;
        latest_temp = fallback;
    }

    const float min_span = profile.min_span;
    const float span = scale_max - scale_min;
    float scale_span = span;
    if (!isfinite(scale_span) || scale_span < min_span) {
        scale_span = min_span;
    }
    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = temp_units_c ? 0.5f : 1.0f;
    }
    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_temp) ? latest_temp : fallback;
        y_min_f = center - min_span;
        y_max_f = center + min_span;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f * 10.0f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f * 10.0f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }
    if ((y_max - y_min) < 10) {
        const lv_coord_t center = static_cast<lv_coord_t>((y_min + y_max) / 2);
        y_min = static_cast<lv_coord_t>(center - 5);
        y_max = static_cast<lv_coord_t>(center + 5);
    }
    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    const int32_t horizontal_min = (profile.horizontal_divisions_min > 0)
        ? static_cast<int32_t>(profile.horizontal_divisions_min)
        : 3;
    const int32_t horizontal_max = (profile.horizontal_divisions_max >= profile.horizontal_divisions_min &&
                                    profile.horizontal_divisions_max > 0)
        ? static_cast<int32_t>(profile.horizontal_divisions_max)
        : 12;
    if (horizontal_divisions < horizontal_min) {
        horizontal_divisions = horizontal_min;
    }
    if (horizontal_divisions > horizontal_max) {
        horizontal_divisions = horizontal_max;
    }
    const uint8_t vertical_divisions = (profile.vertical_divisions > 0) ? profile.vertical_divisions : 15;
    lv_chart_set_div_line_count(objects.chart_temp_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_temp_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_temperature_zone_overlay(profile, y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_temp)) {
            latest_temp = max_temp;
        }
        update_temperature_graph_overlays(profile, true, min_temp, max_temp, latest_temp);
    } else {
        update_temperature_graph_overlays(profile, false, fallback, fallback, fallback);
    }
    update_temperature_time_labels();

    lv_chart_refresh(objects.chart_temp_info);
}

void UiController::update_humidity_info_graph() {
    if (!objects.chart_rh_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_rh_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_rh_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (rh_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }
    lv_chart_set_div_line_count(objects.chart_rh_info, 5, vertical_divisions);

    lv_obj_set_style_bg_color(objects.chart_rh_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_rh_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_rh_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_rh_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_rh_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_rh_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_rh_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_rh_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_rh_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_rh_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_rh_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_rh_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = humidity_graph_points();
    lv_chart_set_point_count(objects.chart_rh_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_rh_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_rh_info,
                                     lv_obj_get_style_line_color(objects.chart_rh_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_rh_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_rh_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_h = FLT_MAX;
    float max_h = -FLT_MAX;
    float latest_h = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_HUMIDITY, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_h = value;
                    max_h = value;
                    has_values = true;
                } else {
                    if (value < min_h) {
                        min_h = value;
                    }
                    if (value > max_h) {
                        max_h = value;
                    }
                }
                latest_h = value;
                point_value = static_cast<lv_coord_t>(lroundf(value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_rh_info, series, i, point_value);
    }

    float scale_min = has_values ? min_h : 50.0f;
    float scale_max = has_values ? max_h : 50.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 10.0f) {
        scale_span = 10.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 5.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        y_min_f = 0.0f;
        y_max_f = 100.0f;
    }

    if (y_min_f < 0.0f) {
        y_min_f = 0.0f;
    }
    if (y_max_f > 100.0f) {
        y_max_f = 100.0f;
    }
    if (y_max_f <= y_min_f) {
        y_min_f = 0.0f;
        y_max_f = 100.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f * 10.0f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f * 10.0f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_rh_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_rh_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_humidity_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_h)) {
            latest_h = max_h;
        }
        update_humidity_graph_overlays(true, min_h, max_h, latest_h);
    } else {
        update_humidity_graph_overlays(false, 50.0f, 50.0f, 50.0f);
    }
    update_humidity_time_labels();

    lv_chart_refresh(objects.chart_rh_info);
}

void UiController::ensure_voc_graph_overlays() {
    if (!objects.chart_voc_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_voc_info) {
            label = lv_label_create(objects.chart_voc_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(voc_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(voc_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(voc_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_voc_graph_overlays(bool has_values,
                                             float min_voc,
                                             float max_voc,
                                             float latest_voc) {
    if (!objects.chart_voc_info) {
        return;
    }

    ensure_voc_graph_overlays();
    if (!voc_graph_label_min_ || !voc_graph_label_now_ || !voc_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_voc_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_voc_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {voc_graph_label_min_, voc_graph_label_now_, voc_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        safe_label_set_text(voc_graph_label_min_, "MIN --");
        safe_label_set_text(voc_graph_label_now_, "NOW --");
        safe_label_set_text(voc_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f idx", min_voc);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f idx", latest_voc);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f idx", max_voc);
    safe_label_set_text(voc_graph_label_min_, min_buf);
    safe_label_set_text(voc_graph_label_now_, now_buf);
    safe_label_set_text(voc_graph_label_max_, max_buf);
}

void UiController::ensure_voc_zone_overlay() {
    if (!objects.voc_info_graph || !objects.chart_voc_info) {
        return;
    }

    if (!voc_graph_zone_overlay_ || !lv_obj_is_valid(voc_graph_zone_overlay_) ||
        lv_obj_get_parent(voc_graph_zone_overlay_) != objects.voc_info_graph) {
        voc_graph_zone_overlay_ = lv_obj_create(objects.voc_info_graph);
        lv_obj_clear_flag(voc_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(voc_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(voc_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_voc_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_voc_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_voc_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_voc_info);

    lv_obj_set_pos(voc_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(voc_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(voc_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_voc_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = voc_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != voc_graph_zone_overlay_) {
            band = lv_obj_create(voc_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(voc_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_voc_info);
}

void UiController::update_voc_zone_overlay(float y_min_display, float y_max_display) {
    ensure_voc_zone_overlay();
    if (!voc_graph_zone_overlay_ || !lv_obj_is_valid(voc_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(voc_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(voc_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (voc_graph_zone_bands_[i]) {
                lv_obj_add_flag(voc_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kVocZoneBounds[] = {
        -1000.0f,
        static_cast<float>(Config::AQ_VOC_GREEN_MAX_INDEX),
        static_cast<float>(Config::AQ_VOC_YELLOW_MAX_INDEX),
        static_cast<float>(Config::AQ_VOC_ORANGE_MAX_INDEX),
        100000.0f};
    static const GraphZoneTone kVocZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kVocZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_voc_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = voc_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kVocZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kVocZoneBounds[i];
        const float zone_high = kVocZoneBounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        const lv_color_t zone_color = resolve_graph_zone_color(kVocZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_voc_time_labels() {
    if (!objects.voc_info_graph || !objects.chart_voc_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.voc_info_graph) {
            label = lv_label_create(objects.voc_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(voc_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_voc_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = voc_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_voc_time_labels() {
    if (!objects.chart_voc_info || !objects.voc_info_graph) {
        return;
    }

    ensure_voc_time_labels();
    if (!voc_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = voc_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = voc_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_voc_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_voc_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_voc_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_voc_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = voc_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_voc_info_graph() {
    if (!objects.chart_voc_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_voc_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_voc_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (voc_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_voc_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_voc_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_voc_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_voc_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_voc_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_voc_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_voc_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_voc_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_voc_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_voc_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_voc_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_voc_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = voc_graph_points();
    lv_chart_set_point_count(objects.chart_voc_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_voc_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_voc_info,
                                     lv_obj_get_style_line_color(objects.chart_voc_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_voc_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_voc_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_voc = FLT_MAX;
    float max_voc = -FLT_MAX;
    float latest_voc = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_VOC, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_voc = value;
                    max_voc = value;
                    has_values = true;
                } else {
                    if (value < min_voc) {
                        min_voc = value;
                    }
                    if (value > max_voc) {
                        max_voc = value;
                    }
                }
                latest_voc = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_voc_info, series, i, point_value);
    }

    float scale_min = has_values ? min_voc : 100.0f;
    float scale_max = has_values ? max_voc : 100.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 80.0f) {
        scale_span = 80.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 25.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_voc) ? latest_voc : 100.0f;
        y_min_f = center - 80.0f;
        y_max_f = center + 80.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_voc_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_voc_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_voc_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_voc)) {
            latest_voc = max_voc;
        }
        update_voc_graph_overlays(true, min_voc, max_voc, latest_voc);
    } else {
        update_voc_graph_overlays(false, 100.0f, 100.0f, 100.0f);
    }
    update_voc_time_labels();

    lv_chart_refresh(objects.chart_voc_info);
}

void UiController::ensure_nox_graph_overlays() {
    if (!objects.chart_nox_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_nox_info) {
            label = lv_label_create(objects.chart_nox_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(nox_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(nox_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(nox_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_nox_graph_overlays(bool has_values,
                                             float min_nox,
                                             float max_nox,
                                             float latest_nox) {
    if (!objects.chart_nox_info) {
        return;
    }

    ensure_nox_graph_overlays();
    if (!nox_graph_label_min_ || !nox_graph_label_now_ || !nox_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_nox_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_nox_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {nox_graph_label_min_, nox_graph_label_now_, nox_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        safe_label_set_text(nox_graph_label_min_, "MIN --");
        safe_label_set_text(nox_graph_label_now_, "NOW --");
        safe_label_set_text(nox_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f idx", min_nox);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f idx", latest_nox);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f idx", max_nox);
    safe_label_set_text(nox_graph_label_min_, min_buf);
    safe_label_set_text(nox_graph_label_now_, now_buf);
    safe_label_set_text(nox_graph_label_max_, max_buf);
}

void UiController::ensure_nox_zone_overlay() {
    if (!objects.nox_info_graph || !objects.chart_nox_info) {
        return;
    }

    if (!nox_graph_zone_overlay_ || !lv_obj_is_valid(nox_graph_zone_overlay_) ||
        lv_obj_get_parent(nox_graph_zone_overlay_) != objects.nox_info_graph) {
        nox_graph_zone_overlay_ = lv_obj_create(objects.nox_info_graph);
        lv_obj_clear_flag(nox_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(nox_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(nox_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_nox_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_nox_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_nox_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_nox_info);

    lv_obj_set_pos(nox_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(nox_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(nox_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_nox_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = nox_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != nox_graph_zone_overlay_) {
            band = lv_obj_create(nox_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(nox_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_nox_info);
}

void UiController::update_nox_zone_overlay(float y_min_display, float y_max_display) {
    ensure_nox_zone_overlay();
    if (!nox_graph_zone_overlay_ || !lv_obj_is_valid(nox_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(nox_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(nox_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (nox_graph_zone_bands_[i]) {
                lv_obj_add_flag(nox_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kNoxZoneBounds[] = {
        -1000.0f,
        static_cast<float>(Config::AQ_NOX_GREEN_MAX_INDEX),
        static_cast<float>(Config::AQ_NOX_YELLOW_MAX_INDEX),
        static_cast<float>(Config::AQ_NOX_ORANGE_MAX_INDEX),
        100000.0f};
    static const GraphZoneTone kNoxZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kNoxZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_nox_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = nox_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kNoxZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kNoxZoneBounds[i];
        const float zone_high = kNoxZoneBounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        const lv_color_t zone_color = resolve_graph_zone_color(kNoxZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_nox_time_labels() {
    if (!objects.nox_info_graph || !objects.chart_nox_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.nox_info_graph) {
            label = lv_label_create(objects.nox_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(nox_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_nox_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = nox_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_nox_time_labels() {
    if (!objects.chart_nox_info || !objects.nox_info_graph) {
        return;
    }

    ensure_nox_time_labels();
    if (!nox_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = nox_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = nox_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_nox_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_nox_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_nox_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_nox_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = nox_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_nox_info_graph() {
    if (!objects.chart_nox_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_nox_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_nox_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (nox_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_nox_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_nox_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_nox_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_nox_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_nox_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_nox_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_nox_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_nox_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_nox_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_nox_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_nox_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_nox_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = nox_graph_points();
    lv_chart_set_point_count(objects.chart_nox_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_nox_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_nox_info,
                                     lv_obj_get_style_line_color(objects.chart_nox_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_nox_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_nox_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_nox = FLT_MAX;
    float max_nox = -FLT_MAX;
    float latest_nox = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_NOX, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_nox = value;
                    max_nox = value;
                    has_values = true;
                } else {
                    if (value < min_nox) {
                        min_nox = value;
                    }
                    if (value > max_nox) {
                        max_nox = value;
                    }
                }
                latest_nox = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_nox_info, series, i, point_value);
    }

    float scale_min = has_values ? min_nox : 50.0f;
    float scale_max = has_values ? max_nox : 50.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 60.0f) {
        scale_span = 60.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 20.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_nox) ? latest_nox : 50.0f;
        y_min_f = center - 60.0f;
        y_max_f = center + 60.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_nox_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_nox_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_nox_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_nox)) {
            latest_nox = max_nox;
        }
        update_nox_graph_overlays(true, min_nox, max_nox, latest_nox);
    } else {
        update_nox_graph_overlays(false, 50.0f, 50.0f, 50.0f);
    }
    update_nox_time_labels();

    lv_chart_refresh(objects.chart_nox_info);
}

void UiController::ensure_hcho_graph_overlays() {
    if (!objects.chart_hcho_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_hcho_info) {
            label = lv_label_create(objects.chart_hcho_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(hcho_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(hcho_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(hcho_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_hcho_graph_overlays(bool has_values,
                                              float min_hcho,
                                              float max_hcho,
                                              float latest_hcho) {
    if (!objects.chart_hcho_info) {
        return;
    }

    ensure_hcho_graph_overlays();
    if (!hcho_graph_label_min_ || !hcho_graph_label_now_ || !hcho_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_hcho_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_hcho_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {hcho_graph_label_min_, hcho_graph_label_now_, hcho_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        safe_label_set_text(hcho_graph_label_min_, "MIN --");
        safe_label_set_text(hcho_graph_label_now_, "NOW --");
        safe_label_set_text(hcho_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f ppb", min_hcho);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f ppb", latest_hcho);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f ppb", max_hcho);
    safe_label_set_text(hcho_graph_label_min_, min_buf);
    safe_label_set_text(hcho_graph_label_now_, now_buf);
    safe_label_set_text(hcho_graph_label_max_, max_buf);
}

void UiController::ensure_hcho_zone_overlay() {
    if (!objects.hcho_info_graph || !objects.chart_hcho_info) {
        return;
    }

    if (!hcho_graph_zone_overlay_ || !lv_obj_is_valid(hcho_graph_zone_overlay_) ||
        lv_obj_get_parent(hcho_graph_zone_overlay_) != objects.hcho_info_graph) {
        hcho_graph_zone_overlay_ = lv_obj_create(objects.hcho_info_graph);
        lv_obj_clear_flag(hcho_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(hcho_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(hcho_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_hcho_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_hcho_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_hcho_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_hcho_info);

    lv_obj_set_pos(hcho_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(hcho_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(hcho_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_hcho_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = hcho_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != hcho_graph_zone_overlay_) {
            band = lv_obj_create(hcho_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(hcho_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_hcho_info);
}

void UiController::update_hcho_zone_overlay(float y_min_display, float y_max_display) {
    ensure_hcho_zone_overlay();
    if (!hcho_graph_zone_overlay_ || !lv_obj_is_valid(hcho_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(hcho_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(hcho_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (hcho_graph_zone_bands_[i]) {
                lv_obj_add_flag(hcho_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kHchoZoneBounds[] = {
        -1000.0f,
        30.0f,
        60.0f,
        100.0f,
        100000.0f};
    static const GraphZoneTone kHchoZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kHchoZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_hcho_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = hcho_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kHchoZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kHchoZoneBounds[i];
        const float zone_high = kHchoZoneBounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        const lv_color_t zone_color = resolve_graph_zone_color(kHchoZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_hcho_time_labels() {
    if (!objects.hcho_info_graph || !objects.chart_hcho_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.hcho_info_graph) {
            label = lv_label_create(objects.hcho_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(hcho_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_hcho_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = hcho_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_hcho_time_labels() {
    if (!objects.chart_hcho_info || !objects.hcho_info_graph) {
        return;
    }

    ensure_hcho_time_labels();
    if (!hcho_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = hcho_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = hcho_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_hcho_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_hcho_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_hcho_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_hcho_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = hcho_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_hcho_info_graph() {
    if (!objects.chart_hcho_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_hcho_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_hcho_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (hcho_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_hcho_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_hcho_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_hcho_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_hcho_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_hcho_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_hcho_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_hcho_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_hcho_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_hcho_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_hcho_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_hcho_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_hcho_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = hcho_graph_points();
    lv_chart_set_point_count(objects.chart_hcho_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_hcho_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_hcho_info,
                                     lv_obj_get_style_line_color(objects.chart_hcho_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_hcho_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_hcho_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_hcho = FLT_MAX;
    float max_hcho = -FLT_MAX;
    float latest_hcho = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_HCHO, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_hcho = value;
                    max_hcho = value;
                    has_values = true;
                } else {
                    if (value < min_hcho) {
                        min_hcho = value;
                    }
                    if (value > max_hcho) {
                        max_hcho = value;
                    }
                }
                latest_hcho = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_hcho_info, series, i, point_value);
    }

    float scale_min = has_values ? min_hcho : 20.0f;
    float scale_max = has_values ? max_hcho : 20.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 40.0f) {
        scale_span = 40.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 10.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_hcho) ? latest_hcho : 20.0f;
        y_min_f = center - 40.0f;
        y_max_f = center + 40.0f;
    }
    if (y_min_f < 0.0f) {
        y_min_f = 0.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_hcho_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_hcho_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_hcho_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_hcho)) {
            latest_hcho = max_hcho;
        }
        update_hcho_graph_overlays(true, min_hcho, max_hcho, latest_hcho);
    } else {
        update_hcho_graph_overlays(false, 20.0f, 20.0f, 20.0f);
    }
    update_hcho_time_labels();

    lv_chart_refresh(objects.chart_hcho_info);
}

void UiController::ensure_co2_graph_overlays() {
    if (!objects.chart_co2_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_co2_info) {
            label = lv_label_create(objects.chart_co2_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(co2_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(co2_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(co2_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_co2_graph_overlays(bool has_values,
                                             float min_co2,
                                             float max_co2,
                                             float latest_co2) {
    if (!objects.chart_co2_info) {
        return;
    }

    ensure_co2_graph_overlays();
    if (!co2_graph_label_min_ || !co2_graph_label_now_ || !co2_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_co2_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_co2_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {co2_graph_label_min_, co2_graph_label_now_, co2_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        safe_label_set_text(co2_graph_label_min_, "MIN --");
        safe_label_set_text(co2_graph_label_now_, "NOW --");
        safe_label_set_text(co2_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.0f ppm", min_co2);
    snprintf(now_buf, sizeof(now_buf), "NOW %.0f ppm", latest_co2);
    snprintf(max_buf, sizeof(max_buf), "MAX %.0f ppm", max_co2);
    safe_label_set_text(co2_graph_label_min_, min_buf);
    safe_label_set_text(co2_graph_label_now_, now_buf);
    safe_label_set_text(co2_graph_label_max_, max_buf);
}

void UiController::ensure_co2_zone_overlay() {
    if (!objects.co2_info_graph || !objects.chart_co2_info) {
        return;
    }

    if (!co2_graph_zone_overlay_ || !lv_obj_is_valid(co2_graph_zone_overlay_) ||
        lv_obj_get_parent(co2_graph_zone_overlay_) != objects.co2_info_graph) {
        co2_graph_zone_overlay_ = lv_obj_create(objects.co2_info_graph);
        lv_obj_clear_flag(co2_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(co2_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(co2_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_co2_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_co2_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_co2_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_co2_info);

    lv_obj_set_pos(co2_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(co2_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(co2_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_co2_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = co2_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != co2_graph_zone_overlay_) {
            band = lv_obj_create(co2_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(co2_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_co2_info);
}

void UiController::update_co2_zone_overlay(float y_min_display, float y_max_display) {
    ensure_co2_zone_overlay();
    if (!co2_graph_zone_overlay_ || !lv_obj_is_valid(co2_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(co2_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(co2_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (co2_graph_zone_bands_[i]) {
                lv_obj_add_flag(co2_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kCo2ZoneBounds[] = {-1000.0f, 800.0f, 1000.0f, 1500.0f, 100000.0f};
    static const GraphZoneTone kCo2ZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kCo2ZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_co2_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = co2_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kCo2ZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kCo2ZoneBounds[i];
        const float zone_high = kCo2ZoneBounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        const lv_color_t zone_color = resolve_graph_zone_color(kCo2ZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_co2_time_labels() {
    if (!objects.co2_info_graph || !objects.chart_co2_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.co2_info_graph) {
            label = lv_label_create(objects.co2_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(co2_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_co2_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = co2_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_co2_time_labels() {
    if (!objects.chart_co2_info || !objects.co2_info_graph) {
        return;
    }

    ensure_co2_time_labels();
    if (!co2_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = co2_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = co2_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_co2_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_co2_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_co2_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_co2_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = co2_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_co2_info_graph() {
    if (!objects.chart_co2_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_co2_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_co2_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (co2_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_co2_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_co2_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_co2_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_co2_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_co2_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_co2_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_co2_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_co2_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_co2_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_co2_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_co2_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_co2_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = co2_graph_points();
    lv_chart_set_point_count(objects.chart_co2_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_co2_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_co2_info,
                                     lv_obj_get_style_line_color(objects.chart_co2_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_co2_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_co2_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_co2 = FLT_MAX;
    float max_co2 = -FLT_MAX;
    float latest_co2 = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_CO2, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_co2 = value;
                    max_co2 = value;
                    has_values = true;
                } else {
                    if (value < min_co2) {
                        min_co2 = value;
                    }
                    if (value > max_co2) {
                        max_co2 = value;
                    }
                }
                latest_co2 = value;
                point_value = static_cast<lv_coord_t>(lroundf(value));
            }
        }
        lv_chart_set_value_by_id(objects.chart_co2_info, series, i, point_value);
    }

    float scale_min = has_values ? min_co2 : 700.0f;
    float scale_max = has_values ? max_co2 : 700.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 150.0f) {
        scale_span = 150.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 50.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_co2) ? latest_co2 : 700.0f;
        y_min_f = center - 150.0f;
        y_max_f = center + 150.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_co2_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_co2_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_co2_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_co2)) {
            latest_co2 = max_co2;
        }
        update_co2_graph_overlays(true, min_co2, max_co2, latest_co2);
    } else {
        update_co2_graph_overlays(false, 700.0f, 700.0f, 700.0f);
    }
    update_co2_time_labels();

    lv_chart_refresh(objects.chart_co2_info);
}

void UiController::ensure_co_graph_overlays() {
    if (!objects.chart_co_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_co_info) {
            label = lv_label_create(objects.chart_co_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(co_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(co_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(co_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_co_graph_overlays(bool has_values,
                                            float min_co,
                                            float max_co,
                                            float latest_co) {
    if (!objects.chart_co_info) {
        return;
    }

    ensure_co_graph_overlays();
    if (!co_graph_label_min_ || !co_graph_label_now_ || !co_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_co_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_co_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {co_graph_label_min_, co_graph_label_now_, co_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        safe_label_set_text(co_graph_label_min_, "MIN --");
        safe_label_set_text(co_graph_label_now_, "NOW --");
        safe_label_set_text(co_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.1f ppm", min_co);
    snprintf(now_buf, sizeof(now_buf), "NOW %.1f ppm", latest_co);
    snprintf(max_buf, sizeof(max_buf), "MAX %.1f ppm", max_co);
    safe_label_set_text(co_graph_label_min_, min_buf);
    safe_label_set_text(co_graph_label_now_, now_buf);
    safe_label_set_text(co_graph_label_max_, max_buf);
}

void UiController::ensure_co_zone_overlay() {
    if (!objects.co_info_graph || !objects.chart_co_info) {
        return;
    }

    if (!co_graph_zone_overlay_ || !lv_obj_is_valid(co_graph_zone_overlay_) ||
        lv_obj_get_parent(co_graph_zone_overlay_) != objects.co_info_graph) {
        co_graph_zone_overlay_ = lv_obj_create(objects.co_info_graph);
        lv_obj_clear_flag(co_graph_zone_overlay_, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(co_graph_zone_overlay_, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_top(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_bottom(co_graph_zone_overlay_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_co_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_co_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_co_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_co_info);

    lv_obj_set_pos(co_graph_zone_overlay_, chart_x, chart_y);
    lv_obj_set_size(co_graph_zone_overlay_, chart_w, chart_h);
    lv_obj_set_style_radius(co_graph_zone_overlay_,
                            lv_obj_get_style_radius(objects.chart_co_info, LV_PART_MAIN),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *&band = co_graph_zone_bands_[i];
        if (!band || !lv_obj_is_valid(band) || lv_obj_get_parent(band) != co_graph_zone_overlay_) {
            band = lv_obj_create(co_graph_zone_overlay_);
            lv_obj_clear_flag(band, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_move_background(band);
    }

    lv_obj_move_background(co_graph_zone_overlay_);
    lv_obj_move_foreground(objects.chart_co_info);
}

void UiController::update_co_zone_overlay(float y_min_display, float y_max_display) {
    ensure_co_zone_overlay();
    if (!co_graph_zone_overlay_ || !lv_obj_is_valid(co_graph_zone_overlay_)) {
        return;
    }

    const lv_coord_t width = lv_obj_get_width(co_graph_zone_overlay_);
    const lv_coord_t height = lv_obj_get_height(co_graph_zone_overlay_);
    if (width <= 0 || height <= 0 || !isfinite(y_min_display) || !isfinite(y_max_display) || y_max_display <= y_min_display) {
        for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
            if (co_graph_zone_bands_[i]) {
                lv_obj_add_flag(co_graph_zone_bands_[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        return;
    }

    static const float kCoZoneBounds[] = {
        -1000.0f,
        Config::AQ_CO_GREEN_MAX_PPM,
        Config::AQ_CO_YELLOW_MAX_PPM,
        Config::AQ_CO_ORANGE_MAX_PPM,
        100000.0f,
    };
    static const GraphZoneTone kCoZoneTones[] = {
        GRAPH_ZONE_GREEN,
        GRAPH_ZONE_YELLOW,
        GRAPH_ZONE_ORANGE,
        GRAPH_ZONE_RED,
    };
    constexpr uint8_t kCoZoneCount = 4;

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_co_info, LV_PART_MAIN);
    const float denom = y_max_display - y_min_display;

    for (uint8_t i = 0; i < kMaxGraphZoneBands; ++i) {
        lv_obj_t *band = co_graph_zone_bands_[i];
        if (!band) {
            continue;
        }
        if (i >= kCoZoneCount) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float zone_low = kCoZoneBounds[i];
        const float zone_high = kCoZoneBounds[i + 1];
        if (!isfinite(zone_low) || !isfinite(zone_high) || zone_high <= zone_low) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        const float visible_low = fmaxf(zone_low, y_min_display);
        const float visible_high = fminf(zone_high, y_max_display);
        if (!(visible_high > visible_low)) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        float top_ratio = (y_max_display - visible_high) / denom;
        float bottom_ratio = (y_max_display - visible_low) / denom;
        if (top_ratio < 0.0f) top_ratio = 0.0f;
        if (top_ratio > 1.0f) top_ratio = 1.0f;
        if (bottom_ratio < 0.0f) bottom_ratio = 0.0f;
        if (bottom_ratio > 1.0f) bottom_ratio = 1.0f;

        lv_coord_t top = static_cast<lv_coord_t>(lroundf(top_ratio * static_cast<float>(height)));
        lv_coord_t bottom = static_cast<lv_coord_t>(lroundf(bottom_ratio * static_cast<float>(height)));
        if (bottom <= top) {
            bottom = static_cast<lv_coord_t>(top + 1);
        }
        if (top < 0) top = 0;
        if (bottom > height) bottom = height;
        if (bottom <= top) {
            lv_obj_add_flag(band, LV_OBJ_FLAG_HIDDEN);
            continue;
        }

        lv_obj_set_pos(band, 0, top);
        lv_obj_set_size(band, width, static_cast<lv_coord_t>(bottom - top));
        const lv_color_t zone_color = resolve_graph_zone_color(kCoZoneTones[i], chart_bg);
        lv_obj_set_style_bg_color(band, zone_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(band);
    }
}

void UiController::ensure_co_time_labels() {
    if (!objects.co_info_graph || !objects.chart_co_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.co_info_graph) {
            label = lv_label_create(objects.co_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(co_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_co_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = co_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_co_time_labels() {
    if (!objects.chart_co_info || !objects.co_info_graph) {
        return;
    }

    ensure_co_time_labels();
    if (!co_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = co_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = co_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_co_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_co_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_co_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_co_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = co_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_co_info_graph() {
    if (!objects.chart_co_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_co_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_co_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (co_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_co_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_co_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_co_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_co_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_co_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_co_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_co_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_co_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_co_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_co_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_co_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_co_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = co_graph_points();
    lv_chart_set_point_count(objects.chart_co_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_co_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_co_info,
                                     lv_obj_get_style_line_color(objects.chart_co_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_co_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_co_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_co = FLT_MAX;
    float max_co = -FLT_MAX;
    float latest_co = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_CO, value, valid) &&
                valid && isfinite(value) && value >= 0.0f) {
                if (!has_values) {
                    min_co = value;
                    max_co = value;
                    has_values = true;
                } else {
                    if (value < min_co) {
                        min_co = value;
                    }
                    if (value > max_co) {
                        max_co = value;
                    }
                }
                latest_co = value;
                point_value = static_cast<lv_coord_t>(lroundf(value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_co_info, series, i, point_value);
    }

    float scale_min = has_values ? min_co : 0.0f;
    float scale_max = has_values ? max_co : Config::AQ_CO_GREEN_MAX_PPM;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 5.0f) {
        scale_span = 5.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 1.0f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_co) ? latest_co : (Config::AQ_CO_GREEN_MAX_PPM * 0.5f);
        y_min_f = center - 5.0f;
        y_max_f = center + 5.0f;
    }
    if (y_min_f < 0.0f) {
        y_min_f = 0.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f * 10.0f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f * 10.0f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_co_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_co_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    update_co_zone_overlay(y_min_f, y_max_f);

    if (has_values) {
        if (!isfinite(latest_co)) {
            latest_co = max_co;
        }
        update_co_graph_overlays(true, min_co, max_co, latest_co);
    } else {
        const float fallback = Config::AQ_CO_GREEN_MAX_PPM * 0.5f;
        update_co_graph_overlays(false, fallback, fallback, fallback);
    }
    update_co_time_labels();

    lv_chart_refresh(objects.chart_co_info);
}

void UiController::ensure_pressure_graph_overlays() {
    if (!objects.chart_pressure_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.chart_pressure_info) {
            label = lv_label_create(objects.chart_pressure_info);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(label, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_70, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_align(label, align, x_ofs, y_ofs);
        lv_obj_move_foreground(label);
    };

    ensure_label(pressure_graph_label_min_, LV_ALIGN_BOTTOM_LEFT, 8, -6);
    ensure_label(pressure_graph_label_now_, LV_ALIGN_TOP_LEFT, 8, 6);
    ensure_label(pressure_graph_label_max_, LV_ALIGN_TOP_RIGHT, -8, 6);
}

void UiController::update_pressure_graph_overlays(bool has_values,
                                                  float min_pressure,
                                                  float max_pressure,
                                                  float latest_pressure) {
    if (!objects.chart_pressure_info) {
        return;
    }

    ensure_pressure_graph_overlays();
    if (!pressure_graph_label_min_ || !pressure_graph_label_now_ || !pressure_graph_label_max_) {
        return;
    }

    const lv_color_t chart_bg = lv_obj_get_style_bg_color(objects.chart_pressure_info, LV_PART_MAIN);
    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_pressure_info, LV_PART_MAIN);
    const lv_color_t text = active_text_color();
    const lv_color_t badge_bg = lv_color_mix(border, chart_bg, LV_OPA_60);

    lv_obj_t *labels[] = {pressure_graph_label_min_, pressure_graph_label_now_, pressure_graph_label_max_};
    for (lv_obj_t *label : labels) {
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(label, badge_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(label, border, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (!has_values) {
        safe_label_set_text(pressure_graph_label_min_, "MIN --");
        safe_label_set_text(pressure_graph_label_now_, "NOW --");
        safe_label_set_text(pressure_graph_label_max_, "MAX --");
        return;
    }

    char min_buf[32];
    char now_buf[32];
    char max_buf[32];
    snprintf(min_buf, sizeof(min_buf), "MIN %.1f hPa", min_pressure);
    snprintf(now_buf, sizeof(now_buf), "NOW %.1f hPa", latest_pressure);
    snprintf(max_buf, sizeof(max_buf), "MAX %.1f hPa", max_pressure);
    safe_label_set_text(pressure_graph_label_min_, min_buf);
    safe_label_set_text(pressure_graph_label_now_, now_buf);
    safe_label_set_text(pressure_graph_label_max_, max_buf);
}

void UiController::ensure_pressure_time_labels() {
    if (!objects.pressure_info_graph || !objects.chart_pressure_info) {
        return;
    }

    auto ensure_label = [this](lv_obj_t *&label) {
        if (!label || !lv_obj_is_valid(label) || lv_obj_get_parent(label) != objects.pressure_info_graph) {
            label = lv_label_create(objects.pressure_info_graph);
            lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_text_font(label, &ui_font_jet_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    };

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        ensure_label(pressure_graph_time_labels_[i]);
    }

    const lv_color_t border = lv_obj_get_style_border_color(objects.chart_pressure_info, LV_PART_MAIN);
    const lv_color_t text = lv_color_mix(active_text_color(), border, LV_OPA_30);
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = pressure_graph_time_labels_[i];
        if (!label) {
            continue;
        }
        lv_obj_set_style_text_color(label, text, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(label, LV_OPA_80, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_pressure_time_labels() {
    if (!objects.chart_pressure_info || !objects.pressure_info_graph) {
        return;
    }

    ensure_pressure_time_labels();
    if (!pressure_graph_time_labels_[0]) {
        return;
    }

    const uint16_t points = pressure_graph_points();
    const uint32_t step_s = Config::CHART_HISTORY_STEP_MS / 1000UL;
    const uint32_t span_points = (points > 1) ? static_cast<uint32_t>(points - 1) : 1U;
    uint32_t duration_s = step_s * span_points;
    if (duration_s == 0) {
        duration_s = 3600U;
    }

    bool absolute_time = timeManager.isSystemTimeValid();
    time_t end_epoch = static_cast<time_t>(chartsHistory.latestEpoch());
    if (!absolute_time || end_epoch <= Config::TIME_VALID_EPOCH) {
        end_epoch = time(nullptr);
        if (end_epoch <= Config::TIME_VALID_EPOCH) {
            absolute_time = false;
        }
    }

    constexpr uint8_t kLastTick = kTempGraphTimeTickCount - 1;
    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = pressure_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        const uint32_t ratio_num = static_cast<uint32_t>(kLastTick - i);
        const uint32_t offset_s = static_cast<uint32_t>(
            (static_cast<uint64_t>(duration_s) * static_cast<uint64_t>(ratio_num)) / static_cast<uint64_t>(kLastTick));

        char buf[24];
        bool formatted = false;
        if (absolute_time) {
            const time_t tick_epoch = end_epoch - static_cast<time_t>(offset_s);
            formatted = format_epoch_hhmm(tick_epoch, buf, sizeof(buf));
        }
        if (!formatted) {
            format_relative_time_label(offset_s, buf, sizeof(buf));
        }
        safe_label_set_text(label, buf);
    }

    const lv_coord_t chart_x = lv_obj_get_x(objects.chart_pressure_info);
    const lv_coord_t chart_y = lv_obj_get_y(objects.chart_pressure_info);
    const lv_coord_t chart_w = lv_obj_get_width(objects.chart_pressure_info);
    const lv_coord_t chart_h = lv_obj_get_height(objects.chart_pressure_info);
    const lv_coord_t label_y = chart_y + chart_h + 4;

    for (uint8_t i = 0; i < kTempGraphTimeTickCount; ++i) {
        lv_obj_t *label = pressure_graph_time_labels_[i];
        if (!label) {
            continue;
        }

        lv_obj_update_layout(label);
        const lv_coord_t label_w = lv_obj_get_width(label);
        lv_coord_t tick_x = chart_x;
        if (chart_w > 1) {
            tick_x = chart_x + static_cast<lv_coord_t>(
                (static_cast<int32_t>(chart_w - 1) * static_cast<int32_t>(i)) / static_cast<int32_t>(kLastTick));
        }

        lv_coord_t label_x = tick_x - (label_w / 2);
        const lv_coord_t min_x = chart_x;
        const lv_coord_t max_x = chart_x + chart_w - label_w;
        if (label_x < min_x) {
            label_x = min_x;
        }
        if (label_x > max_x) {
            label_x = max_x;
        }

        lv_obj_set_pos(label, label_x, label_y);
        lv_obj_move_foreground(label);
    }
}

void UiController::update_pressure_info_graph() {
    if (!objects.chart_pressure_info) {
        return;
    }

    lv_color_t card_bg = lv_color_hex(0xff160c09);
    lv_color_t border_color = color_card_border();
    if (objects.card_co2_pro) {
        card_bg = lv_obj_get_style_bg_color(objects.card_co2_pro, LV_PART_MAIN);
        border_color = lv_obj_get_style_border_color(objects.card_co2_pro, LV_PART_MAIN);
    }

    const lv_color_t text_color = active_text_color();
    const lv_color_t grid_color = lv_color_mix(border_color, card_bg, LV_OPA_50);
    const lv_color_t line_color = lv_color_mix(border_color, text_color, LV_OPA_40);

    lv_chart_set_type(objects.chart_pressure_info, LV_CHART_TYPE_LINE);
    lv_chart_set_update_mode(objects.chart_pressure_info, LV_CHART_UPDATE_MODE_SHIFT);

    uint8_t vertical_divisions = 13;
    if (pressure_graph_range_ == TEMP_GRAPH_RANGE_24H) {
        vertical_divisions = 25;
    }

    lv_obj_set_style_bg_color(objects.chart_pressure_info, card_bg, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(objects.chart_pressure_info, LV_OPA_30, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(objects.chart_pressure_info, border_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(objects.chart_pressure_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(objects.chart_pressure_info, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(objects.chart_pressure_info, grid_color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pressure_info, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pressure_info, 1, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_line_color(objects.chart_pressure_info, line_color, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(objects.chart_pressure_info, 3, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_line_opa(objects.chart_pressure_info, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_size(objects.chart_pressure_info, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    const uint16_t points = pressure_graph_points();
    lv_chart_set_point_count(objects.chart_pressure_info, points);

    lv_chart_series_t *series = lv_chart_get_series_next(objects.chart_pressure_info, nullptr);
    if (!series) {
        series = lv_chart_add_series(objects.chart_pressure_info,
                                     lv_obj_get_style_line_color(objects.chart_pressure_info, LV_PART_ITEMS),
                                     LV_CHART_AXIS_PRIMARY_Y);
    }
    if (!series) {
        return;
    }

    series->color = lv_obj_get_style_line_color(objects.chart_pressure_info, LV_PART_ITEMS);
    lv_chart_set_all_value(objects.chart_pressure_info, series, LV_CHART_POINT_NONE);

    const uint16_t total_count = chartsHistory.count();
    const uint16_t available = (total_count < points) ? total_count : points;
    const uint16_t missing_prefix = points - available;
    const uint16_t start_offset = total_count - available;

    bool has_values = false;
    float min_p = FLT_MAX;
    float max_p = -FLT_MAX;
    float latest_p = NAN;

    for (uint16_t i = 0; i < points; ++i) {
        lv_coord_t point_value = LV_CHART_POINT_NONE;
        if (i >= missing_prefix) {
            const uint16_t offset = start_offset + (i - missing_prefix);
            float value = 0.0f;
            bool valid = false;
            if (chartsHistory.metricValueFromOldest(offset, ChartsHistory::METRIC_PRESSURE, value, valid) &&
                valid && isfinite(value)) {
                if (!has_values) {
                    min_p = value;
                    max_p = value;
                    has_values = true;
                } else {
                    if (value < min_p) {
                        min_p = value;
                    }
                    if (value > max_p) {
                        max_p = value;
                    }
                }
                latest_p = value;
                point_value = static_cast<lv_coord_t>(lroundf(value * 10.0f));
            }
        }
        lv_chart_set_value_by_id(objects.chart_pressure_info, series, i, point_value);
    }

    float scale_min = has_values ? min_p : 1013.0f;
    float scale_max = has_values ? max_p : 1013.0f;
    float scale_span = scale_max - scale_min;
    if (!isfinite(scale_span) || scale_span < 2.0f) {
        scale_span = 2.0f;
    }

    float step = graph_nice_step(scale_span / 4.0f);
    if (!isfinite(step) || step <= 0.0f) {
        step = 0.5f;
    }

    float y_min_f = floorf((scale_min - (step * 0.9f)) / step) * step;
    float y_max_f = ceilf((scale_max + (step * 0.9f)) / step) * step;
    if ((y_max_f - y_min_f) < (step * 2.0f)) {
        y_min_f -= step;
        y_max_f += step;
    }
    if (!isfinite(y_min_f) || !isfinite(y_max_f) || y_max_f <= y_min_f) {
        const float center = isfinite(latest_p) ? latest_p : 1013.0f;
        y_min_f = center - 2.0f;
        y_max_f = center + 2.0f;
    }

    lv_coord_t y_min = static_cast<lv_coord_t>(floorf(y_min_f * 10.0f));
    lv_coord_t y_max = static_cast<lv_coord_t>(ceilf(y_max_f * 10.0f));
    if (y_max <= y_min) {
        y_max = static_cast<lv_coord_t>(y_min + 10);
    }

    int32_t horizontal_divisions = static_cast<int32_t>(lroundf((y_max_f - y_min_f) / step));
    if (horizontal_divisions < 3) {
        horizontal_divisions = 3;
    }
    if (horizontal_divisions > 12) {
        horizontal_divisions = 12;
    }

    lv_chart_set_div_line_count(objects.chart_pressure_info,
                                static_cast<uint8_t>(horizontal_divisions),
                                vertical_divisions);
    lv_chart_set_range(objects.chart_pressure_info, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);

    if (has_values) {
        if (!isfinite(latest_p)) {
            latest_p = max_p;
        }
        update_pressure_graph_overlays(true, min_p, max_p, latest_p);
    } else {
        update_pressure_graph_overlays(false, 1013.0f, 1013.0f, 1013.0f);
    }
    update_pressure_time_labels();

    lv_chart_refresh(objects.chart_pressure_info);
}
