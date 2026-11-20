#pragma once

#include "esp_idf_version.h"

// A2DP relies on the classic Bluetooth (BR/EDR) controller and the Bluedroid
// host stack. The original ESP32 and ESP32-S31 are the targets listed here;
// ESP32-S2/S3/C3/... only support BLE. Whether classic Bluetooth is actually
// available depends on the SDK build, so it is verified via sdkconfig below.
#include "sdkconfig.h"
#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S31)
#  define A2DP_TARGET_HAS_BT 1
#else
#  define A2DP_TARGET_HAS_BT 0
#endif

// The A2DP implementation needs the classic Bluetooth controller (BR/EDR),
// the Bluedroid host stack and the A2DP profile. These are only available
// if they are enabled in the ESP-IDF sdkconfig (menuconfig or
// sdkconfig.defaults). If one is missing we report it here, mark the platform
// as invalid and define A2DP_SDKCONFIG_ERROR, so that no further (misleading)
// missing-header errors are produced.
#if !A2DP_TARGET_HAS_BT
#  define IS_VALID_PLATFORM false
#elif !defined(CONFIG_BT_CLASSIC_ENABLED) || !CONFIG_BT_CLASSIC_ENABLED
#  define IS_VALID_PLATFORM false
#  define A2DP_SDKCONFIG_ERROR 1
#  error "ESP32-A2DP: CONFIG_BT_CLASSIC_ENABLED is not set. Enable 'Bluetooth -> Classic Bluetooth' in the ESP-IDF sdkconfig (menuconfig or sdkconfig.defaults). Note: this target may not provide classic Bluetooth at all."
#elif !defined(CONFIG_BT_BLUEDROID_ENABLED) || !CONFIG_BT_BLUEDROID_ENABLED
#  define IS_VALID_PLATFORM false
#  define A2DP_SDKCONFIG_ERROR 1
#  error "ESP32-A2DP: CONFIG_BT_BLUEDROID_ENABLED is not set. Enable 'Bluetooth -> Bluedroid' (host stack) in the ESP-IDF sdkconfig (menuconfig or sdkconfig.defaults)."
#elif !defined(CONFIG_BT_A2DP_ENABLE) || !CONFIG_BT_A2DP_ENABLE
#  define IS_VALID_PLATFORM false
#  define A2DP_SDKCONFIG_ERROR 1
#  error "ESP32-A2DP: CONFIG_BT_A2DP_ENABLE is not set. Enable the A2DP profile ('Bluetooth -> Bluedroid -> A2DP') in the ESP-IDF sdkconfig (menuconfig or sdkconfig.defaults)."
#else
#  define IS_VALID_PLATFORM true
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

// Managed multi-codec encode framework (A2DPEncoder/A2DPAudioEncoder) for
// BluetoothA2DPSource: registers a stream endpoint per encoder added via
// BluetoothA2DPSource::add_encoder() and encodes the PCM provided by the data
// callbacks with the codec negotiated with the sink, using audio_tools
// encoders (e.g. A2DPEncoderSBC, A2DPEncoderAAC). Same compile-time
// requirements as the decoder framework; at runtime it additionally requires
// CONFIG_BT_A2DP_USE_EXTERNAL_CODEC=y. If no encoder is added, the source
// uses the legacy (internal ESP-IDF SBC encoder) data path unchanged.
#ifndef A2DP_MANAGED_ENCODER_SUPPORTED
#  define A2DP_MANAGED_ENCODER_SUPPORTED (IS_VALID_PLATFORM && ESP_IDF_VERSION >=ESP_IDF_VERSION_VAL(5, 5, 0) && A2DP_I2S_AUDIOTOOLS)
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

#ifndef A2DP_MANAGED_ENCODE_TASK_STACK
#  define A2DP_MANAGED_ENCODE_TASK_STACK 8192
#endif

#ifndef A2DP_MANAGED_ENCODE_TASK_PRIO
#  define A2DP_MANAGED_ENCODE_TASK_PRIO (tskIDLE_PRIORITY + 5)
#endif

// PCM bytes requested from the data callback per encode loop iteration
#ifndef A2DP_MANAGED_ENCODE_CHUNK_SIZE
#  define A2DP_MANAGED_ENCODE_CHUNK_SIZE 512
#endif

// How far (in ms) the encode task may run ahead of real time: the stack
// only queues a limited number of packets, so the PCM is paced
#ifndef A2DP_MANAGED_ENCODE_LEAD_MS
#  define A2DP_MANAGED_ENCODE_LEAD_MS 100
#endif

// If the encode task falls behind real time by more than this (in ms), the
// pacing is reset instead of trying to catch up with a burst
#ifndef A2DP_MANAGED_ENCODE_MAX_LAG_MS
#  define A2DP_MANAGED_ENCODE_MAX_LAG_MS 200
#endif

// Maximum time (in ms) to retry sending a packet while the stack's tx queue
// is full, before the packet is dropped
#ifndef A2DP_MANAGED_ENCODE_SEND_RETRY_MS
#  define A2DP_MANAGED_ENCODE_SEND_RETRY_MS 100
#endif
