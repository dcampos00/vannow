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
    ArduinoMock::advanceMillis(15); // Release debounce
    TEST_ASSERT_FALSE(btn.checkEvent(action)); // Pressed -> WaitDoubleClick
    ArduinoMock::advanceMillis(320);
    TEST_ASSERT_TRUE(btn.checkEvent(action)); // Double-click window expired -> Click
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
    ArduinoMock::advanceMillis(15); // Release debounce
    TEST_ASSERT_TRUE(btn.checkEvent(action)); // Call at t=640: Holding -> Idle, returns Release
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

void test_encoder_handler_ccw_and_noop(void) {
    EncoderHandler encoder(5, 6, 7); // Pin A = 5, Pin B = 6, Pin SW = 7
    encoder.begin();
    
    ArduinoMock::setPinState(5, HIGH);
    ArduinoMock::setPinState(6, HIGH);
    
    ActionType action;
    int8_t steps = 0;
    TEST_ASSERT_FALSE(encoder.checkEvent(action, steps));
    
    // CCW: A goes LOW when B is LOW
    ArduinoMock::setPinState(6, LOW);
    ArduinoMock::setPinState(5, LOW);
    encoder.update();
    
    TEST_ASSERT_TRUE(encoder.checkEvent(action, steps));
    TEST_ASSERT_EQUAL(ActionType::EncoderTurn, action);
    TEST_ASSERT_EQUAL(-1, steps);
    
    // Ignore rising edge
    ArduinoMock::setPinState(5, HIGH);
    encoder.update();
    TEST_ASSERT_FALSE(encoder.checkEvent(action, steps));
}

void test_encoder_handler_button_click(void) {
    EncoderHandler encoder(5, 6, 7);
    encoder.begin();
    
    ArduinoMock::setPinState(7, HIGH);
    ActionType action;
    int8_t steps = 0;
    TEST_ASSERT_FALSE(encoder.checkEvent(action, steps));
    
    // Press SW (LOW)
    ArduinoMock::setPinState(7, LOW);
    TEST_ASSERT_FALSE(encoder.checkEvent(action, steps));
    
    // Debounce
    ArduinoMock::advanceMillis(10);
    TEST_ASSERT_FALSE(encoder.checkEvent(action, steps));
    
    ArduinoMock::advanceMillis(15);
    TEST_ASSERT_FALSE(encoder.checkEvent(action, steps));
    
    // Release SW (HIGH)
    ArduinoMock::setPinState(7, HIGH);
    ArduinoMock::advanceMillis(15); // Release debounce
    TEST_ASSERT_TRUE(encoder.checkEvent(action, steps));
    TEST_ASSERT_EQUAL(ActionType::Click, action);
    TEST_ASSERT_EQUAL(0, steps);
}

void test_power_manager_sleep_execution(void) {
    PowerManager pm(1500);
    pm.begin();
    
    uint8_t wakeupPins[3] = {10, 11, 12};
    pm.goToSleep(wakeupPins, 3);
    
    TEST_ASSERT_TRUE(true);
}

#include "BinaryMatrixHandler.h"

void test_binary_matrix_click(void) {
    BinaryMatrixHandler matrix(0, 1, 2);
    matrix.begin();

    // All pins default HIGH (no button pressed)
    ArduinoMock::setPinState(0, HIGH);
    ArduinoMock::setPinState(1, HIGH);
    ArduinoMock::setPinState(2, HIGH);

    uint8_t btnIdx = 255;
    ActionType action;
    TEST_ASSERT_FALSE(matrix.checkEvent(btnIdx, action));
    TEST_ASSERT_EQUAL(-1, matrix.readRawButton());

    // Test Button 2 (Code 3: pin 0 = LOW, pin 1 = LOW, pin 2 = HIGH)
    ArduinoMock::setPinState(0, LOW);
    ArduinoMock::setPinState(1, LOW);
    TEST_ASSERT_EQUAL(2, matrix.readRawButton());
    TEST_ASSERT_FALSE(matrix.checkEvent(btnIdx, action)); // Debounce started

    // Pass debounce (25 ms)
    ArduinoMock::advanceMillis(25);
    TEST_ASSERT_FALSE(matrix.checkEvent(btnIdx, action)); // Transitioned to Pressed

    // Release button
    ArduinoMock::setPinState(0, HIGH);
    ArduinoMock::setPinState(1, HIGH);
    ArduinoMock::advanceMillis(15); // Release debounce

    TEST_ASSERT_TRUE(matrix.checkEvent(btnIdx, action));
    TEST_ASSERT_EQUAL(2, btnIdx); // Button 2
    TEST_ASSERT_EQUAL(ActionType::Click, action);
}

void test_binary_matrix_hold_and_release(void) {
    BinaryMatrixHandler matrix(0, 1, 2);
    matrix.begin();

    ArduinoMock::setPinState(0, HIGH);
    ArduinoMock::setPinState(1, HIGH);
    ArduinoMock::setPinState(2, HIGH);

    // Press Button 4 (Code 5: pin 0 = LOW, pin 1 = HIGH, pin 2 = LOW)
    ArduinoMock::setPinState(0, LOW);
    ArduinoMock::setPinState(2, LOW);
    TEST_ASSERT_EQUAL(4, matrix.readRawButton());

    uint8_t btnIdx = 255;
    ActionType action;
    matrix.checkEvent(btnIdx, action);

    // Advance 25 ms debounce
    ArduinoMock::advanceMillis(25);
    matrix.checkEvent(btnIdx, action);

    // Advance 450 ms (hold threshold 400 ms)
    ArduinoMock::advanceMillis(450);
    TEST_ASSERT_TRUE(matrix.checkEvent(btnIdx, action));
    TEST_ASSERT_EQUAL(4, btnIdx);
    TEST_ASSERT_EQUAL(ActionType::StartHold, action);

    // Advance 150 ms (periodic hold repeat)
    ArduinoMock::advanceMillis(150);
    TEST_ASSERT_TRUE(matrix.checkEvent(btnIdx, action));
    TEST_ASSERT_EQUAL(4, btnIdx);
    TEST_ASSERT_EQUAL(ActionType::StartHold, action);

    // Release
    ArduinoMock::setPinState(0, HIGH);
    ArduinoMock::setPinState(2, HIGH);
    ArduinoMock::advanceMillis(15);
    TEST_ASSERT_TRUE(matrix.checkEvent(btnIdx, action));
    TEST_ASSERT_EQUAL(4, btnIdx);
    TEST_ASSERT_EQUAL(ActionType::Release, action);
}

void test_button_handler_double_click(void) {
    ButtonHandler btn(10, 0);
    btn.begin();
    ArduinoMock::setPinState(10, HIGH);
    ActionType action;

    ArduinoMock::setPinState(10, LOW);
    btn.checkEvent(action);
    ArduinoMock::advanceMillis(25);
    btn.checkEvent(action);

    ArduinoMock::setPinState(10, HIGH);
    ArduinoMock::advanceMillis(15);
    TEST_ASSERT_FALSE(btn.checkEvent(action));

    ArduinoMock::setPinState(10, LOW);
    TEST_ASSERT_FALSE(btn.checkEvent(action));
    ArduinoMock::advanceMillis(15);
    TEST_ASSERT_TRUE(btn.checkEvent(action));
    TEST_ASSERT_EQUAL(ActionType::DoubleClick, action);
}

void test_button_handler_latching_edges(void) {
    ButtonHandler sw(8, 0, ButtonHandler::InputMode::Latching);
    sw.begin();
    ArduinoMock::setPinState(8, HIGH);
    ActionType action;

    ArduinoMock::setPinState(8, LOW);
    TEST_ASSERT_FALSE(sw.checkEvent(action));
    ArduinoMock::advanceMillis(15);
    TEST_ASSERT_TRUE(sw.checkEvent(action));
    TEST_ASSERT_EQUAL(ActionType::Click, action);

    TEST_ASSERT_FALSE(sw.checkEvent(action));
    ArduinoMock::advanceMillis(200);
    TEST_ASSERT_FALSE(sw.checkEvent(action));

    ArduinoMock::setPinState(8, HIGH);
    ArduinoMock::advanceMillis(15);
    TEST_ASSERT_TRUE(sw.checkEvent(action));
    TEST_ASSERT_EQUAL(ActionType::Click, action);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_button_handler_click);
    RUN_TEST(test_button_handler_hold_and_release);
    RUN_TEST(test_encoder_handler_rotation);
    RUN_TEST(test_power_manager_watchdog);
    RUN_TEST(test_encoder_handler_ccw_and_noop);
    RUN_TEST(test_encoder_handler_button_click);
    RUN_TEST(test_power_manager_sleep_execution);
    RUN_TEST(test_binary_matrix_click);
    RUN_TEST(test_binary_matrix_hold_and_release);
    RUN_TEST(test_button_handler_double_click);
    RUN_TEST(test_button_handler_latching_edges);
    return UNITY_END();
}

