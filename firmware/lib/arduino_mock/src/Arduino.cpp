#include "Arduino.h"
#include <stdio.h>
#include <stdarg.h>

// Static virtual register states
static uint32_t mockMillis = 0;
static uint8_t mockPinModes[128] = {0};
static uint8_t mockPinStates[128] = {0};
static int mockAnalogValues[128] = {0};
static uint32_t mockLEDCValues[128] = {0};

MockSerial Serial;

void pinMode(uint8_t pin, uint8_t mode) {
    if (pin < 128) {
        mockPinModes[pin] = mode;
    }
}

void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin < 128) {
        mockPinStates[pin] = val;
    }
}

int digitalRead(uint8_t pin) {
    if (pin < 128) {
        return mockPinStates[pin];
    }
    return LOW;
}

int analogRead(uint8_t pin) {
    if (pin < 128) {
        return mockAnalogValues[pin];
    }
    return 0;
}

void ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution) {}
void ledcAttachPin(uint8_t pin, uint8_t channel) {}
void ledcWrite(uint8_t pin, uint32_t duty) {
    if (pin < 128) {
        mockLEDCValues[pin] = duty;
    }
}
void ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution) {}

uint32_t millis(void) {
    return mockMillis;
}

void delay(uint32_t ms) {
    mockMillis += ms;
}

namespace ArduinoMock {
    void reset() {
        mockMillis = 0;
        memset(mockPinModes, 0, sizeof(mockPinModes));
        // Default pins to HIGH (pull-up states)
        memset(mockPinStates, HIGH, sizeof(mockPinStates));
        memset(mockAnalogValues, 0, sizeof(mockAnalogValues));
        memset(mockLEDCValues, 0, sizeof(mockLEDCValues));
    }
    
    void setMillis(uint32_t ms) {
        mockMillis = ms;
    }
    
    void advanceMillis(uint32_t ms) {
        mockMillis += ms;
    }
    
    void setPinState(uint8_t pin, uint8_t state) {
        if (pin < 128) mockPinStates[pin] = state;
    }
    
    uint8_t getPinState(uint8_t pin) {
        if (pin < 128) return mockPinStates[pin];
        return LOW;
    }
    
    void setPinMode(uint8_t pin, uint8_t mode) {
        if (pin < 128) mockPinModes[pin] = mode;
    }
    
    uint8_t getPinMode(uint8_t pin) {
        if (pin < 128) return mockPinModes[pin];
        return 0;
    }
    
    void setAnalogValue(uint8_t pin, int value) {
        if (pin < 128) mockAnalogValues[pin] = value;
    }
    
    uint32_t getLEDCWrite(uint8_t pin) {
        if (pin < 128) return mockLEDCValues[pin];
        return 0;
    }
}

// Implement mock serial methods
void MockSerial::print(const char* s) {
    printf("%s", s);
}
void MockSerial::print(int n) {
    printf("%d", n);
}
void MockSerial::print(float f) {
    printf("%.2f", f);
}
void MockSerial::println(const char* s) {
    printf("%s\n", s);
}
void MockSerial::println(int n) {
    printf("%d\n", n);
}
void MockSerial::println(float f) {
    printf("%.2f\n", f);
}
void MockSerial::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
