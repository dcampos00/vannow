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

void test_channel_base_properties(void) {
    DigitalChannel digChan("DigName", 9);
    TEST_ASSERT_EQUAL_STRING("DigName", digChan.getName());
    TEST_ASSERT_EQUAL(9, digChan.getPin());
    TEST_ASSERT_FALSE(digChan.isDimmable());

    DimmableChannel dimChan("DimName", 10);
    TEST_ASSERT_EQUAL_STRING("DimName", dimChan.getName());
    TEST_ASSERT_EQUAL(10, dimChan.getPin());
    TEST_ASSERT_TRUE(dimChan.isDimmable());
}

void test_dimmable_channel_encoder_turn(void) {
    DimmableChannel channel("TestLight", 12);
    channel.begin();
    
    channel.setBrightness(50);
    TEST_ASSERT_EQUAL(50, channel.getBrightness());
    
    // Rotate CW: increase by 25% -> 75%
    channel.handleAction(ActionType::EncoderTurn, 5);
    TEST_ASSERT_EQUAL(75, channel.getBrightness());
    
    // Rotate CCW: decrease by 15% -> 60%
    channel.handleAction(ActionType::EncoderTurn, -3);
    TEST_ASSERT_EQUAL(60, channel.getBrightness());
    
    // Saturate at 100%
    channel.handleAction(ActionType::EncoderTurn, 10);
    TEST_ASSERT_EQUAL(100, channel.getBrightness());
    
    // Saturate at 0%
    channel.handleAction(ActionType::EncoderTurn, -25);
    TEST_ASSERT_EQUAL(0, channel.getBrightness());
    TEST_ASSERT_FALSE(channel.getState());
}

void test_dimmable_channel_ramping_limits(void) {
    DimmableChannel channel("TestLight", 12);
    channel.begin();
    
    channel.setBrightness(98);
    // Start hold toggles direction to -1, call again to make it 1 (up)
    channel.handleAction(ActionType::StartHold, 0);
    channel.handleAction(ActionType::StartHold, 0);
    
    // 98% -> 99%
    ArduinoMock::advanceMillis(30);
    channel.update();
    TEST_ASSERT_EQUAL(99, channel.getBrightness());
    
    // 99% -> 100% (direction flips to -1)
    ArduinoMock::advanceMillis(30);
    channel.update();
    TEST_ASSERT_EQUAL(100, channel.getBrightness());
    
    // 100% -> 99%
    ArduinoMock::advanceMillis(30);
    channel.update();
    TEST_ASSERT_EQUAL(99, channel.getBrightness());
    
    // Test ramping down limit (5%)
    channel.setBrightness(7);
    // Start hold to flip direction to -1 (down)
    channel.handleAction(ActionType::StartHold, 0);
    channel.handleAction(ActionType::StartHold, 0);
    
    // 7% -> 6%
    ArduinoMock::advanceMillis(30);
    channel.update();
    TEST_ASSERT_EQUAL(6, channel.getBrightness());
    
    // 6% -> 5% (direction flips to 1)
    ArduinoMock::advanceMillis(30);
    channel.update();
    TEST_ASSERT_EQUAL(5, channel.getBrightness());
    
    // 5% -> 6%
    ArduinoMock::advanceMillis(30);
    channel.update();
    TEST_ASSERT_EQUAL(6, channel.getBrightness());
}

void test_dimmable_channel_hold_timeout(void) {
    DimmableChannel channel("TestLight", 12);
    channel.begin();
    channel.setBrightness(50);
    
    channel.handleAction(ActionType::StartHold, 0);
    
    // Normal ramp step down (3 steps)
    for (int i = 0; i < 3; i++) {
        ArduinoMock::advanceMillis(30);
        channel.update();
    }
    TEST_ASSERT_EQUAL(47, channel.getBrightness());
    
    // Exceed timeout (300ms)
    ArduinoMock::advanceMillis(350);
    channel.update();
    
    // Brightness shouldn't change anymore
    ArduinoMock::advanceMillis(30);
    channel.update();
    TEST_ASSERT_EQUAL(47, channel.getBrightness());
}

void test_system_controller_basics(void) {
    SystemController controller;
    controller.begin();
    
    TEST_ASSERT_EQUAL(INPUT, ArduinoMock::getPinMode(1));
    TEST_ASSERT_EQUAL(OUTPUT, ArduinoMock::getPinMode(4));
    
    uint8_t entryMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 0;
    msg.action = (uint8_t)ActionType::Click;
    msg.battery_voltage = 3.0f;
    
    controller.dispatchMessage(entryMac, msg);
    controller.update();
    
    // Fade up to target (80%)
    for (int i = 0; i < 80; i++) {
        ArduinoMock::advanceMillis(5);
        controller.update();
    }
    TEST_ASSERT_EQUAL(204, ArduinoMock::getLEDCWrite(12));
    
    // Water pump
    msg.button_index = 2;
    controller.dispatchMessage(entryMac, msg);
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(4));
    
    // Low battery trigger alert
    msg.button_index = 0;
    msg.battery_voltage = 1.8f;
    controller.dispatchMessage(entryMac, msg);
    
    // Bed panel turn off all lights
    uint8_t bedMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    msg.remote_id = 2;
    msg.button_index = 3;
    msg.action = (uint8_t)ActionType::Click;
    msg.battery_voltage = 2.9f;
    
    controller.dispatchMessage(bedMac, msg);
    controller.update();
    
    for (int i = 0; i < 80; i++) {
        ArduinoMock::advanceMillis(5);
        controller.update();
    }
    TEST_ASSERT_EQUAL(0, ArduinoMock::getLEDCWrite(12));
}

void test_system_controller_battery_adc(void) {
    SystemController controller;
    controller.begin();
    
    ArduinoMock::setAnalogValue(1, 2048);
    float voltage = controller.readMainBatteryVoltage();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 7.29f, voltage);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_digital_channel_toggle);
    RUN_TEST(test_dimmable_channel_fade);
    RUN_TEST(test_dimmable_channel_hold_ramping);
    RUN_TEST(test_channel_base_properties);
    RUN_TEST(test_dimmable_channel_encoder_turn);
    RUN_TEST(test_dimmable_channel_ramping_limits);
    RUN_TEST(test_dimmable_channel_hold_timeout);
    RUN_TEST(test_system_controller_basics);
    RUN_TEST(test_system_controller_battery_adc);
    return UNITY_END();
}
