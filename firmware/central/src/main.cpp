/**
 * @file main.cpp
 * @brief ESP32 Central Controller for Camper Van Remote Switching System
 * 
 * Runs on the ESP32-S3 Central unit in the electrical cabinet.
 * Listens for ESP-NOW messages from remote panels, dispatches commands
 * to the registered digital and dimmable channels, and monitors battery levels.
 */

#include <Arduino.h>
#include "SystemController.h"
#include "WirelessManager.h"
#include "protocol.h"

// Instantiate the global system coordinator and wireless manager
SystemController systemController;
WirelessManager wirelessManager;

// Allow-listed Remote MAC addresses (must match actual remotes)
const uint8_t remoteMacs[][6] = {
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11}, // Remote 1 (Entry Panel)
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x22}  // Remote 2 (Bed Panel)
};
const uint8_t NUM_REMOTES = sizeof(remoteMacs) / sizeof(remoteMacs[0]);
const uint8_t ESP_NOW_LMK[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

// Sequence number trackers for replay protection
uint16_t lastSeqNumbers[NUM_REMOTES] = {0};

// Callback when data is received over ESP-NOW
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
    if (len != sizeof(SwitchMessage)) {
        Serial.printf("Error: Invalid packet size received (%d bytes, expected %d)\n", len, sizeof(SwitchMessage));
        return;
    }

    // 1. MAC Address Allow-list validation
    int remoteIdx = -1;
    for (int i = 0; i < NUM_REMOTES; i++) {
        if (memcmp(recv_info->src_addr, remoteMacs[i], 6) == 0) {
            remoteIdx = i;
            break;
        }
    }

    if (remoteIdx == -1) {
        Serial.println("Rejected packet: sender MAC address not in allow-list.");
        return;
    }

    SwitchMessage msg;
    memcpy(&msg, incomingData, sizeof(msg));

    // 2. Sequence number validation (replay protection)
    if (msg.seq <= lastSeqNumbers[remoteIdx]) {
        if (msg.seq == 1 && lastSeqNumbers[remoteIdx] != 1) {
            Serial.printf("Notice: Remote %d power-cycle detected (seq reset to 1).\n", remoteIdx + 1);
        } else {
            Serial.printf("Rejected replay packet: received seq %d, last seq was %d\n", msg.seq, lastSeqNumbers[remoteIdx]);
            return;
        }
    }
    lastSeqNumbers[remoteIdx] = msg.seq;

    // Delegate message handling to the controller using sender's MAC from recv_info
    systemController.dispatchMessage(recv_info->src_addr, msg);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting VanNOW Central Controller...");

    // Initialize physical outputs and controllers
    systemController.begin();

    Serial.print("Central MAC Address: ");
    Serial.println(WiFi.macAddress());

    // Initialize Wi-Fi and ESP-NOW via WirelessManager
    if (!wirelessManager.begin()) {
        Serial.println("Fatal: Error initializing wireless subsystem. Rebooting...");
        delay(2000);
        ESP.restart();
    }

    // Register allowed remote panels as encrypted peers
    for (int i = 0; i < NUM_REMOTES; i++) {
        if (!wirelessManager.addPeer(remoteMacs[i], ESP_NOW_LMK)) {
            Serial.printf("Error adding Remote %d to peer list.\n", i + 1);
        }
    }

    // Register callback for incoming data
    if (!wirelessManager.registerReceiveCallback(OnDataRecv)) {
        Serial.println("Fatal: Error registering receive callback. Rebooting...");
        delay(2000);
        ESP.restart();
    }

    Serial.println("System initialized. Awaiting wireless ESP-NOW commands.");
}

void loop() {
    // Process channel transitions, dimming ramps, and timeouts
    systemController.update();

    // Heartbeat reporting to confirm operational status
    static uint32_t lastHeartbeat = 0;
    uint32_t now = millis();
    if (now - lastHeartbeat > 30000) {
        lastHeartbeat = now;
        float mainBattery = systemController.readMainBatteryVoltage();
        Serial.printf("[Heartbeat] Central Active | Cabin Battery: %.2f V\n", mainBattery);
    }
    
    delay(5);
}
