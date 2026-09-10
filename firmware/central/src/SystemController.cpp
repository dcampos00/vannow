#include "SystemController.h"

SystemController::SystemController() {
    // Define the 11 physical channels using ESP32-S3 DevKit pins as per schematic
    // Channels 0 to 3: Dimmable LED Zones
    _channels[0] = new DimmableChannel("Lights Zone 1", 12); // Pin 12
    _channels[1] = new DimmableChannel("Lights Zone 2", 13); // Pin 13
    _channels[2] = new DimmableChannel("Lights Zone 3", 14); // Pin 14
    _channels[3] = new DimmableChannel("Lights Zone 4", 15); // Pin 15

    // Channels 4 to 8: Digital Outputs (Bomba de Agua, Aux 1-3, Ventilador)
    _channels[4] = new DigitalChannel("Water Pump", 4);          // Pin 4
    _channels[5] = new DigitalChannel("Aux Outlet 1", 5);        // Pin 5
    _channels[6] = new DigitalChannel("Aux Outlet 2", 6);        // Pin 6
    _channels[7] = new DigitalChannel("Aux Outlet 3", 7);        // Pin 7
    _channels[8] = new DigitalChannel("Ceiling Fan", 17);        // Pin 17

    // Channels 9 and 10: Optocouplers for Inverter and DC-DC charger
    _channels[9] = new DigitalChannel("Inverter (Multiplus II)", 21); // Pin 21
    _channels[10] = new DigitalChannel("DC-DC Charger", 18);          // Pin 18
}

SystemController::~SystemController() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        delete _channels[i];
    }
}

void SystemController::begin() {
    // 1. Initialize physical channels
    for (int i = 0; i < NUM_CHANNELS; i++) {
        _channels[i]->begin();
        Serial.printf("Configured channel: %s (Pin %d)\n", _channels[i]->getName(), _channels[i]->getPin());
    }

    // 2. Configure safety timers and restore policies
    // Channel 4 is Water Pump: set 10-minute auto-off safety timeout
    static_cast<DigitalChannel*>(_channels[4])->setAutoOffTimeout(10 * 60 * 1000); // 600,000 ms = 10 minutes
    // Water pump must NOT automatically restore ON across reboots/brownouts for flood prevention
    _channels[4]->setRestoreOnBoot(false);

    // 3. Restore persisted channel states from NVS before attaching runtime change callbacks
    loadPersistedStates();

    // 4. Register state change callbacks for persistence
    for (int i = 0; i < NUM_CHANNELS; i++) {
        _channels[i]->setChangeCallback(onChannelChanged, i, this);
    }

    // 5. Configure battery ADC attenuation for 11dB (0-3.3V range)
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
}

void SystemController::update() {
    // Polymorphically update all channels (transitions, safety timeouts, debounce)
    for (int i = 0; i < NUM_CHANNELS; i++) {
        _channels[i]->update();
    }
}

void SystemController::onChannelChanged(uint8_t channelIndex, void* context) {
    SystemController* self = static_cast<SystemController*>(context);
    if (self) {
        self->saveChannelState(channelIndex);
    }
}

void SystemController::saveChannelState(uint8_t channelIndex) {
    if (channelIndex >= NUM_CHANNELS) return;

    Preferences prefs;
    if (!prefs.begin("vannow_state", false)) {
        Serial.println("[NVS] Error opening NVS namespace 'vannow_state' for writing.");
        return;
    }

    char keyState[16];
    snprintf(keyState, sizeof(keyState), "ch%u_state", channelIndex);
    bool currState = _channels[channelIndex]->getState();

    // Prevent redundant flash writes
    if (!prefs.isKey(keyState) || prefs.getBool(keyState, !currState) != currState) {
        prefs.putBool(keyState, currState);
    }

    if (_channels[channelIndex]->isDimmable()) {
        DimmableChannel* dim = static_cast<DimmableChannel*>(_channels[channelIndex]);
        char keyBri[16];
        snprintf(keyBri, sizeof(keyBri), "ch%u_bri", channelIndex);
        uint8_t currBri = dim->getLastOnBrightness();
        if (!prefs.isKey(keyBri) || prefs.getUChar(keyBri, 0) != currBri) {
            prefs.putUChar(keyBri, currBri);
        }
    }

    prefs.end();
}

void SystemController::loadPersistedStates() {
    Preferences prefs;
    if (!prefs.begin("vannow_state", true)) {
        Serial.println("[NVS] No previous persisted state found or unable to read NVS.");
        return;
    }

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char keyState[16];
        snprintf(keyState, sizeof(keyState), "ch%u_state", i);

        bool hasState = prefs.isKey(keyState);
        bool savedState = hasState ? prefs.getBool(keyState, false) : false;

        if (_channels[i]->isDimmable()) {
            DimmableChannel* dim = static_cast<DimmableChannel*>(_channels[i]);
            char keyBri[16];
            snprintf(keyBri, sizeof(keyBri), "ch%u_bri", i);
            uint8_t savedBri = prefs.getUChar(keyBri, 80);
            if (savedBri > 0) {
                dim->setLastOnBrightness(savedBri);
            }
            if (savedState && _channels[i]->shouldRestoreOnBoot()) {
                dim->setBrightness(savedBri);
                Serial.printf("[NVS] Restored dimmable channel %d (%s) ON at %u%%\n",
                              i, _channels[i]->getName(), savedBri);
            }
        } else {
            if (savedState && _channels[i]->shouldRestoreOnBoot()) {
                _channels[i]->setState(true);
                Serial.printf("[NVS] Restored digital channel %d (%s) ON\n",
                              i, _channels[i]->getName());
            }
        }
    }

    prefs.end();
}

AntiReplayFilter& SystemController::getAntiReplayFilter() {
    return _antiReplay;
}

Channel* SystemController::getChannel(uint8_t index) const {
    if (index < NUM_CHANNELS) {
        return _channels[index];
    }
    return nullptr;
}

void SystemController::dispatchMessage(const uint8_t* senderMac, const SwitchMessage& msg) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             senderMac[0], senderMac[1], senderMac[2], senderMac[3], senderMac[4], senderMac[5]);

    Serial.printf("\n--- Message from [%s] ---\n", macStr);
    Serial.printf("Remote ID: %d | Button: %d | Action: %d | Seq: %u\n",
                  msg.remote_id, msg.button_index, msg.action, msg.seq);
    Serial.printf("Remote Battery: %.2f V\n", msg.battery_voltage);

    // RFC 6479 Anti-Replay validation
    if (!_antiReplay.validateAndAdvance(msg.remote_id, msg.seq)) {
        Serial.printf("[SECURITY] Rejected replay packet from Remote %d (seq: %u, last_max: %u)\n",
                      msg.remote_id, msg.seq, _antiReplay.getMaxSeq(msg.remote_id));
        return;
    }

    if (msg.battery_voltage < 2.2f && (msg.battery_voltage > 0.5f || msg.battery_voltage == 0.0f)) {
        Serial.printf("[ALERT] Critical battery on Panel %d. Replace AA batteries.\n", msg.remote_id);
    }

    int targetChannelIdx = -1;

    // Handle mappings according to remote_id
    if (msg.remote_id == 1) { // Entry Panel
        if (msg.button_index == 0) targetChannelIdx = 0; // Lights Zone 1
        if (msg.button_index == 1) targetChannelIdx = 1; // Lights Zone 2
        if (msg.button_index == 2) targetChannelIdx = 4; // Water Pump
        if (msg.button_index == 3) targetChannelIdx = 9; // Inverter
        if (msg.button_index == 4) targetChannelIdx = 2; // Lights Zone 3
        if (msg.button_index == 5) targetChannelIdx = 3; // Lights Zone 4
        if (msg.button_index == 6) targetChannelIdx = 8; // Ceiling Fan
    } 
    else if (msg.remote_id == 2) { // Bed Panel
        if (msg.button_index == 0) targetChannelIdx = 2; // Lights Zone 3
        if (msg.button_index == 1) targetChannelIdx = 3; // Lights Zone 4
        if (msg.button_index == 2) targetChannelIdx = 8; // Ceiling Fan
        if (msg.button_index == 3) {
            // Action Type: toggle off all dimmable lights (Channels 0, 1, 2, 3)
            Serial.println("Bed Panel Action: Turn off all lights");
            for (int i = 0; i < 4; i++) {
                _channels[i]->setState(false);
            }
            return;
        }
        if (msg.button_index == 4) targetChannelIdx = 0; // Lights Zone 1
        if (msg.button_index == 5) targetChannelIdx = 1; // Lights Zone 2
        if (msg.button_index == 6) targetChannelIdx = 4; // Water Pump
    }

    // Forward action to target channel
    if (targetChannelIdx >= 0 && targetChannelIdx < NUM_CHANNELS) {
        _channels[targetChannelIdx]->handleAction((ActionType)msg.action, msg.rotation_steps);
        Serial.printf("Channel [%s] state updated.\n", _channels[targetChannelIdx]->getName());
    } else {
        Serial.println("Warning: Received action mapped to an invalid channel index.");
    }
}

float SystemController::readMainBatteryVoltage() {
    uint32_t mv = analogReadMilliVolts(BATTERY_ADC_PIN);
    float adcVoltage = mv / 1000.0f;
    return adcVoltage * DIVIDER_RATIO;
}
