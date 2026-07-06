#include "SystemController.h"

SystemController::SystemController() {
    // Define the 11 physical channels using ESP32-S3 DevKit pins as per specifications
    // Channels 0 to 3: Dimmable LED Zones
    _channels[0] = new DimmableChannel("Lights Zone 1", 12, 0); // Pin 12, LEDC Channel 0
    _channels[1] = new DimmableChannel("Lights Zone 2", 13, 1); // Pin 13, LEDC Channel 1
    _channels[2] = new DimmableChannel("Lights Zone 3", 14, 2); // Pin 14, LEDC Channel 2
    _channels[3] = new DimmableChannel("Lights Zone 4", 27, 3); // Pin 27, LEDC Channel 3

    // Channels 4 to 8: Digital Outputs (Bomba de Agua, Aux 1-3, Ventilador)
    _channels[4] = new DigitalChannel("Water Pump", 25);         // Pin 25
    _channels[5] = new DigitalChannel("Aux Outlet 1", 26);       // Pin 26
    _channels[6] = new DigitalChannel("Aux Outlet 2", 32);       // Pin 32
    _channels[7] = new DigitalChannel("Aux Outlet 3", 33);       // Pin 33
    _channels[8] = new DigitalChannel("Ceiling Fan", 19);        // Pin 19

    // Channels 9 and 10: Optocouplers for Inverter and DC-DC charger
    _channels[9] = new DigitalChannel("Inverter (Multiplus II)", 21); // Pin 21
    _channels[10] = new DigitalChannel("DC-DC Charger", 22);          // Pin 22
}

SystemController::~SystemController() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        delete _channels[i];
    }
}

void SystemController::begin() {
    // Initialize each individual channel
    for (int i = 0; i < NUM_CHANNELS; i++) {
        _channels[i]->begin();
        Serial.printf("Configured channel: %s (Pin %d)\n", _channels[i]->getName(), _channels[i]->getPin());
    }

    // Initialize battery ADC pin
    pinMode(BATTERY_ADC_PIN, INPUT);
}

void SystemController::update() {
    // Call update on all channels to process dimming/fading
    for (int i = 0; i < NUM_CHANNELS; i++) {
        // Only dimmable channels need active loops
        if (_channels[i]->isDimmable()) {
            static_cast<DimmableChannel*>(_channels[i])->update();
        }
    }
}

void SystemController::dispatchMessage(const uint8_t* senderMac, const SwitchMessage& msg) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             senderMac[0], senderMac[1], senderMac[2], senderMac[3], senderMac[4], senderMac[5]);

    Serial.printf("\n--- Message from [%s] ---\n", macStr);
    Serial.printf("Remote ID: %d | Button: %d | Action: %d\n", msg.remote_id, msg.button_index, msg.action);
    Serial.printf("Remote Battery: %.2f V\n", msg.battery_voltage);

    if (msg.battery_voltage < 2.2f && msg.battery_voltage > 0.5f) {
        Serial.printf("[ALERT] Critical battery on Panel %d. Replace AA batteries.\n", msg.remote_id);
    }

    int targetChannelIdx = -1;

    // Handle mappings according to remote_id
    if (msg.remote_id == 1) { // Entry Panel
        if (msg.button_index == 0) targetChannelIdx = 0; // Lights Zone 1
        if (msg.button_index == 1) targetChannelIdx = 1; // Lights Zone 2
        if (msg.button_index == 2) targetChannelIdx = 4; // Water Pump
        if (msg.button_index == 3) targetChannelIdx = 9; // Inverter
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
    int raw = analogRead(BATTERY_ADC_PIN);
    // ESP32-S3 defaults to 12-bit resolution (0-4095)
    float adcVoltage = (raw / 4095.0f) * ADC_VOLTAGE_REF;
    return adcVoltage * DIVIDER_RATIO;
}
