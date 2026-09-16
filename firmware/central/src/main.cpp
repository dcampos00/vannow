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

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_task_wdt.h>
#define WDT_TIMEOUT_SECONDS 10

// FreeRTOS queue for thread-safe transfer from wifi_task to loopTask
struct PacketQueueItem {
    uint8_t senderMac[6];
    SwitchMessage msg;
};
static QueueHandle_t packetQueue = nullptr;
#endif

// Instantiate the global system coordinator and wireless manager
SystemController systemController;
WirelessManager wirelessManager;

// Provisioned Central MAC address
const uint8_t centralMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// Allow-listed Remote MAC addresses (must match actual remotes)
const uint8_t remoteMacs[][6] = {
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11}, // Remote 1 (Entry Panel)
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x22}, // Remote 2 (Bed Panel)
    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x33}  // Remote 3 (Cockpit Panel)
};
const uint8_t NUM_REMOTES = sizeof(remoteMacs) / sizeof(remoteMacs[0]);
const uint8_t ESP_NOW_LMK[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

// Callback when data is received over ESP-NOW (executes in wifi_task context)
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

#if defined(ARDUINO_ARCH_ESP32)
    // 2. Post packet to FreeRTOS queue to avoid data race with loopTask
    PacketQueueItem item;
    memcpy(item.senderMac, recv_info->src_addr, 6);
    memcpy(&item.msg, incomingData, sizeof(SwitchMessage));
    if (packetQueue && xQueueSend(packetQueue, &item, 0) != pdTRUE) {
        Serial.println("[WARN] ESP-NOW packet queue full, dropped frame.");
    }
#else
    SwitchMessage msg;
    memcpy(&msg, incomingData, sizeof(msg));
    systemController.dispatchMessage(recv_info->src_addr, msg);
#endif
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting VanNOW Central Controller (alpha, not 1.0)...");

#ifdef RGB_BUILTIN
    // GPIO 38 (v1.1) / GPIO 48 (v1.0) are optocoupler outputs. Do not drive the DevKit WS2812.
    pinMode(RGB_BUILTIN, INPUT);
#endif

    // Initialize physical outputs, safety timers, and restore persisted states
    systemController.begin();

    Serial.print("Central MAC Address: ");
    Serial.println(WiFi.macAddress());

#if defined(ARDUINO_ARCH_ESP32)
    // Create thread-safe packet queue (depth 32 to prevent overflow under NVS write load)
    // FIX [HIGH-02]: Increased from 16 to 32 to handle bursty traffic during Serial/NVS blocking
    packetQueue = xQueueCreate(32, sizeof(PacketQueueItem));

    // Initialize Task Watchdog Timer (10s timeout)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    esp_task_wdt_init(&twdt_config);
#else
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);
#endif
    esp_task_wdt_add(NULL); // Subscribe current loopTask
    Serial.printf("Task Watchdog Timer configured with %d second timeout.\n", WDT_TIMEOUT_SECONDS);
#endif

    // Initialize Wi-Fi and ESP-NOW via WirelessManager with provisioned MAC
    if (!wirelessManager.begin(centralMac)) {
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
#if defined(ARDUINO_ARCH_ESP32)
    // Pet Task Watchdog Timer
    esp_task_wdt_reset();

    // Process queued ESP-NOW packets safely in loopTask context
    PacketQueueItem item;
    while (packetQueue && xQueueReceive(packetQueue, &item, 0) == pdTRUE) {
        systemController.dispatchMessage(item.senderMac, item.msg);
    }
#endif

    // Process channel transitions, dimming ramps, auto-off timers, and debounce
    systemController.update();

    // Heartbeat reporting to confirm operational status
    static uint32_t lastHeartbeat = 0;
    uint32_t now = millis();
    if (now - lastHeartbeat > 30000) {
        lastHeartbeat = now;
        float mainBattery = systemController.readMainBatteryVoltage();
        Serial.printf("[Heartbeat] Central Active | Cabin Battery: %.2f V\n", mainBattery);
    }
    
    // FIX [MEDIUM-02]: Reduced delay from 5ms to 1ms for better responsiveness (loop rate 1 kHz vs 200 Hz)
    // Maintains sufficient headroom for 200 Hz PROFET PWM while reducing input latency
    delay(1);
}
