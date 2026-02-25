#include <limits>

#include <unity.h>

#include "web/OtaDeferredRestart.h"

void setUp() {}
void tearDown() {}

void test_deadline_reached_handles_wraparound() {
    const uint32_t start = std::numeric_limits<uint32_t>::max() - 20u;
    const uint32_t due = start + 40u; // wraps around

    TEST_ASSERT_FALSE(OtaDeferredRestart::deadline_reached(start + 10u, due));
    TEST_ASSERT_TRUE(OtaDeferredRestart::deadline_reached(start + 40u, due));
    TEST_ASSERT_TRUE(OtaDeferredRestart::deadline_reached(start + 60u, due));
}

void test_controller_schedule_poll_consume_flow() {
    OtaDeferredRestart::Controller c;
    c.reset();

    TEST_ASSERT_FALSE(c.is_scheduled());
    TEST_ASSERT_FALSE(c.is_requested());
    TEST_ASSERT_FALSE(c.is_busy(false));
    TEST_ASSERT_TRUE(c.is_busy(true));

    c.schedule(1000u, 200u);
    TEST_ASSERT_TRUE(c.is_scheduled());
    TEST_ASSERT_FALSE(c.is_requested());
    TEST_ASSERT_TRUE(c.is_busy(false));
    TEST_ASSERT_EQUAL_UINT32(1200u, c.due_ms());

    TEST_ASSERT_FALSE(c.poll(1199u));
    TEST_ASSERT_TRUE(c.is_scheduled());
    TEST_ASSERT_FALSE(c.is_requested());

    TEST_ASSERT_TRUE(c.poll(1200u));
    TEST_ASSERT_FALSE(c.is_scheduled());
    TEST_ASSERT_TRUE(c.is_requested());
    TEST_ASSERT_TRUE(c.is_busy(false));

    TEST_ASSERT_TRUE(c.consume_request());
    TEST_ASSERT_FALSE(c.is_requested());
    TEST_ASSERT_FALSE(c.is_busy(false));
    TEST_ASSERT_FALSE(c.consume_request());
}

void test_controller_reschedule_replaces_previous_deadline() {
    OtaDeferredRestart::Controller c;
    c.schedule(100u, 50u);   // due 150
    c.schedule(120u, 200u);  // due 320

    TEST_ASSERT_TRUE(c.is_scheduled());
    TEST_ASSERT_EQUAL_UINT32(320u, c.due_ms());
    TEST_ASSERT_FALSE(c.poll(319u));
    TEST_ASSERT_TRUE(c.poll(320u));
    TEST_ASSERT_TRUE(c.consume_request());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_deadline_reached_handles_wraparound);
    RUN_TEST(test_controller_schedule_poll_consume_flow);
    RUN_TEST(test_controller_reschedule_replaces_previous_deadline);
    return UNITY_END();
}
