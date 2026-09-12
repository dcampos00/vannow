#ifndef REMOTE_SENDER_H
#define REMOTE_SENDER_H

#include <Arduino.h>
#include "WirelessManager.h"
#include "protocol.h"

class RemoteSender {
public:
    RemoteSender(uint8_t remoteId, const uint8_t* targetMac, uint8_t batteryPin);

    // Initialize inputs, Wi-Fi, ESP-NOW, and target peer
    bool begin(const uint8_t* customMac = nullptr);

    // Pack and send action packet to the target peer
    bool send(ActionType action, uint8_t buttonIndex, int8_t rotationSteps);

    // Persist monotonic sequence to NVS so a battery swap does not lock out the remote
    void persistSequence();

    // Read battery voltage using external 1:1 divisor
    float readBatteryVoltage();

private:
    uint8_t _remoteId;
    uint8_t _targetMac[6];
    uint8_t _batteryPin;
    WirelessManager _wireless;

    // Static variables required for the C-style ESP-NOW callback
    static volatile bool _messageSent;
    static volatile bool _deliverySuccess;
    static void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);
};

#endif // REMOTE_SENDER_H
