/**
 * @file main.cpp
 * @brief ESP32 Central Controller for Camper Van Remote Relay System
 * 
 * This code runs on the central MCU (Seeed Studio XIAO ESP32-C6 or ESP32-S3)
 * in the electrical cabinet. It listens for ESP-NOW messages from remote
 * battery-powered switch modules and switches the corresponding outputs.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ==========================================
// CONFIGURACIÓN DE PINES (XIAO ESP32-C6 vs ESP32-S3)
// ==========================================
#define NUM_CHANNELS 11

struct Channel {
    const char* name;
    uint8_t pin;
    bool state;
    bool is_dimmable; // Indica si soporta atenuación PWM
};

#if defined(ARDUINO_SEEED_XIAO_ESP32C6)
// Mapeo optimizado utilizando los 11 pines físicos del XIAO ESP32-C6 (D0 - D10)
Channel channels[NUM_CHANNELS] = {
    {"Luces Zona 1", 0, false, true},     // D0 (GPIO 0)
    {"Luces Zona 2", 1, false, true},     // D1 (GPIO 1)
    {"Luces Zona 3", 2, false, true},     // D2 (GPIO 2)
    {"Luces Zona 4", 3, false, true},     // D3 (GPIO 3)
    {"Bomba de Agua", 4, false, false},   // D4 (GPIO 4)
    {"Salida 12V Aux 1", 5, false, false}, // D5 (GPIO 5)
    {"Salida 12V Aux 2", 6, false, false}, // D6 (GPIO 6)
    {"Salida 12V Aux 3", 7, false, false}, // D7 (GPIO 7)
    {"Ventilador de Techo", 19, false, false}, // D8 (GPIO 19)
    {"Inversor (Multiplus II)", 20, false, false}, // D9 (GPIO 20)
    {"Cargador DC-DC", 18, false, false}   // D10 (GPIO 18)
};
#else
// Mapeo por defecto para placas con más pines como ESP32-S3 DevKit
Channel channels[NUM_CHANNELS] = {
    {"Luces Zona 1", 12, false, true},
    {"Luces Zona 2", 13, false, true},
    {"Luces Zona 3", 14, false, true},
    {"Luces Zona 4", 27, false, true},
    {"Bomba de Agua", 25, false, false},
    {"Salida 12V Aux 1", 26, false, false},
    {"Salida 12V Aux 2", 32, false, false},
    {"Salida 12V Aux 3", 33, false, false},
    {"Ventilador de Techo", 19, false, false},
    {"Inversor (Multiplus II)", 21, false, false},
    {"Cargador DC-DC", 22, false, false}
};
#endif

// ==========================================
// ESTRUCTURA DEL PAYLOAD ESP-NOW
// ==========================================
struct __attribute__((packed)) SwitchMessage {
    uint8_t remote_id;      // ID único del control remoto
    uint8_t button_index;   // 0 a 3
    uint8_t action;         // 0 = Click
    float battery_voltage;  // Voltaje de las baterías del remoto
};

// ==========================================
// CALLBACK DE RECEPCIÓN ESP-NOW
// ==========================================
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
    if (len != sizeof(SwitchMessage)) {
        Serial.printf("Error: Paquete de tamaño incorrecto (%d bytes)\n", len);
        return;
    }

    SwitchMessage msg;
    memcpy(&msg, incomingData, sizeof(msg));

    // Obtener MAC del emisor
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
             recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);

    Serial.printf("\n--- Comando Recibido desde [%s] ---\n", macStr);
    Serial.printf("Remoto ID: %d | Botón: %d | Acción: %d\n", msg.remote_id, msg.button_index, msg.action);
    Serial.printf("Batería Remoto: %.2f V\n", msg.battery_voltage);

    if (msg.battery_voltage < 2.2f && msg.battery_voltage > 0.5f) {
        Serial.printf("[ALERTA] Batería crítica en Panel %d. Cambiar pilas.\n", msg.remote_id);
    }

    // Lógica de Asociación
    int target_channel = -1;

    if (msg.remote_id == 1) { // Panel de Entrada
        if (msg.button_index == 0) target_channel = 0; // Luces Zona 1
        if (msg.button_index == 1) target_channel = 1; // Luces Zona 2
        if (msg.button_index == 2) target_channel = 4; // Bomba de Agua
        if (msg.button_index == 3) target_channel = 9; // Inversor
    } 
    else if (msg.remote_id == 2) { // Panel de Cama
        if (msg.button_index == 0) target_channel = 2; // Luces Zona 3
        if (msg.button_index == 1) target_channel = 3; // Luces Zona 4
        if (msg.button_index == 2) target_channel = 8; // Ventilador
        if (msg.button_index == 3) {
            Serial.println("Acción Especial: Apagar todas las luces");
            for (int i = 0; i < 4; i++) {
                channels[i].state = false;
                digitalWrite(channels[i].pin, LOW);
            }
        }
    }

    // Cambiar estado de salida
    if (target_channel >= 0 && target_channel < NUM_CHANNELS) {
        channels[target_channel].state = !channels[target_channel].state;
        digitalWrite(channels[target_channel].pin, channels[target_channel].state ? HIGH : LOW);
        Serial.printf("Canal [%s] -> %s\n", 
                      channels[target_channel].name, 
                      channels[target_channel].state ? "ENCENDIDO" : "APAGADO");
    }
}

// ==========================================
// SETUP & LOOP
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Iniciando ESP32 Central de Relés...");

    // Inicializar pines
    for (int i = 0; i < NUM_CHANNELS; i++) {
        pinMode(channels[i].pin, OUTPUT);
        digitalWrite(channels[i].pin, LOW);
        Serial.printf("Pin %02d configurado para: %s\n", channels[i].pin, channels[i].name);
    }

    // Inicializar Wi-Fi en modo Station
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.print("MAC del Central: ");
    Serial.println(WiFi.macAddress());

    // Inicializar ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW. Reiniciando...");
        delay(2000);
        ESP.restart();
    }

    // Registrar callback de recepción
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Listo para recibir comandos ESP-NOW.");
}

void loop() {
    static uint32_t last_heartbeat = 0;
    if (millis() - last_heartbeat > 30000) {
        last_heartbeat = millis();
        Serial.println("[Heartbeat] Central Operativa.");
    }
    delay(100);
}
