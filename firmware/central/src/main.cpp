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

// Callback when data is received over ESP-NOW
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
    if (len != sizeof(SwitchMessage)) {
        Serial.printf("Error: Invalid packet size received (%d bytes, expected %d)\n", len, sizeof(SwitchMessage));
        return;
    }

    SwitchMessage msg;
    memcpy(&msg, incomingData, sizeof(msg));

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
