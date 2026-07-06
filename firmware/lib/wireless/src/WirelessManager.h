#ifndef WIRELESS_MANAGER_H
#define WIRELESS_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "protocol.h"

class WirelessManager {
public:
    WirelessManager();

    // Initialize Wi-Fi in Station mode and initialize ESP-NOW. Returns true on success.
    bool begin();

    // Register receiver callback (Central)
    bool registerReceiveCallback(esp_now_recv_cb_t callback);

    // Register sender callback (Remote)
    bool registerSendCallback(esp_now_send_cb_t callback);

    // Register peer (Remote)
    bool addPeer(const uint8_t* peerMac);

    // Send payload to registered peer
    bool sendPayload(const uint8_t* peerMac, const uint8_t* data, size_t len);
};

#endif // WIRELESS_MANAGER_H
