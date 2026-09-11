#include "SystemController.h"
#include <string.h>

const ChannelConfig SystemController::DEFAULT_24CH_CONFIG[DEFAULT_CHANNEL_COUNT] = {
    // 14 High-Side Power Channels (12V PROFET BTS5008)
    {"Lights Zone 1",            12, ChannelType::Dimmable,       0,      true,  80, false}, // Ch 0
    {"Lights Zone 2",            13, ChannelType::Dimmable,       0,      true,  80, false}, // Ch 1
    {"Lights Zone 3",            14, ChannelType::Dimmable,       0,      true,  80, false}, // Ch 2
    {"Lights Zone 4",            15, ChannelType::Dimmable,       0,      true,  80, false}, // Ch 3
    {"Water Pump",                4, ChannelType::Digital,   600000,     false,   0, false}, // Ch 4 (10m safety timer, flood prevention: restore=false)
    {"Exterior Driver Light",     5, ChannelType::Digital,        0,      true,   0, false}, // Ch 5
    {"Exterior Passenger Light",  6, ChannelType::Digital,        0,      true,   0, false}, // Ch 6
    {"Aux Lightbar / Roof",       7, ChannelType::Digital,        0,      true,   0, false}, // Ch 7
    {"Maxxair Fan Power",        17, ChannelType::Digital,        0,      true,   0, false}, // Ch 8
    {"Aux Power 1 (Boiler)",      8, ChannelType::Digital,        0,      true,   0, false}, // Ch 9
    {"Aux Power 2 (Grey Valve)",  9, ChannelType::Digital,        0,     false,   0, false}, // Ch 10
    {"Aux Power 3 (Tank Heat)",  10, ChannelType::Digital,        0,     false,   0, false}, // Ch 11
    {"Aux Power 4 (Aux Sockets)",11, ChannelType::Digital,        0,      true,   0, false}, // Ch 12
    {"Aux Power 5 (Awning)",     18, ChannelType::Digital,        0,      true,   0, false}, // Ch 13

    // 10 Isolated Signal Channels (PC817 Optocouplers / Dry Contacts)
    {"Inverter (Multiplus II)",  21, ChannelType::Digital,        0,      true,   0, false}, // Ch 14
    {"DC-DC Orion-XS #1",        38, ChannelType::Digital,        0,      true,   0, false}, // Ch 15
    {"DC-DC Orion-XS #2",        39, ChannelType::Digital,        0,      true,   0, false}, // Ch 16
    {"SmartSolar MPPT",          40, ChannelType::Digital,        0,      true,   0, false}, // Ch 17
    {"Diesel Heater",            41, ChannelType::Digital,        0,      true,   0, false}, // Ch 18
    {"12V Fridge Compressor",    42, ChannelType::Digital,        0,      true,   0, false}, // Ch 19
    {"Maxxair Keypad Pulse",     47, ChannelType::MomentaryPulse, 250,   false,   0, false}, // Ch 20 (250ms momentary pulse)
    {"Aux Signal 1 (Alarm)",     48, ChannelType::Digital,        0,     false,   0, false}, // Ch 21
    {"Aux Signal 2 (LPG Valve)",  2, ChannelType::Digital,        0,     false,   0, false}, // Ch 22
    {"Aux Signal 3 (Gen Start)", 16, ChannelType::MomentaryPulse, 500,   false,   0, false}  // Ch 23 (500ms momentary pulse)
};

const RemoteMapping SystemController::DEFAULT_REMOTE_MAPPINGS[20] = {
    // Remote 1: Entry Panel (7 buttons)
    {1, 0, 0, SpecialRemoteAction::None},  // Button 0 -> Ch 0: Lights Zone 1
    {1, 1, 1, SpecialRemoteAction::None},  // Button 1 -> Ch 1: Lights Zone 2
    {1, 2, 4, SpecialRemoteAction::None},  // Button 2 -> Ch 4: Water Pump
    {1, 3, 14, SpecialRemoteAction::None}, // Button 3 -> Ch 14: Inverter MultiPlus II
    {1, 4, 2, SpecialRemoteAction::None},  // Button 4 -> Ch 2: Lights Zone 3
    {1, 5, 3, SpecialRemoteAction::None},  // Button 5 -> Ch 3: Lights Zone 4
    {1, 6, 8, SpecialRemoteAction::None},  // Button 6 -> Ch 8: Maxxair Fan Power

    // Remote 2: Bed Panel (7 buttons)
    {2, 0, 2, SpecialRemoteAction::None},  // Button 0 -> Ch 2: Lights Zone 3
    {2, 1, 3, SpecialRemoteAction::None},  // Button 1 -> Ch 3: Lights Zone 4
    {2, 2, 8, SpecialRemoteAction::None},  // Button 2 -> Ch 8: Maxxair Fan Power
    {2, 3, -1, SpecialRemoteAction::TurnOffAllLights}, // Button 3 -> Master Lights Off
    {2, 4, 0, SpecialRemoteAction::None},  // Button 4 -> Ch 0: Lights Zone 1
    {2, 5, 1, SpecialRemoteAction::None},  // Button 5 -> Ch 1: Lights Zone 2
    {2, 6, 4, SpecialRemoteAction::None},  // Button 6 -> Ch 4: Water Pump

    // Remote 3: Cockpit Panel (6 Carling rocker switches)
    {3, 0, 5, SpecialRemoteAction::None},  // SW1 -> Ch 5: Exterior Driver Light
    {3, 1, 15, SpecialRemoteAction::None}, // SW2 -> Ch 15: Orion-XS #1 Remote Enable
    {3, 2, 0, SpecialRemoteAction::None},  // SW3 -> Ch 0: Lights Zone 1
    {3, 3, 4, SpecialRemoteAction::None},  // SW4 -> Ch 4: Water Pump
    {3, 4, 14, SpecialRemoteAction::None}, // SW5 -> Ch 14: Inverter MultiPlus II
    {3, 5, 8, SpecialRemoteAction::None}   // SW6 -> Ch 8: Maxxair Fan Power
};

const size_t SystemController::DEFAULT_REMOTE_MAPPING_COUNT = 20;

SystemController::SystemController()
    : SystemController(DEFAULT_24CH_CONFIG, DEFAULT_CHANNEL_COUNT,
                       DEFAULT_REMOTE_MAPPINGS, DEFAULT_REMOTE_MAPPING_COUNT) {}

SystemController::SystemController(const ChannelConfig* channelConfigs, size_t channelCount,
                                   const RemoteMapping* remoteMappings, size_t mappingCount)
    : _channelCount(0),
      _remoteMappingCount(0),
      _pumpShowerTimeoutMs(300000), // Default 5 minutes (300,000 ms)
      _pumpChannelIndex(-1) {
    for (size_t i = 0; i < MAX_CHANNELS; i++) {
        _channels[i] = nullptr;
    }

    // Initialize channels from configuration array
    initChannels(channelConfigs, channelCount);

    // Register remote mappings
    if (remoteMappings && mappingCount > 0) {
        setRemoteMappings(remoteMappings, mappingCount);
    } else if (channelCount >= 11) {
        setRemoteMappings(DEFAULT_REMOTE_MAPPINGS, DEFAULT_REMOTE_MAPPING_COUNT);
    }
}

SystemController::~SystemController() {
    for (uint8_t i = 0; i < _channelCount; i++) {
        if (_channels[i]) {
            delete _channels[i];
            _channels[i] = nullptr;
        }
    }
}

void SystemController::initChannels(const ChannelConfig* channelConfigs, size_t channelCount) {
    if (!channelConfigs) return;

    if (channelCount > MAX_CHANNELS) {
        channelCount = MAX_CHANNELS;
    }

    _channelCount = channelCount;

    for (size_t i = 0; i < channelCount; i++) {
        const ChannelConfig& cfg = channelConfigs[i];
        switch (cfg.type) {
            case ChannelType::Dimmable: {
                DimmableChannel* dim = new DimmableChannel(cfg.name, cfg.pin);
                if (cfg.defaultBrightness > 0) {
                    dim->setLastOnBrightness(cfg.defaultBrightness);
                }
                dim->setRestoreOnBoot(cfg.restoreOnBoot);
                _channels[i] = dim;
                break;
            }
            case ChannelType::MomentaryPulse: {
                uint32_t duration = (cfg.autoOffTimeoutMs > 0) ? cfg.autoOffTimeoutMs : 250;
                PulseChannel* pulse = new PulseChannel(cfg.name, cfg.pin, duration, cfg.activeLow);
                pulse->setRestoreOnBoot(false);
                _channels[i] = pulse;
                break;
            }
            case ChannelType::Digital:
            default: {
                DigitalChannel* dig = new DigitalChannel(cfg.name, cfg.pin, cfg.activeLow);
                if (cfg.autoOffTimeoutMs > 0) {
                    dig->setAutoOffTimeout(cfg.autoOffTimeoutMs);
                }
                dig->setRestoreOnBoot(cfg.restoreOnBoot);
                _channels[i] = dig;
                break;
            }
        }

        // Auto-detect water pump channel for shower mode binding
        if (strstr(cfg.name, "Water Pump") != nullptr || strstr(cfg.name, "Bomba") != nullptr) {
            _pumpChannelIndex = (int8_t)i;
        }
    }

    // Default pump index fallback if channel 4 is defined and not explicitly matched
    if (_pumpChannelIndex == -1 && _channelCount > 4) {
        _pumpChannelIndex = 4;
    }
}

void SystemController::setRemoteMappings(const RemoteMapping* mappings, size_t count) {
    if (!mappings) {
        _remoteMappingCount = 0;
        return;
    }
    if (count > MAX_REMOTE_MAPPINGS) {
        count = MAX_REMOTE_MAPPINGS;
    }
    for (size_t i = 0; i < count; i++) {
        _remoteMappings[i] = mappings[i];
    }
    _remoteMappingCount = count;
}

void SystemController::addRemoteMapping(uint8_t remoteId, uint8_t buttonIndex, int16_t targetChannelIndex,
                                        SpecialRemoteAction specialAction) {
    // Check if entry already exists to update it
    for (size_t i = 0; i < _remoteMappingCount; i++) {
        if (_remoteMappings[i].remoteId == remoteId && _remoteMappings[i].buttonIndex == buttonIndex) {
            _remoteMappings[i].targetChannelIndex = targetChannelIndex;
            _remoteMappings[i].specialAction = specialAction;
            return;
        }
    }
    // Otherwise append new mapping if room available
    if (_remoteMappingCount < MAX_REMOTE_MAPPINGS) {
        _remoteMappings[_remoteMappingCount++] = {remoteId, buttonIndex, targetChannelIndex, specialAction};
    }
}

void SystemController::begin() {
    // 1. Initialize physical channels
    for (uint8_t i = 0; i < _channelCount; i++) {
        if (_channels[i]) {
            _channels[i]->begin();
            Serial.printf("Configured channel [%u]: %s (Pin %d)\n",
                          i, _channels[i]->getName(), _channels[i]->getPin());
        }
    }

    // 2. Restore persisted channel states from NVS before attaching runtime change callbacks
    loadPersistedStates();

    // 3. Register state change callbacks for persistence
    for (uint8_t i = 0; i < _channelCount; i++) {
        if (_channels[i]) {
            _channels[i]->setChangeCallback(onChannelChanged, i, this);
        }
    }

    // 4. Configure battery ADC attenuation for 11dB (0-3.3V range)
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
}

void SystemController::update() {
    // Polymorphically update all active channels (transitions, safety timeouts, debounce)
    for (uint8_t i = 0; i < _channelCount; i++) {
        if (_channels[i]) {
            _channels[i]->update();
        }
    }
}

void SystemController::onChannelChanged(uint8_t channelIndex, void* context) {
    SystemController* self = static_cast<SystemController*>(context);
    if (self) {
        self->saveChannelState(channelIndex);
    }
}

void SystemController::saveChannelState(uint8_t channelIndex) {
    if (channelIndex >= _channelCount || !_channels[channelIndex]) return;

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

    for (uint8_t i = 0; i < _channelCount; i++) {
        if (!_channels[i]) continue;

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

    _pumpShowerTimeoutMs = prefs.getUInt("pump_shower_ms", 300000);

    prefs.end();
}

void SystemController::setPumpShowerTimeout(uint32_t ms) {
    _pumpShowerTimeoutMs = ms;
    Preferences prefs;
    if (prefs.begin("vannow_state", false)) {
        prefs.putUInt("pump_shower_ms", ms);
        prefs.end();
    }
}

uint32_t SystemController::getPumpShowerTimeout() const {
    return _pumpShowerTimeoutMs;
}

void SystemController::setPumpChannelIndex(int8_t channelIndex) {
    _pumpChannelIndex = channelIndex;
}

int8_t SystemController::getPumpChannelIndex() const {
    return _pumpChannelIndex;
}

AntiReplayFilter& SystemController::getAntiReplayFilter() {
    return _antiReplay;
}

Channel* SystemController::getChannel(uint8_t index) const {
    if (index < _channelCount) {
        return _channels[index];
    }
    return nullptr;
}

Channel* SystemController::getChannelByName(const char* name) const {
    if (!name) return nullptr;
    for (uint8_t i = 0; i < _channelCount; i++) {
        if (_channels[i] && strcmp(_channels[i]->getName(), name) == 0) {
            return _channels[i];
        }
    }
    return nullptr;
}

int8_t SystemController::getChannelIndexByName(const char* name) const {
    if (!name) return -1;
    for (uint8_t i = 0; i < _channelCount; i++) {
        if (_channels[i] && strcmp(_channels[i]->getName(), name) == 0) {
            return (int8_t)i;
        }
    }
    return -1;
}

uint8_t SystemController::getChannelCount() const {
    return _channelCount;
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
    SpecialRemoteAction specialAction = SpecialRemoteAction::None;

    // Look up mapping in decoupled routing table
    for (size_t i = 0; i < _remoteMappingCount; i++) {
        if (_remoteMappings[i].remoteId == msg.remote_id &&
            _remoteMappings[i].buttonIndex == msg.button_index) {
            targetChannelIdx = _remoteMappings[i].targetChannelIndex;
            specialAction = _remoteMappings[i].specialAction;
            break;
        }
    }

    // Handle global macro actions
    if (specialAction == SpecialRemoteAction::TurnOffAllLights) {
        Serial.println("Action: Turn off all dimmable lights");
        for (uint8_t i = 0; i < _channelCount; i++) {
            if (_channels[i] && _channels[i]->isDimmable()) {
                _channels[i]->setState(false);
            }
        }
        return;
    }

    // Special handling for Water Pump Shower Mode
    if (targetChannelIdx >= 0 && targetChannelIdx == _pumpChannelIndex &&
        (msg.action == (uint8_t)ActionType::DoubleClick || msg.action == (uint8_t)ActionType::StartHold)) {
        DigitalChannel* pump = static_cast<DigitalChannel*>(_channels[_pumpChannelIndex]);
        if (pump) {
            if (pump->isTimedActive()) {
                Serial.println("[Shower Mode] Pump cancelled early by user.");
                pump->setState(false);
            } else {
                Serial.printf("[Shower Mode] Activated: %u ms with acoustic chirp\n", _pumpShowerTimeoutMs);
                pump->activateTimer(_pumpShowerTimeoutMs, true);
            }
            return;
        }
    }

    // Forward action to target channel
    if (targetChannelIdx >= 0 && targetChannelIdx < _channelCount && _channels[targetChannelIdx]) {
        _channels[targetChannelIdx]->handleAction((ActionType)msg.action, msg.rotation_steps);
        Serial.printf("Channel [%s] (Index %d) state updated.\n",
                      _channels[targetChannelIdx]->getName(), targetChannelIdx);
    } else {
        Serial.println("Warning: Received action mapped to an invalid or unconfigured channel index.");
    }
}

float SystemController::readMainBatteryVoltage() {
    uint32_t mv = analogReadMilliVolts(BATTERY_ADC_PIN);
    float adcVoltage = mv / 1000.0f;
    return adcVoltage * DIVIDER_RATIO;
}
