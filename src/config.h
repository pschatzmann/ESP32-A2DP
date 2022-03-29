#pragma once

#define DEPRECATED __attribute__((deprecated))


#if __has_include("esp_arduino_version.h")
#include "esp_arduino_version.h"
#endif

#ifndef AUTOCONNECT_TRY_NUM
#define AUTOCONNECT_TRY_NUM 1000
#endif

// Enable ESP_IDF_4 if we are using a current version of ESP IDF e.g. 4.3
// ESP Arduino 2.0 is using ESP IDF 4.4
#if ESP_IDF_VERSION_MAJOR >= 4 || ESP_ARDUINO_VERSION_MAJOR >= 2
#define ESP_IDF_4
#endif

// Compile ESP32C3
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2)
#error "ESP32C3 or ESP32S2 do not support A2DP"
#endif
