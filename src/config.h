#pragma once

#include "esp_idf_version.h"

// Only the original ESP32 has the classic Bluetooth (BR/EDR) controller
// that A2DP relies on - ESP32-S2/S3/C3/... only support BLE.
#include "sdkconfig.h"
#if defined(CONFIG_IDF_TARGET_ESP32)
#  define IS_VALID_PLATFORM true
#elif defined(CONFIG_IDF_TARGET_ESP32S31)
#  define IS_VALID_PLATFORM true
#else
#  define IS_VALID_PLATFORM false
#endif

#ifndef AUTOCONNECT_TRY_NUM
#  define AUTOCONNECT_TRY_NUM 1000
#endif

// Activate I2S Support (legacy i2s)
#ifndef A2DP_LEGACY_I2S_SUPPORT
#  define A2DP_LEGACY_I2S_SUPPORT (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0))
#endif

// Use https://pschatzmann.github.io/arduino-audio-tools for output
#ifndef A2DP_I2S_AUDIOTOOLS
#  if __has_include("AudioTools.h")
#    define A2DP_I2S_AUDIOTOOLS 1
#  endif
#endif

// Activate SPP Support
#ifndef A2DP_SPP_SUPPORT
#  define A2DP_SPP_SUPPORT 1
#endif

// Managed multi-codec decode framework (A2DPDecoder/A2DPAudioDecoder):
// registers a stream endpoint per decoder added via
// BluetoothA2DPSink::add_decoder() and decodes whichever codec the source
// negotiates using audio_tools decoders (e.g. A2DPDecoderSBC, A2DPDecoderAAC),
// not any ESP-IDF codec component. Requires ESP-IDF >= 5.5 for the
// esp_a2d_sink_register_stream_endpoint()/register_audio_data_callback() APIs,
// and the AudioTools library. Registering a non-SBC (e.g. AAC) stream
// endpoint additionally requires ESP-IDF >= 6.1 at the Bluedroid stack level
// (see A2DPDecoderAAC) - that is a runtime concern (reported via
// ESP_A2D_SEP_REG_STATE_EVT), not a compile-time one.
#ifndef A2DP_MANAGED_DECODER_SUPPORTED
#  define A2DP_MANAGED_DECODER_SUPPORTED (IS_VALID_PLATFORM && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0) && A2DP_I2S_AUDIOTOOLS)
#endif

// Maximum write size
#ifndef A2DP_I2S_MAX_WRITE_SIZE 
#  define A2DP_I2S_MAX_WRITE_SIZE 1024 * 5
#endif

#ifndef A2DP_I2S_MAX_WRITE_DELAY_MS 
#  define A2DP_I2S_MAX_WRITE_DELAY_MS 0
#endif

// Maximum wait time for status change in 100 ms when calling end()
#ifndef A2DP_DISCONNECT_LIMIT 
#  define A2DP_DISCONNECT_LIMIT 20
#endif

#ifndef A2DP_MANAGED_DECODE_Q_DEPTH
#  define A2DP_MANAGED_DECODE_Q_DEPTH 24
#endif

#ifndef A2DP_MANAGED_DECODE_TASK_STACK
#  define A2DP_MANAGED_DECODE_TASK_STACK 4096
#endif

#ifndef A2DP_MANAGED_DECODE_TASK_PRIO
#  define A2DP_MANAGED_DECODE_TASK_PRIO (tskIDLE_PRIORITY + 5)
#endif
