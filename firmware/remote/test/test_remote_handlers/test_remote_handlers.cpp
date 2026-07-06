#include <unity.h>
#include <Arduino.h>
#include "ButtonHandler.h"
#include "EncoderHandler.h"
#include "PowerManager.h"

void setUp(void) {
    ArduinoMock::reset();
}

void tearDown(void) {
    // Clean up
}

void test_button_handler_click(void) {
    ButtonHandler btn(10, 0); // Pin 10, ID 0
    btn.begin();
    
    // Initial state: Pin 10 is HIGH (not pressed)
    ArduinoMock::setPinState(10, HIGH);
    
    ActionType action;
    TEST_ASSERT_FALSE(btn.checkEvent(action));
    
    // Press button (LOW) at t = 0
    ArduinoMock::setPinState(10, LOW);
    TEST_ASSERT_FALSE(btn.checkEvent(action)); // Call at t=0: Idle -> Debounce
    
    // Check click debounce (15ms). Advance 5ms -> debounce incomplete
    ArduinoMock::advanceMillis(5);
    TEST_ASSERT_FALSE(btn.checkEvent(action));
    
    // Advance remaining 20ms to pass debounce (total 25ms)
    ArduinoMock::advanceMillis(20);
    TEST_ASSERT_FALSE(btn.checkEvent(action)); // Call at t=25: Debounce -> Pressed
    
    // Release button (HIGH)
    ArduinoMock::setPinState(10, HIGH);
    TEST_ASSERT_TRUE(btn.checkEvent(action)); // Call at t=25: Pressed -> Idle, returns Click
    TEST_ASSERT_EQUAL(ActionType::Click, action);
}

void test_button_handler_hold_and_release(void) {
    ButtonHandler btn(10, 0);
    btn.begin();
    
    ArduinoMock::setPinState(10, HIGH);
    ActionType action;
    
    // Press button (LOW) at t = 0
    ArduinoMock::setPinState(10, LOW);
    TEST_ASSERT_FALSE(btn.checkEvent(action)); // Call at t=0: Idle -> Debounce
    
    ArduinoMock::advanceMillis(25); // Debounce
    TEST_ASSERT_FALSE(btn.checkEvent(action)); // Call at t=25: Debounce -> Pressed
    
    // Advance 600ms (threshold is 500ms)
    ArduinoMock::advanceMillis(600);
    TEST_ASSERT_TRUE(btn.checkEvent(action)); // Call at t=625: Pressed -> Holding, returns StartHold
    TEST_ASSERT_EQUAL(ActionType::StartHold, action);
    
    // Verify it doesn't trigger hold again immediately
    TEST_ASSERT_FALSE(btn.checkEvent(action));
    
    // Release button
    ArduinoMock::setPinState(10, HIGH);
    TEST_ASSERT_TRUE(btn.checkEvent(action)); // Call at t=625: Holding -> Idle, returns Release
    TEST_ASSERT_EQUAL(ActionType::Release, action);
}

void test_encoder_handler_rotation(void) {
    EncoderHandler encoder(5, 6, 7); // Pin A = 5, Pin B = 6, Pin SW = 7
    encoder.begin();
    
    // Initial A and B are HIGH
    ArduinoMock::setPinState(5, HIGH);
    ArduinoMock::setPinState(6, HIGH);
    
    ActionType action;
    int8_t steps = 0;
    TEST_ASSERT_FALSE(encoder.checkEvent(action, steps));
    
    // Simulating Clockwise Turn:
    // A goes LOW (falling edge). Since B is HIGH, this registers as a CW step.
    ArduinoMock::setPinState(5, LOW);
    encoder.update();
    
    TEST_ASSERT_TRUE(encoder.checkEvent(action, steps));
    TEST_ASSERT_EQUAL(ActionType::EncoderTurn, action);
    TEST_ASSERT_EQUAL(1, steps);
}

void test_power_manager_watchdog(void) {
    PowerManager pm(1500); // 1.5s timeout
    pm.begin();
    
    TEST_ASSERT_FALSE(pm.isExpired());
    
    // Advance 1000ms -> not expired
    ArduinoMock::advanceMillis(1000);
    TEST_ASSERT_FALSE(pm.isExpired());
    
    // Feed the power manager watchdog
    pm.feed();
    
    // Advance another 1000ms -> still not expired (since feed reset it)
    ArduinoMock::advanceMillis(1000);
    TEST_ASSERT_FALSE(pm.isExpired());
    
    // Advance 600ms more (total 1600ms since last feed) -> expired!
    ArduinoMock::advanceMillis(600);
    TEST_ASSERT_TRUE(pm.isExpired());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_button_handler_click);
    RUN_TEST(test_button_handler_hold_and_release);
    RUN_TEST(test_encoder_handler_rotation);
    RUN_TEST(test_power_manager_watchdog);
    return UNITY_END();
}
