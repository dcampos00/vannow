#include "RemoteSender.h"

// Define static members
volatile bool RemoteSender::_messageSent = false;
volatile bool RemoteSender::_deliverySuccess = false;

RemoteSender::RemoteSender(uint8_t remoteId, const uint8_t* targetMac, uint8_t batteryPin)
    : _remoteId(remoteId), _batteryPin(batteryPin) {
    memcpy(_targetMac, targetMac, 6);
}

void RemoteSender::OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    _deliverySuccess = (status == ESP_NOW_SEND_SUCCESS);
    _messageSent = true;
}

bool RemoteSender::begin() {
    // Initialize battery reading pin
    pinMode(_batteryPin, INPUT);

    // Initialize wireless manager
    if (!_wireless.begin()) {
        Serial.println("Error initializing wireless manager");
        return false;
    }

    // Register callback for data transmission status
    if (!_wireless.registerSendCallback(OnDataSent)) {
        Serial.println("Error registering send callback");
        return false;
    }

    // Add Central as a peer
    if (!_wireless.addPeer(_targetMac)) {
        Serial.println("Error adding Central peer");
        return false;
    }

    return true;
}

float RemoteSender::readBatteryVoltage() {
    int raw = analogRead(_batteryPin);
    // Seeed Studio XIAO ESP32-C6 defaults to 12-bit ADC (0-4095) with ~3.3V reference
    float adcVoltage = (raw / 4095.0f) * 3.3f;
    float batteryVoltage = adcVoltage * 2.0f; // 1:1 divisor (100k + 100k)
    return (batteryVoltage < 0.5f) ? 3.0f : batteryVoltage;
}

bool RemoteSender::send(ActionType action, uint8_t buttonIndex, int8_t rotationSteps) {
    SwitchMessage msg;
    msg.remote_id = _remoteId;
    msg.button_index = buttonIndex;
    msg.action = (uint8_t)action;
    msg.rotation_steps = rotationSteps;
    msg.battery_voltage = readBatteryVoltage();

    Serial.printf("Sending payload: Remote %d | Btn %d | Act %d | Steps %d | Bat %.2fV\n",
                  msg.remote_id, msg.button_index, msg.action, msg.rotation_steps, msg.battery_voltage);

    _messageSent = false;
    if (!_wireless.sendPayload(_targetMac, (uint8_t *)&msg, sizeof(msg))) {
        Serial.println("Error triggering ESP-NOW transmission.");
        return false;
    }

    // Await delivery status confirmation (timeout 200ms)
    uint32_t startWait = millis();
    while (!_messageSent && (millis() - startWait < 200)) {
        delay(1);
    }

    return _deliverySuccess;
}
