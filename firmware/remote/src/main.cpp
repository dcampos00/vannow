/**
 * @file main.cpp
 * @brief Seeed Studio XIAO ESP32-C6 Remote Switch Panel (Battery Powered)
 * 
 * Configured for Seeed Studio XIAO ESP32-C6.
 * The MCU stays in Deep Sleep. Pressing a button wakes the MCU,
 * reads the battery voltage, sends an ESP-NOW command to the central,
 * and enters Deep Sleep again.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_sleep.h>

// ==========================================
// CONFIGURACIÓN DE IDENTIDAD Y DESTINO
// ==========================================
#define REMOTE_ID 1 

// DIRECCIÓN MAC DEL ESP32 CENTRAL
// Reemplazar con la MAC del Central que se muestra en su monitor serial.
uint8_t centralMacAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// ==========================================
// CONFIGURACIÓN DE PINES (XIAO ESP32-C6)
// ==========================================
#define NUM_BUTTONS 4
// Pines D0, D1, D2 y D3 en la placa (corresponden a GPIO 0, 1, 2 y 3)
const int buttonPins[NUM_BUTTONS] = {0, 1, 2, 3};

// Pin de lectura de batería (D4 / A4 en el XIAO, corresponde a GPIO 4)
#define BATTERY_ADC_PIN 4 

// ==========================================
// ESTRUCTURA DEL PAYLOAD ESP-NOW
// ==========================================
struct __attribute__((packed)) SwitchMessage {
    uint8_t remote_id;      
    uint8_t button_index;   
    uint8_t action;         
    float battery_voltage;  
};

// Variables globales de sincronización
volatile bool messageSent = false;
volatile bool deliverySuccess = false;

// ==========================================
// FUNCIONES AUXILIARES
// ==========================================

// Lee el voltaje de las baterías AA (divisor de voltaje externo 1:1 en pin D4)
float readBatteryVoltage() {
    int raw = analogRead(BATTERY_ADC_PIN);
    // En el ESP32-C6 el ADC tiene por defecto 12 bits de resolución (0-4095)
    // El voltaje de referencia interno es de 1.1V o 3.3V atenuado (por defecto Arduino usa atenuación a 3.3V)
    float adc_voltage = (raw / 4095.0) * 3.3; 
    float battery_voltage = adc_voltage * 2.0; // Multiplicamos por 2 debido al divisor 1:1 (100k + 100k)
    
    if (battery_voltage < 0.5) return 3.0; // Si no hay lectura real, retorna valor típico de 2x AA
    return battery_voltage;
}

// Callback de estado de transmisión ESP-NOW
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    deliverySuccess = (status == ESP_NOW_SEND_SUCCESS);
    messageSent = true;
    Serial.printf("Transmisión: %s\n", deliverySuccess ? "EXITOSA" : "FALLIDA");
}

void enterDeepSleep() {
    Serial.println("Entrando en Deep Sleep...");
    
    // Configurar el bitmask de los pines de despertar (GPIOs 0, 1, 2, 3)
    uint64_t pinMask = 0;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMask |= (1ULL << buttonPins[i]);
    }
    
    // Configurar EXT1 para despertar cuando CUALQUIER pin del mask se ponga en LOW (presión del switch a GND)
    esp_sleep_enable_ext1_wakeup(pinMask, ESP_EXT1_WAKEUP_ANY_LOW);
    
    delay(10); // Estabilizar serial antes de dormir
    esp_deep_sleep_start();
}

// ==========================================
// SETUP (Lógica de despertado)
// ==========================================
void setup() {
    Serial.begin(115200);
    
    // Identificar causa de despertado
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    // Configurar pines de botones
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }
    
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT1) {
        Serial.println("Inicio frío detectado (reinicio o inserción de baterías). Configurando y durmiendo...");
        enterDeepSleep();
    }

    // Identificar qué botón causó el despertar (lectura en estado LOW)
    int pressedButton = -1;
    delay(15); // Debounce físico inicial
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (digitalRead(buttonPins[i]) == LOW) {
            pressedButton = i;
            break;
        }
    }

    if (pressedButton == -1) {
        Serial.println("Wakeup por ruido de señal (falso despertar). Reingresando a sleep.");
        enterDeepSleep();
    }

    Serial.printf("Despertado por Botón: D%d (GPIO %d)\n", pressedButton, buttonPins[pressedButton]);

    // Inicializar Wi-Fi rápido en modo STA
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW.");
        enterDeepSleep();
    }

    // Registrar callback
    esp_now_register_send_cb(OnDataSent);

    // Agregar el central
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, centralMacAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error configurando peer central.");
        enterDeepSleep();
    }

    // Payload de envío
    SwitchMessage msg;
    msg.remote_id = REMOTE_ID;
    msg.button_index = pressedButton;
    msg.action = 0; // Click
    msg.battery_voltage = readBatteryVoltage();

    Serial.printf("Enviando comando (Batería: %.2fV)...\n", msg.battery_voltage);

    esp_err_t result = esp_now_send(centralMacAddress, (uint8_t *) &msg, sizeof(msg));
    
    if (result != ESP_OK) {
        Serial.println("Error de transmisión inicial.");
        enterDeepSleep();
    }

    // Esperar respuesta o timeout de 200ms
    uint32_t start_wait = millis();
    while (!messageSent && (millis() - start_wait < 200)) {
        delay(1);
    }

    if (!messageSent) {
        Serial.println("Timeout: Sin respuesta del central.");
    }

    // Dormir de nuevo
    enterDeepSleep();
}

void loop() {
    // Nunca se ejecuta
}
