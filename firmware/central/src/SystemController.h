#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include "Channel.h"
#include "DigitalChannel.h"
#include "DimmableChannel.h"
#include "PulseChannel.h"
#include "protocol.h"

#include "AntiReplayFilter.h"
#include <Preferences.h>

// Channel operational archetype
enum class ChannelType : uint8_t {
    Dimmable,
    Digital,
    MomentaryPulse
};

// Parameterized configuration entry for modular channel definition
struct ChannelConfig {
    const char* name;
    uint8_t pin;
    ChannelType type;
    uint32_t autoOffTimeoutMs; // 0 = disabled. For MomentaryPulse: pulse active duration in ms.
    bool restoreOnBoot;        // Whether state should restore from NVS across reboots
    uint8_t defaultBrightness; // For dimmable channels (0 - 100)
    bool activeLow;           // Invert pin output polarity (true = active LOW)
};

// Special multi-channel or global macro actions
enum class SpecialRemoteAction : int16_t {
    None = 0,
    TurnOffAllLights = -1
};

// Decoupled remote button mapping definition
struct RemoteMapping {
    uint8_t remoteId;
    uint8_t buttonIndex;
    int16_t targetChannelIndex;        // Channel array index (0..MAX_CHANNELS-1), or < 0 if purely special
    SpecialRemoteAction specialAction; // Special macro action if any
};

class SystemController {
public:
    static constexpr uint8_t MAX_CHANNELS = 32;
    static constexpr uint8_t MAX_REMOTE_MAPPINGS = 64;
    static constexpr uint8_t DEFAULT_CHANNEL_COUNT = 24;
    static constexpr uint8_t NUM_CHANNELS = DEFAULT_CHANNEL_COUNT; // Backwards-compatible alias
    static constexpr uint8_t ENCODER_BUTTON_INDEX = 7;
    static constexpr uint8_t MAX_TRACKED_REMOTES = 8;

    // Default constructor: loads default 24-channel configuration and standard remote mappings
    SystemController();

    // Parameterized constructor: custom channel configurations and optional custom remote mappings
    SystemController(const ChannelConfig* channelConfigs, size_t channelCount,
                     const RemoteMapping* remoteMappings = nullptr, size_t mappingCount = 0);

    ~SystemController();

    // Initialize all registered physical channels, safety timers, and restore persisted states
    void begin();

    // Update all channels (called in loop for transitions, timers, and debounce)
    void update();

    // Dispatch incoming SwitchMessage commands with anti-replay validation and routing
    void dispatchMessage(const uint8_t* senderMac, const SwitchMessage& msg);

    // Read the main 12V battery voltage (analog divisor read on GPIO 1)
    float readMainBatteryVoltage();

    // Replay protection access
    AntiReplayFilter& getAntiReplayFilter();

    // Channel accessors for status inspection and testing
    Channel* getChannel(uint8_t index) const;
    Channel* getChannelByName(const char* name) const;
    int8_t getChannelIndexByName(const char* name) const;
    uint8_t getChannelCount() const;

    // Remote mapping management
    void setRemoteMappings(const RemoteMapping* mappings, size_t count);
    void addRemoteMapping(uint8_t remoteId, uint8_t buttonIndex, int16_t targetChannelIndex,
                          SpecialRemoteAction specialAction = SpecialRemoteAction::None);

    // NVS state persistence operations
    void loadPersistedStates();
    void saveChannelState(uint8_t channelIndex);

    // Water pump timed shower configuration (in milliseconds)
    void setPumpShowerTimeout(uint32_t ms);
    uint32_t getPumpShowerTimeout() const;
    void setPumpChannelIndex(int8_t channelIndex);
    int8_t getPumpChannelIndex() const;

    int8_t getFocusChannel(uint8_t remoteId) const;

    // Authoritative default configurations
    static const ChannelConfig DEFAULT_24CH_CONFIG[DEFAULT_CHANNEL_COUNT];
    static const RemoteMapping DEFAULT_REMOTE_MAPPINGS[20];
    static const size_t DEFAULT_REMOTE_MAPPING_COUNT;

private:
    void initChannels(const ChannelConfig* channelConfigs, size_t channelCount);
    static void onChannelChanged(uint8_t channelIndex, void* context);
    void rememberFocus(uint8_t remoteId, int16_t channelIndex);
    void loadAntiReplayState();
    void saveAntiReplayState(uint8_t remoteId);

    Channel* _channels[MAX_CHANNELS];
    uint8_t _channelCount;

    RemoteMapping _remoteMappings[MAX_REMOTE_MAPPINGS];
    size_t _remoteMappingCount;

    AntiReplayFilter _antiReplay;
    uint32_t _pumpShowerTimeoutMs;
    int8_t _pumpChannelIndex;
    int8_t _focusChannel[MAX_TRACKED_REMOTES + 1];

    // Configuration parameters for main battery reading
    static constexpr uint8_t BATTERY_ADC_PIN = 1;
    static constexpr float DIVIDER_RATIO = (100.0f + 18.0f) / 18.0f; // Divisor resistor values: 100k and 18k
};

#endif // SYSTEM_CONTROLLER_H
