#pragma once

#define A2DP_DEPRECATED __attribute__((deprecated))

// Enable ESP_IDF_4 if we are using a current version of ESP IDF e.g. 4.3
// ESP Arduino 2.0 is using ESP IDF 4.4
#if __has_include("esp_arduino_version.h")
#include "esp_arduino_version.h"
#  if ESP_ARDUINO_VERSION_MAJOR >= 2
#    define ESP_IDF_4
#  endif
#endif

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#  if ESP_IDF_VERSION_MAJOR >= 4
#    ifndef ESP_IDF_4
#      define ESP_IDF_4
#    endif
#  endif
#endif

#ifndef AUTOCONNECT_TRY_NUM
#  define AUTOCONNECT_TRY_NUM 1000
#endif

// If you use #include "I2S.h" the i2s functionality is hidden in a namespace
// this hack prevents any error messages
#ifdef _I2S_H_INCLUDED
using namespace esp_i2s;
#endif

// Compile only for ESP32
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#error "ESP32C3, ESP32S2, ESP32S3 do not support A2DP"
#endif
