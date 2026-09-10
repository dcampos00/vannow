#include "Preferences.h"
#include <sstream>

static std::map<std::string, std::map<std::string, std::string>> g_mockNvs;

Preferences::Preferences() : _opened(false), _readOnly(false) {}

Preferences::~Preferences() {
    end();
}

bool Preferences::begin(const char* name, bool readOnly) {
    if (!name) return false;
    _currentNamespace = name;
    _readOnly = readOnly;
    _opened = true;
    return true;
}

void Preferences::end() {
    _opened = false;
    _currentNamespace.clear();
}

bool Preferences::clear() {
    if (!_opened || _readOnly) return false;
    g_mockNvs[_currentNamespace].clear();
    return true;
}

bool Preferences::remove(const char* key) {
    if (!_opened || _readOnly || !key) return false;
    return g_mockNvs[_currentNamespace].erase(key) > 0;
}

size_t Preferences::putBool(const char* key, bool value) {
    if (!_opened || _readOnly || !key) return 0;
    g_mockNvs[_currentNamespace][key] = value ? "1" : "0";
    return 1;
}

bool Preferences::getBool(const char* key, bool defaultValue) {
    if (!_opened || !key) return defaultValue;
    auto nsIt = g_mockNvs.find(_currentNamespace);
    if (nsIt == g_mockNvs.end()) return defaultValue;
    auto keyIt = nsIt->second.find(key);
    if (keyIt == nsIt->second.end()) return defaultValue;
    return keyIt->second == "1";
}

size_t Preferences::putUChar(const char* key, uint8_t value) {
    if (!_opened || _readOnly || !key) return 0;
    g_mockNvs[_currentNamespace][key] = std::to_string(value);
    return 1;
}

uint8_t Preferences::getUChar(const char* key, uint8_t defaultValue) {
    if (!_opened || !key) return defaultValue;
    auto nsIt = g_mockNvs.find(_currentNamespace);
    if (nsIt == g_mockNvs.end()) return defaultValue;
    auto keyIt = nsIt->second.find(key);
    if (keyIt == nsIt->second.end()) return defaultValue;
    try {
        return static_cast<uint8_t>(std::stoul(keyIt->second));
    } catch (...) {
        return defaultValue;
    }
}

size_t Preferences::putUInt(const char* key, uint32_t value) {
    if (!_opened || _readOnly || !key) return 0;
    g_mockNvs[_currentNamespace][key] = std::to_string(value);
    return 4;
}

uint32_t Preferences::getUInt(const char* key, uint32_t defaultValue) {
    if (!_opened || !key) return defaultValue;
    auto nsIt = g_mockNvs.find(_currentNamespace);
    if (nsIt == g_mockNvs.end()) return defaultValue;
    auto keyIt = nsIt->second.find(key);
    if (keyIt == nsIt->second.end()) return defaultValue;
    try {
        return static_cast<uint32_t>(std::stoul(keyIt->second));
    } catch (...) {
        return defaultValue;
    }
}

size_t Preferences::putInt(const char* key, int32_t value) {
    if (!_opened || _readOnly || !key) return 0;
    g_mockNvs[_currentNamespace][key] = std::to_string(value);
    return 4;
}

int32_t Preferences::getInt(const char* key, int32_t defaultValue) {
    if (!_opened || !key) return defaultValue;
    auto nsIt = g_mockNvs.find(_currentNamespace);
    if (nsIt == g_mockNvs.end()) return defaultValue;
    auto keyIt = nsIt->second.find(key);
    if (keyIt == nsIt->second.end()) return defaultValue;
    try {
        return static_cast<int32_t>(std::stol(keyIt->second));
    } catch (...) {
        return defaultValue;
    }
}

bool Preferences::isKey(const char* key) {
    if (!_opened || !key) return false;
    auto nsIt = g_mockNvs.find(_currentNamespace);
    if (nsIt == g_mockNvs.end()) return false;
    return nsIt->second.find(key) != nsIt->second.end();
}

void Preferences::resetMockStorage() {
    g_mockNvs.clear();
}
