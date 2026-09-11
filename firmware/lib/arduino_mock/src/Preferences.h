#ifndef PREFERENCES_MOCK_H
#define PREFERENCES_MOCK_H

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <map>

class Preferences {
public:
    Preferences();
    ~Preferences();

    bool begin(const char* name, bool readOnly = false);
    void end();
    bool clear();
    bool remove(const char* key);

    size_t putBool(const char* key, bool value);
    bool getBool(const char* key, bool defaultValue = false);

    size_t putUChar(const char* key, uint8_t value);
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0);

    size_t putUInt(const char* key, uint32_t value);
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0);

    size_t putULong64(const char* key, uint64_t value);
    uint64_t getULong64(const char* key, uint64_t defaultValue = 0);

    size_t putInt(const char* key, int32_t value);
    int32_t getInt(const char* key, int32_t defaultValue = 0);

    bool isKey(const char* key);

    // Mock test helper: reset entire mock storage
    static void resetMockStorage();

private:
    std::string _currentNamespace;
    bool _opened;
    bool _readOnly;
};

#endif // PREFERENCES_MOCK_H
