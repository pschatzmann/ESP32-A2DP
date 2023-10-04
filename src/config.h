#pragma once

#define A2DP_DEPRECATED __attribute__((deprecated))



#include "esp_idf_version.h"

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

// Activate I2S Support (legacy i2s)
#ifndef A2DP_I2S_SUPPORT
#  define A2DP_I2S_SUPPORT true
#endif