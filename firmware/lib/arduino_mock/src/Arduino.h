#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Core constants
#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

// Type stubs
typedef bool boolean;
typedef uint8_t byte;

// GPIO stubs
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);

// LEDC stubs (mocked)
void ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution);
void ledcAttachPin(uint8_t pin, uint8_t channel);
void ledcWrite(uint8_t pin, uint32_t duty);
void ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution);

// Time stubs
uint32_t millis(void);
void delay(uint32_t ms);

// Mock controller state access for unit tests
namespace ArduinoMock {
    void reset();
    void setMillis(uint32_t ms);
    void advanceMillis(uint32_t ms);
    
    // Virtual GPIO state
    void setPinState(uint8_t pin, uint8_t state);
    uint8_t getPinState(uint8_t pin);
    void setPinMode(uint8_t pin, uint8_t mode);
    uint8_t getPinMode(uint8_t pin);
    
    void setAnalogValue(uint8_t pin, int value);
    
    // Virtual LEDC state
    uint32_t getLEDCWrite(uint8_t pin);
}

// Mock Serial class
class MockSerial {
public:
    void begin(unsigned long baud) {}
    void print(const char* s);
    void print(int n);
    void print(float f);
    void println(const char* s);
    void println(int n);
    void println(float f);
    void printf(const char* format, ...);
};

extern MockSerial Serial;

#endif // ARDUINO_MOCK_H
