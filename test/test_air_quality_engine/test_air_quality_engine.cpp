#include <unity.h>

#include "config/AppConfig.h"
#include "core/AirQualityEngine.h"

using AirQualityEngine::Band;
using AirQualityEngine::Group;
using AirQualityEngine::Metric;

void setUp() {}
void tearDown() {}

void test_air_quality_metric_pm05_uses_threshold_profile() {
    SensorData data{};
    data.pm05_valid = true;
    data.pm05 = Config::AQ_PM05_YELLOW_MAX_PPCM3;

    const AirQualityEngine::MetricEvaluation metric =
        AirQualityEngine::evaluateMetric(Metric::PM05, data, false);

    TEST_ASSERT_TRUE(metric.valid);
    TEST_ASSERT_EQUAL_INT(50, metric.score);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Group::Particulates), static_cast<int>(metric.group));
}

void test_air_quality_pm_group_picks_worst_metric_including_pm05() {
    SensorData data{};
    data.pm05_valid = true;
    data.pm05 = Config::AQ_PM05_ORANGE_MAX_PPCM3;
    data.pm25_valid = true;
    data.pm25 = Config::AQ_PM25_YELLOW_MAX_UGM3;
    data.pm10_valid = true;
    data.pm10 = Config::AQ_PM10_GREEN_MAX_UGM3;

    const AirQualityEngine::Result result = AirQualityEngine::evaluate(data, false);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_TRUE(result.pm.valid);
    TEST_ASSERT_EQUAL_INT(75, result.pm.score);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Metric::PM05),
                          static_cast<int>(result.pm.dominant_metric));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Metric::PM05),
                          static_cast<int>(result.dominant_metric));
}

void test_air_quality_warmup_excludes_reactive_gas_metrics() {
    SensorData data{};
    data.voc_valid = true;
    data.voc_index = Config::AQ_VOC_ORANGE_MAX_INDEX;
    data.nox_valid = true;
    data.nox_index = Config::AQ_NOX_ORANGE_MAX_INDEX;

    const AirQualityEngine::Result result = AirQualityEngine::evaluate(data, true);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_FALSE(result.reactive_gas.valid);
}

void test_air_quality_warmup_keeps_aqi_invalid_when_only_hcho_is_available() {
    SensorData data{};
    data.hcho_valid = true;
    data.hcho = 0.0f;

    const AirQualityEngine::Result result = AirQualityEngine::evaluate(data, true);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_TRUE(result.toxic_gas.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Metric::HCHO),
                          static_cast<int>(result.toxic_gas.dominant_metric));
}

void test_air_quality_warmup_allows_aqi_once_pm_or_co2_is_ready() {
    SensorData data{};
    data.hcho_valid = true;
    data.hcho = 0.0f;
    data.pm25_valid = true;
    data.pm25 = Config::AQ_PM25_GREEN_MAX_UGM3;

    const AirQualityEngine::Result result = AirQualityEngine::evaluate(data, true);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_TRUE(result.pm.valid);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Metric::PM25),
                          static_cast<int>(result.dominant_metric));
}

void test_air_quality_combines_multiple_valid_groups() {
    SensorData data{};
    data.pm05_valid = true;
    data.pm05 = Config::AQ_PM05_ORANGE_MAX_PPCM3;
    data.co2_valid = true;
    data.co2 = static_cast<int>(Config::AQ_CO2_YELLOW_MAX_PPM);
    data.voc_valid = true;
    data.voc_index = Config::AQ_VOC_GREEN_MAX_INDEX;

    const AirQualityEngine::Result result = AirQualityEngine::evaluate(data, false);

    TEST_ASSERT_TRUE(result.valid);
    TEST_ASSERT_EQUAL_UINT8(3, result.valid_group_count);
    TEST_ASSERT_EQUAL_INT(88, result.score);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Band::Poor), static_cast<int>(result.band));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Group::Particulates),
                          static_cast<int>(result.dominant_group));
}

void test_air_quality_toxic_group_prefers_co_and_absent_sensor_is_ignored() {
    SensorData data{};
    data.co_sensor_present = false;
    data.co_valid = true;
    data.co_ppm = Config::AQ_CO_YELLOW_MAX_PPM;
    data.hcho_valid = true;
    data.hcho = Config::AQ_HCHO_ORANGE_MAX_PPB;

    AirQualityEngine::Result without_co = AirQualityEngine::evaluate(data, false);
    TEST_ASSERT_TRUE(without_co.valid);
    TEST_ASSERT_EQUAL_INT(75, without_co.toxic_gas.score);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Metric::HCHO),
                          static_cast<int>(without_co.toxic_gas.dominant_metric));

    data.co_sensor_present = true;
    AirQualityEngine::Result with_co = AirQualityEngine::evaluate(data, false);
    TEST_ASSERT_TRUE(with_co.valid);
    TEST_ASSERT_EQUAL_INT(90, with_co.toxic_gas.score);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Metric::CO),
                          static_cast<int>(with_co.toxic_gas.dominant_metric));
}

void test_air_quality_returns_invalid_when_no_metrics_are_available() {
    SensorData data{};

    const AirQualityEngine::Result result = AirQualityEngine::evaluate(data, false);

    TEST_ASSERT_FALSE(result.valid);
    TEST_ASSERT_EQUAL_INT(0, result.score);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Band::Invalid), static_cast<int>(result.band));
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_air_quality_metric_pm05_uses_threshold_profile);
    RUN_TEST(test_air_quality_pm_group_picks_worst_metric_including_pm05);
    RUN_TEST(test_air_quality_warmup_excludes_reactive_gas_metrics);
    RUN_TEST(test_air_quality_warmup_keeps_aqi_invalid_when_only_hcho_is_available);
    RUN_TEST(test_air_quality_warmup_allows_aqi_once_pm_or_co2_is_ready);
    RUN_TEST(test_air_quality_combines_multiple_valid_groups);
    RUN_TEST(test_air_quality_toxic_group_prefers_co_and_absent_sensor_is_ignored);
    RUN_TEST(test_air_quality_returns_invalid_when_no_metrics_are_available);
    return UNITY_END();
}
