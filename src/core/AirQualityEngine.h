#pragma once

#include <stdint.h>

#include "config/AppData.h"

namespace AirQualityEngine {

enum class Metric : uint8_t {
    None = 0,
    PM05,
    PM1,
    PM25,
    PM4,
    PM10,
    CO2,
    VOC,
    NOX,
    HCHO,
    CO,
};

enum class Group : uint8_t {
    None = 0,
    Particulates,
    Ventilation,
    ReactiveGas,
    ToxicGas,
};

enum class Band : uint8_t {
    Invalid = 0,
    Excellent,
    Good,
    Moderate,
    Poor,
};

struct MetricEvaluation {
    Metric metric = Metric::None;
    Group group = Group::None;
    bool valid = false;
    int score = 0;
};

struct GroupEvaluation {
    Group group = Group::None;
    bool valid = false;
    int score = 0;
    Metric dominant_metric = Metric::None;
};

struct Result {
    bool valid = false;
    int score = 0;
    Band band = Band::Invalid;
    Metric dominant_metric = Metric::None;
    Group dominant_group = Group::None;
    uint8_t valid_group_count = 0;
    GroupEvaluation pm{};
    GroupEvaluation ventilation{};
    GroupEvaluation reactive_gas{};
    GroupEvaluation toxic_gas{};
};

MetricEvaluation evaluateMetric(Metric metric, const SensorData &data, bool gas_warmup);
Band bandFromScore(int score);
Result evaluate(const SensorData &data, bool gas_warmup);

} // namespace AirQualityEngine
