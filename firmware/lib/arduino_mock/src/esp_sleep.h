#ifndef ESP_SLEEP_MOCK_H
#define ESP_SLEEP_MOCK_H

#include <stdint.h>

#define ESP_EXT1_WAKEUP_ANY_LOW 0

typedef enum {
    ESP_SLEEP_WAKEUP_EXT1
} esp_sleep_wakeup_cause_t;

inline void esp_sleep_enable_ext1_wakeup(uint64_t mask, int mode) {}
inline void esp_deep_sleep_start() {}
inline esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause() { return ESP_SLEEP_WAKEUP_EXT1; }

#endif // ESP_SLEEP_MOCK_H
