#include <unity.h>
#include <Arduino.h>
#include <Preferences.h>
#include "DigitalChannel.h"
#include "DimmableChannel.h"
#include "PulseChannel.h"
#include "SystemController.h"

void setUp(void) {
    ArduinoMock::reset();
    Preferences::resetMockStorage();
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
    // Start hold toggles direction to -1, release and start hold again to toggle to +1 (up)
    channel.handleAction(ActionType::StartHold, 0);
    channel.handleAction(ActionType::Release, 0);
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
    // Release and start hold to set direction to -1 (down)
    channel.handleAction(ActionType::Release, 0);
    channel.handleAction(ActionType::StartHold, 0);
    channel.handleAction(ActionType::Release, 0);
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
    
    TEST_ASSERT_EQUAL(OUTPUT, ArduinoMock::getPinMode(4));
    
    uint8_t entryMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 0;
    msg.action = (uint8_t)ActionType::Click;
    msg.battery_voltage = 3.0f;
    msg.seq = 1; // Valid monotonic sequence
    
    controller.dispatchMessage(entryMac, msg);
    controller.update();
    
    // Fade up to target (80%)
    for (int i = 0; i < 80; i++) {
        ArduinoMock::advanceMillis(5);
        controller.update();
    }
    TEST_ASSERT_EQUAL(204, ArduinoMock::getLEDCWrite(12));
    
    // Water pump (entrance BTN5)
    msg.button_index = 4;
    msg.seq = 2;
    controller.dispatchMessage(entryMac, msg);
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(4));
    
    // Low battery trigger alert
    msg.button_index = 0;
    msg.seq = 3;
    msg.battery_voltage = 1.8f;
    controller.dispatchMessage(entryMac, msg);
    
    // Bed panel turn off all lights
    uint8_t bedMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    msg.remote_id = 2;
    msg.button_index = 3;
    msg.action = (uint8_t)ActionType::Click;
    msg.battery_voltage = 2.9f;
    msg.seq = 1; // Remote 2 first sequence
    
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
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 10.816f, voltage);
}

void test_anti_replay_sliding_window(void) {
    AntiReplayFilter filter;

    // Reject sequence 0
    TEST_ASSERT_FALSE(filter.validateAndAdvance(1, 0));

    // First sequence accepted
    TEST_ASSERT_TRUE(filter.validateAndAdvance(1, 10));
    TEST_ASSERT_EQUAL(10, filter.getMaxSeq(1));

    // In-order forward sequence accepted
    TEST_ASSERT_TRUE(filter.validateAndAdvance(1, 11));
    TEST_ASSERT_EQUAL(11, filter.getMaxSeq(1));

    // Out-of-order within window accepted (seq 9 never seen)
    TEST_ASSERT_TRUE(filter.validateAndAdvance(1, 9));

    // Duplicate replay rejected (seq 11 already seen)
    TEST_ASSERT_FALSE(filter.validateAndAdvance(1, 11));

    // Duplicate replay rejected (seq 9 already seen)
    TEST_ASSERT_FALSE(filter.validateAndAdvance(1, 9));

    // Advance window significantly (jump to 100)
    TEST_ASSERT_TRUE(filter.validateAndAdvance(1, 100));
    TEST_ASSERT_EQUAL(100, filter.getMaxSeq(1));

    // Packet at 35 is 65 steps behind max (100 - 35 = 65 >= 64): outside window, rejected
    TEST_ASSERT_FALSE(filter.validateAndAdvance(1, 35));

    // Packet at 37 is 63 steps behind max (100 - 37 = 63 < 64): inside window, accepted
    TEST_ASSERT_TRUE(filter.validateAndAdvance(1, 37));

    // Duplicate at 37 rejected
    TEST_ASSERT_FALSE(filter.validateAndAdvance(1, 37));

    // Multi-remote isolation: Remote 2 state should be completely separate
    TEST_ASSERT_FALSE(filter.isInitialized(2));
    TEST_ASSERT_TRUE(filter.validateAndAdvance(2, 5));
    TEST_ASSERT_EQUAL(5, filter.getMaxSeq(2));

    // Reset single remote
    filter.resetRemote(1);
    TEST_ASSERT_FALSE(filter.isInitialized(1));
    TEST_ASSERT_TRUE(filter.isInitialized(2)); // Remote 2 remains intact
}

void test_water_pump_auto_off_timer(void) {
    DigitalChannel pump("WaterPump", 4);
    pump.begin();
    pump.setAutoOffTimeout(600000); // 10 minutes = 600,000 ms

    TEST_ASSERT_FALSE(pump.getState());
    TEST_ASSERT_EQUAL(0, pump.getRemainingTime());

    // Turn ON pump
    pump.handleAction(ActionType::Click, 0);
    TEST_ASSERT_TRUE(pump.getState());
    TEST_ASSERT_EQUAL(600000, pump.getRemainingTime());

    // Advance 5 minutes (300,000 ms)
    ArduinoMock::advanceMillis(300000);
    pump.update();
    TEST_ASSERT_TRUE(pump.getState());
    TEST_ASSERT_EQUAL(300000, pump.getRemainingTime());

    // Advance 4 minutes and 59 seconds (299,000 ms) -> Total 599,000 ms
    ArduinoMock::advanceMillis(299000);
    pump.update();
    TEST_ASSERT_TRUE(pump.getState());
    TEST_ASSERT_EQUAL(1000, pump.getRemainingTime());

    // Advance 2 seconds (2,000 ms) -> Total 601,000 ms (timeout expired)
    ArduinoMock::advanceMillis(2000);
    pump.update();
    TEST_ASSERT_FALSE(pump.getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(4));
    TEST_ASSERT_EQUAL(0, pump.getRemainingTime());
}

void test_nvs_state_persistence_and_restore(void) {
    uint8_t mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    // Session 1: configure states and let settle
    {
        SystemController controller;
        controller.begin();

        // 1. Turn ON Zone 1 (channel 0) and dim to 65% via encoder
        SwitchMessage msg = {};
        msg.remote_id = 1;
        msg.button_index = 0; // Zone 1
        msg.action = (uint8_t)ActionType::Click;
        msg.seq = 1;
        controller.dispatchMessage(mac, msg);

        // Turn ON Aux 1 (channel 5)
        Channel* aux1 = controller.getChannel(5);
        TEST_ASSERT_NOT_NULL(aux1);
        aux1->setState(true);

        // Turn ON Water Pump (channel 4)
        Channel* pump = controller.getChannel(4);
        TEST_ASSERT_NOT_NULL(pump);
        pump->setState(true);

        // Adjust Zone 1 brightness to 65% via encoder
        DimmableChannel* zone1 = static_cast<DimmableChannel*>(controller.getChannel(0));
        zone1->setBrightness(65);
        zone1->handleAction(ActionType::Release, 0); // triggers immediate save

        // Allow settle timer to flush
        ArduinoMock::advanceMillis(1500);
        controller.update();

        TEST_ASSERT_TRUE(zone1->getState());
        TEST_ASSERT_EQUAL(65, zone1->getLastOnBrightness());
        TEST_ASSERT_TRUE(aux1->getState());
        TEST_ASSERT_TRUE(pump->getState());
    }

    // Session 2: "Reboot" - create a fresh SystemController instance
    {
        SystemController freshController;
        freshController.begin(); // loads persisted state from NVS

        DimmableChannel* zone1 = static_cast<DimmableChannel*>(freshController.getChannel(0));
        Channel* aux1 = freshController.getChannel(5);
        Channel* pump = freshController.getChannel(4);

        // Zone 1 should restore ON at 65% brightness
        TEST_ASSERT_TRUE(zone1->getState());
        TEST_ASSERT_EQUAL(65, zone1->getBrightness());
        TEST_ASSERT_EQUAL(65, zone1->getLastOnBrightness());

        // Aux 1 should restore ON
        TEST_ASSERT_TRUE(aux1->getState());

        // CRITICAL SAFETY CHECK: Water Pump must NOT restore ON after reboot!
        TEST_ASSERT_FALSE(pump->getState());
    }
}

void test_water_pump_shower_timer_with_chirp(void) {
    DigitalChannel pump("Water Pump", 4);
    pump.begin();

    TEST_ASSERT_FALSE(pump.getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(4));

    // Activate 5-minute shower mode with chirp (300,000 ms) at t = 0
    pump.activateTimer(300000, true);
    TEST_ASSERT_TRUE(pump.getState());
    TEST_ASSERT_TRUE(pump.isTimedActive());
    // Pulse 1 ON immediately
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(4));

    // Advance 155 ms -> Pulse 1 completes, transitions to Pulse 1 OFF
    ArduinoMock::advanceMillis(155);
    pump.update();
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(4));

    // Advance 125 ms -> Pulse 1 pause completes, transitions to Pulse 2 ON
    ArduinoMock::advanceMillis(125);
    pump.update();
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(4));

    // Advance 155 ms -> Pulse 2 completes, transitions to Pulse 2 OFF
    ArduinoMock::advanceMillis(155);
    pump.update();
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(4));

    // Advance 125 ms -> Chirp finished, enters steady Running mode (Pin HIGH)
    ArduinoMock::advanceMillis(125);
    pump.update();
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(4));
    TEST_ASSERT_TRUE(pump.isTimedActive());

    // Total elapsed: 560 ms. Remaining time should be ~ 299,440 ms
    TEST_ASSERT_UINT32_WITHIN(100, 299440, pump.getRemainingTime());

    // Advance 200 seconds (200,000 ms) -> still running
    ArduinoMock::advanceMillis(200000);
    pump.update();
    TEST_ASSERT_TRUE(pump.getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(4));

    // Advance to 300,001 ms -> shower timer expires!
    ArduinoMock::advanceMillis(100000);
    pump.update();
    TEST_ASSERT_FALSE(pump.getState());
    TEST_ASSERT_FALSE(pump.isTimedActive());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(4));
    TEST_ASSERT_EQUAL(0, pump.getRemainingTime());
}

void test_water_pump_shower_mode_dispatch_and_cancel(void) {
    SystemController controller;
    controller.begin();
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11};

    DigitalChannel* pump = static_cast<DigitalChannel*>(controller.getChannel(4));
    TEST_ASSERT_NOT_NULL(pump);
    TEST_ASSERT_FALSE(pump->getState());

    // 1. Entry panel (remote_id 1, button 4) sends DoubleClick -> starts Shower Mode
    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 4; // Water Pump
    msg.action = (uint8_t)ActionType::DoubleClick;
    msg.seq = 100;
    controller.dispatchMessage(mac, msg);

    TEST_ASSERT_TRUE(pump->getState());
    TEST_ASSERT_TRUE(pump->isTimedActive());

    // Let chirp finish (advance 1000ms in small steps simulating loop updates)
    for (int i = 0; i < 10; i++) {
        ArduinoMock::advanceMillis(100);
        controller.update();
    }
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(4));

    // 2. User finishes shower early after 2 minutes and presses pump button (Click)
    ArduinoMock::advanceMillis(120000);
    controller.update();
    TEST_ASSERT_TRUE(pump->getState());

    SwitchMessage cancelMsg = {};
    cancelMsg.remote_id = 1;
    cancelMsg.button_index = 4;
    cancelMsg.action = (uint8_t)ActionType::Click;
    cancelMsg.seq = 101;
    controller.dispatchMessage(mac, cancelMsg);

    // Pump must turn OFF immediately and cancel timer
    TEST_ASSERT_FALSE(pump->getState());
    TEST_ASSERT_FALSE(pump->isTimedActive());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(4));
}

void test_shower_mode_nvs_configuration(void) {
    // Session 1: Custom shower timer configuration (e.g. 3 minutes = 180,000 ms)
    {
        SystemController controller;
        controller.begin();
        TEST_ASSERT_EQUAL_UINT32(300000, controller.getPumpShowerTimeout());

        controller.setPumpShowerTimeout(180000); // 3 minutes
        TEST_ASSERT_EQUAL_UINT32(180000, controller.getPumpShowerTimeout());
    }

    // Session 2: "Reboot" -> verify custom timeout was restored from NVS
    {
        SystemController freshController;
        freshController.begin();
        TEST_ASSERT_EQUAL_UINT32(180000, freshController.getPumpShowerTimeout());
    }
}

void test_pulse_channel(void) {
    PulseChannel pulse("MaxxairPulse", 47, 250); // 250 ms momentary pulse
    pulse.begin();

    // Initial state must be LOW/inactive
    TEST_ASSERT_FALSE(pulse.getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(47));
    TEST_ASSERT_EQUAL(OUTPUT, ArduinoMock::getPinMode(47));
    TEST_ASSERT_FALSE(pulse.shouldRestoreOnBoot());

    // Trigger pulse via Click action
    pulse.handleAction(ActionType::Click, 0);
    TEST_ASSERT_TRUE(pulse.getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(47));

    // Advance 200 ms: still active
    ArduinoMock::advanceMillis(200);
    pulse.update();
    TEST_ASSERT_TRUE(pulse.getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(47));

    // Advance 60 ms (total 260 ms > 250 ms): pulse expires and deactivates
    ArduinoMock::advanceMillis(60);
    pulse.update();
    TEST_ASSERT_FALSE(pulse.getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(47));

    // Trigger via trigger() method
    pulse.trigger();
    TEST_ASSERT_TRUE(pulse.getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(47));

    ArduinoMock::advanceMillis(260);
    pulse.update();
    TEST_ASSERT_FALSE(pulse.getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(47));
}

void test_modular_system_controller_custom_config(void) {
    // Define a custom 5-channel system: 2 dimmable, 2 digital, 1 pulse
    const ChannelConfig customConfigs[5] = {
        {"Reading Light", 12, ChannelType::Dimmable, 0, true, 75, false},
        {"Cabin Light",   13, ChannelType::Dimmable, 0, true, 80, false},
        {"Water Pump",     4, ChannelType::Digital, 300000, false, 0, false}, // 5 min auto-off
        {"Fridge",         5, ChannelType::Digital, 0, true, 0, false},
        {"Horn Pulse",    16, ChannelType::MomentaryPulse, 400, false, 0, false}
    };

    const RemoteMapping customMappings[3] = {
        {1, 0, 0, SpecialRemoteAction::None}, // Remote 1, Button 0 -> Reading Light
        {1, 1, 2, SpecialRemoteAction::None}, // Remote 1, Button 1 -> Water Pump
        {1, 2, 4, SpecialRemoteAction::None}  // Remote 1, Button 2 -> Horn Pulse
    };

    SystemController controller(customConfigs, 5, customMappings, 3);
    controller.begin();

    // Verify channel count
    TEST_ASSERT_EQUAL_UINT8(5, controller.getChannelCount());
    TEST_ASSERT_NOT_NULL(controller.getChannel(0));
    TEST_ASSERT_NOT_NULL(controller.getChannel(4));
    TEST_ASSERT_NULL(controller.getChannel(5)); // Out of bounds

    // Channel names and types
    TEST_ASSERT_EQUAL_STRING("Reading Light", controller.getChannel(0)->getName());
    TEST_ASSERT_TRUE(controller.getChannel(0)->isDimmable());
    TEST_ASSERT_EQUAL_STRING("Horn Pulse", controller.getChannel(4)->getName());
    TEST_ASSERT_FALSE(controller.getChannel(4)->isDimmable());

    // Channel lookup by name
    TEST_ASSERT_EQUAL(2, controller.getChannelIndexByName("Water Pump"));
    TEST_ASSERT_EQUAL(-1, controller.getChannelIndexByName("NonExistent"));

    // Water pump auto-detected index
    TEST_ASSERT_EQUAL(2, controller.getPumpChannelIndex());

    // Dispatch message to custom mapped pulse channel
    uint8_t mac[6] = {0xAA, 0x11, 0x22, 0x33, 0x44, 0x55};
    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 2; // Mapped to Horn Pulse (ch 4, pin 16)
    msg.action = (uint8_t)ActionType::Click;
    msg.seq = 1;

    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_TRUE(controller.getChannel(4)->getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(16));

    // Advance 410 ms -> pulse completes
    ArduinoMock::advanceMillis(410);
    controller.update();
    TEST_ASSERT_FALSE(controller.getChannel(4)->getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(16));
}

void test_11_channel_legacy_profile(void) {
    SystemController controller(SystemController::DEFAULT_11CH_CONFIG,
                                SystemController::DEFAULT_11CH_COUNT);
    controller.begin();

    TEST_ASSERT_EQUAL_UINT8(11, controller.getChannelCount());
    TEST_ASSERT_EQUAL(4, controller.getChannel(4)->getPin());
    TEST_ASSERT_EQUAL_STRING("Water Pump", controller.getChannel(4)->getName());
    TEST_ASSERT_EQUAL(18, controller.getChannel(10)->getPin());
    TEST_ASSERT_EQUAL_STRING("DC-DC Orion-XS #1", controller.getChannel(10)->getName());
    TEST_ASSERT_NULL(controller.getChannel(14));

    uint8_t mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    SwitchMessage msg = {};
    msg.remote_id = 3;
    msg.button_index = 4;
    msg.action = (uint8_t)ActionType::Click;
    msg.seq = 1;
    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_TRUE(controller.getChannel(9)->getState());
    TEST_ASSERT_EQUAL(21, controller.getChannel(9)->getPin());
}

void test_24_channel_full_matrix(void) {
    SystemController controller; // Default 24 channels
    controller.begin();

    TEST_ASSERT_EQUAL_UINT8(24, controller.getChannelCount());

    // Verify 14 Power Channels (Ch 0..13)
    for (uint8_t i = 0; i < 4; i++) {
        Channel* ch = controller.getChannel(i);
        TEST_ASSERT_NOT_NULL(ch);
        TEST_ASSERT_TRUE(ch->isDimmable());
    }
    for (uint8_t i = 4; i < 14; i++) {
        Channel* ch = controller.getChannel(i);
        TEST_ASSERT_NOT_NULL(ch);
        TEST_ASSERT_FALSE(ch->isDimmable());
    }

    // Verify 10 Signal Channels (Ch 14..23)
    for (uint8_t i = 14; i < 20; i++) {
        Channel* ch = controller.getChannel(i);
        TEST_ASSERT_NOT_NULL(ch);
        TEST_ASSERT_FALSE(ch->isDimmable());
    }
    // Ch 20 is Maxxair Keypad Pulse
    Channel* maxxairPulse = controller.getChannel(20);
    TEST_ASSERT_NOT_NULL(maxxairPulse);
    TEST_ASSERT_EQUAL_STRING("Maxxair Keypad Pulse", maxxairPulse->getName());
    TEST_ASSERT_EQUAL(47, maxxairPulse->getPin());

    // Ch 23 is Aux Signal 3 (Gen Start)
    Channel* genStart = controller.getChannel(23);
    TEST_ASSERT_NOT_NULL(genStart);
    TEST_ASSERT_EQUAL_STRING("Aux Signal 3 (Gen Start)", genStart->getName());
    TEST_ASSERT_EQUAL(16, genStart->getPin());

    // Cockpit Remote 3 SW5 (button 4) mapped to Inverter MultiPlus II (Ch 14, pin 21)
    uint8_t mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    SwitchMessage msg = {};
    msg.remote_id = 3;
    msg.button_index = 4;
    msg.action = (uint8_t)ActionType::Click;
    msg.seq = 1;

    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_TRUE(controller.getChannel(14)->getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(21));

    // Cockpit Remote 3 SW2 (button 1) mapped to Orion-XS #1 (Ch 15, pin 38)
    msg.remote_id = 3;
    msg.button_index = 1;
    msg.seq = 2;

    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_TRUE(controller.getChannel(15)->getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(38));

    // Cockpit Remote 3 SW5 (button 4) toggles Inverter (Ch 14, pin 21) -> should turn OFF
    msg.remote_id = 3;
    msg.button_index = 4;
    msg.seq = 3;

    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_FALSE(controller.getChannel(14)->getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(21));
}

void test_remote_mapping_customization(void) {
    SystemController controller;
    controller.begin();

    // Dynamically rebind Remote 1 Button 6 to Ch 20 (Maxxair Keypad Pulse)
    controller.addRemoteMapping(1, 6, 20, SpecialRemoteAction::None);

    uint8_t mac[6] = {0xBB, 0x22, 0x33, 0x44, 0x55, 0x66};
    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 6;
    msg.action = (uint8_t)ActionType::Click;
    msg.seq = 1;

    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_TRUE(controller.getChannel(20)->getState());
    TEST_ASSERT_EQUAL(HIGH, ArduinoMock::getPinState(47));

    ArduinoMock::advanceMillis(260);
    controller.update();
    TEST_ASSERT_FALSE(controller.getChannel(20)->getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(47));
}

void test_digital_channel_ignores_hold_shower(void) {
    DigitalChannel inverter("Inverter (Multiplus II)", 21);
    inverter.begin();

    inverter.handleAction(ActionType::StartHold, 0);
    TEST_ASSERT_FALSE(inverter.getState());
    TEST_ASSERT_FALSE(inverter.isTimedActive());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(21));

    inverter.handleAction(ActionType::DoubleClick, 0);
    TEST_ASSERT_FALSE(inverter.getState());
    TEST_ASSERT_FALSE(inverter.isTimedActive());
}

void test_pulse_channel_ignores_hold_keepalive(void) {
    PulseChannel pulse("Maxxair Keypad Pulse", 47, 250);
    pulse.begin();

    pulse.handleAction(ActionType::StartHold, 0);
    TEST_ASSERT_FALSE(pulse.getState());
    TEST_ASSERT_EQUAL(LOW, ArduinoMock::getPinState(47));

    pulse.handleAction(ActionType::Click, 0);
    TEST_ASSERT_TRUE(pulse.getState());
}

void test_shower_mode_ignores_hold_keepalives(void) {
    SystemController controller;
    controller.begin();
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11};

    DigitalChannel* pump = static_cast<DigitalChannel*>(controller.getChannel(4));
    TEST_ASSERT_NOT_NULL(pump);

    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 4;
    msg.action = (uint8_t)ActionType::StartHold;
    msg.seq = 10;
    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_TRUE(pump->isTimedActive());

    msg.seq = 11;
    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_TRUE(pump->isTimedActive());
    TEST_ASSERT_TRUE(pump->getState());

    msg.seq = 12;
    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_TRUE(pump->isTimedActive());

    msg.action = (uint8_t)ActionType::Click;
    msg.seq = 13;
    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_FALSE(pump->getState());
    TEST_ASSERT_FALSE(pump->isTimedActive());
}

void test_encoder_focus_zone_and_boost(void) {
    SystemController controller;
    controller.begin();
    uint8_t mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 2; // Zone 3
    msg.action = (uint8_t)ActionType::Click;
    msg.seq = 1;
    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_EQUAL(2, controller.getFocusChannel(1));

    DimmableChannel* zone3 = static_cast<DimmableChannel*>(controller.getChannel(2));
    zone3->setBrightness(40);

    msg.button_index = SystemController::ENCODER_BUTTON_INDEX;
    msg.action = (uint8_t)ActionType::EncoderTurn;
    msg.rotation_steps = 2;
    msg.seq = 2;
    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_EQUAL(50, zone3->getBrightness());

    DimmableChannel* zone1 = static_cast<DimmableChannel*>(controller.getChannel(0));
    TEST_ASSERT_EQUAL(0, zone1->getBrightness());

    msg.action = (uint8_t)ActionType::Click;
    msg.rotation_steps = 0;
    msg.seq = 3;
    controller.dispatchMessage(mac, msg);
    TEST_ASSERT_EQUAL(100, zone3->getBrightness());
}

void test_anti_replay_persists_across_reboot(void) {
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11};
    {
        SystemController controller;
        controller.begin();
        SwitchMessage msg = {};
        msg.remote_id = 1;
        msg.button_index = 0;
        msg.action = (uint8_t)ActionType::Click;
        msg.seq = 50;
        controller.dispatchMessage(mac, msg);
        TEST_ASSERT_TRUE(controller.getChannel(0)->getState());
    }

    SystemController rebooted;
    rebooted.begin();
    TEST_ASSERT_TRUE(rebooted.getChannel(0)->getState());

    SwitchMessage replay = {};
    replay.remote_id = 1;
    replay.button_index = 0;
    replay.action = (uint8_t)ActionType::Click;
    replay.seq = 50;
    rebooted.dispatchMessage(mac, replay);
    TEST_ASSERT_TRUE(rebooted.getChannel(0)->getState());

    replay.seq = 51;
    rebooted.dispatchMessage(mac, replay);
    TEST_ASSERT_FALSE(rebooted.getChannel(0)->getState());
}

void test_entrance_night_button_turns_off_lights(void) {
    SystemController controller;
    controller.begin();
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11};

    SwitchMessage msg = {};
    msg.remote_id = 1;
    msg.button_index = 0;
    msg.action = (uint8_t)ActionType::Click;
    msg.seq = 1;
    controller.dispatchMessage(mac, msg);
    for (int i = 0; i < 80; i++) {
        ArduinoMock::advanceMillis(5);
        controller.update();
    }
    TEST_ASSERT_TRUE(controller.getChannel(0)->getState());

    msg.button_index = 5;
    msg.seq = 2;
    controller.dispatchMessage(mac, msg);
    controller.update();
    for (int i = 0; i < 80; i++) {
        ArduinoMock::advanceMillis(5);
        controller.update();
    }
    TEST_ASSERT_EQUAL(0, static_cast<DimmableChannel*>(controller.getChannel(0))->getBrightness());
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
    RUN_TEST(test_anti_replay_sliding_window);
    RUN_TEST(test_water_pump_auto_off_timer);
    RUN_TEST(test_nvs_state_persistence_and_restore);
    RUN_TEST(test_water_pump_shower_timer_with_chirp);
    RUN_TEST(test_water_pump_shower_mode_dispatch_and_cancel);
    RUN_TEST(test_shower_mode_nvs_configuration);
    RUN_TEST(test_pulse_channel);
    RUN_TEST(test_modular_system_controller_custom_config);
    RUN_TEST(test_11_channel_legacy_profile);
    RUN_TEST(test_24_channel_full_matrix);
    RUN_TEST(test_remote_mapping_customization);
    RUN_TEST(test_digital_channel_ignores_hold_shower);
    RUN_TEST(test_pulse_channel_ignores_hold_keepalive);
    RUN_TEST(test_shower_mode_ignores_hold_keepalives);
    RUN_TEST(test_encoder_focus_zone_and_boost);
    RUN_TEST(test_anti_replay_persists_across_reboot);
    RUN_TEST(test_entrance_night_button_turns_off_lights);
    return UNITY_END();
}


