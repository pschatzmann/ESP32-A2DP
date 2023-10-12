#pragma once

#ifndef AUTOCONNECT_TRY_NUM
#  define AUTOCONNECT_TRY_NUM 1000
#endif

// Activate I2S Support (legacy i2s)
#ifndef A2DP_I2S_SUPPORT
#  define A2DP_I2S_SUPPORT (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0))
#endif

#ifndef A2DP_SPP_SUPPORT
#  define A2DP_SPP_SUPPORT (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0))
#endif