#include <unity.h>
#include <Arduino.h>
#include "DigitalChannel.h"
#include "DimmableChannel.h"
#include "SystemController.h"

void setUp(void) {
    ArduinoMock::reset();
}

void tearDown(void) {
    // Clean up if needed
}

void test_digital_channel_toggle(void) {
    DigitalChannel channel("TestPump", 5);
    channel.begin();
    
    // Initial state should be inactive (LOW)
    TEST_ASSERT_FALSE(channel.getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(5));
    TEST_ASSERT_EQUAL(OUTPUT, ArduinoMock::getPinMode(5));
    
    // Click action should toggle state to active (HIGH)
    channel.handleAction(ActionType::Click, 0);
    TEST_ASSERT_TRUE(channel.getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(5));
    
    // Second Click action should toggle state back to inactive (LOW)
    channel.handleAction(ActionType::Click, 0);
    TEST_ASSERT_FALSE(channel.getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(5));
}

void test_dimmable_channel_fade(void) {
    DimmableChannel channel("TestLight", 12);
    channel.begin();
    
    TEST_ASSERT_FALSE(channel.getState());
    TEST_ASSERT_EQUAL(0, channel.getBrightness());
    
    // Click to turn ON: should target memory brightness of 80%
    channel.handleAction(ActionType::Click, 0);
    TEST_ASSERT_TRUE(channel.getState());
    
    // Transition updates: fades 1% every 5ms
    // Simulate 200ms by calling update 40 times (40 * 5ms = 200ms)
    for (int i = 0; i < 40; i++) {
        ArduinoMock::advanceMillis(5);
        channel.update();
    }
    TEST_ASSERT_EQUAL(40, channel.getBrightness());
    
    // Simulate another 200ms to complete transition to 80%
    for (int i = 0; i < 40; i++) {
        ArduinoMock::advanceMillis(5);
        channel.update();
    }
    TEST_ASSERT_EQUAL(80, channel.getBrightness());
}

void test_dimmable_channel_hold_ramping(void) {
    DimmableChannel channel("TestLight", 12);
    channel.begin();
    channel.setBrightness(50); // Set initial brightness
    
    // Start hold (ramps in reverse of last direction: default direction was up, so now down)
    channel.handleAction(ActionType::StartHold, 0);
    
    // Ramps 1% every 30ms
    // Simulate holding button for 300ms (10 steps down -> 40%)
    for (int i = 0; i < 10; i++) {
        ArduinoMock::advanceMillis(30);
        channel.update();
    }
    TEST_ASSERT_EQUAL(40, channel.getBrightness());
    
    // Release hold: should save 40% as memory
    channel.handleAction(ActionType::Release, 0);
    channel.update();
    
    // Click to turn OFF
    channel.handleAction(ActionType::Click, 0);
    channel.update();
    
    // Fade out (40 steps * 5ms = 200ms)
    for (int i = 0; i < 40; i++) {
        ArduinoMock::advanceMillis(5);
        channel.update();
    }
    TEST_ASSERT_EQUAL(0, channel.getBrightness());
    
    // Click to turn ON: should retrieve stored memory of 40%
    channel.handleAction(ActionType::Click, 0);
    channel.update();
    
    // Fade in (40 steps * 5ms = 200ms)
    for (int i = 0; i < 40; i++) {
        ArduinoMock::advanceMillis(5);
        channel.update();
    }
    TEST_ASSERT_EQUAL(40, channel.getBrightness());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_digital_channel_toggle);
    RUN_TEST(test_dimmable_channel_fade);
    RUN_TEST(test_dimmable_channel_hold_ramping);
    return UNITY_END();
}
