#include "WirelessManager.h"

WirelessManager::WirelessManager() {}

bool WirelessManager::begin() {
    // 1. Initialize Wi-Fi in Station mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // 2. Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return false;
    }
    return true;
}

bool WirelessManager::registerReceiveCallback(esp_now_recv_cb_t callback) {
    esp_err_t err = esp_now_register_recv_cb(callback);
    return (err == ESP_OK);
}

bool WirelessManager::registerSendCallback(esp_now_send_cb_t callback) {
    esp_err_t err = esp_now_register_send_cb(callback);
    return (err == ESP_OK);
}

bool WirelessManager::addPeer(const uint8_t* peerMac) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peerInfo);
    return (err == ESP_OK);
}

bool WirelessManager::sendPayload(const uint8_t* peerMac, const uint8_t* data, size_t len) {
    esp_err_t err = esp_now_send(peerMac, data, len);
    return (err == ESP_OK);
}
