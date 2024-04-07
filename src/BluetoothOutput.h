#pragma once
#include "BluetoothA2DPCommon.h"

#ifdef ARDUINO
#include "Print.h"
#endif

#if A2DP_LEGACY_I2S_SUPPORT
#include "driver/i2s.h"
#endif

#if A2DP_I2S_AUDIOTOOLS
#include "AudioTools.h"
#endif
/**
 * @brief Abstract Output Class
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class BluetoothOutput {
 public:
  virtual bool begin() = 0;
  virtual size_t write(const uint8_t *data, size_t len) = 0;
  virtual void end() = 0;
  virtual void set_sample_rate(int rate);
  virtual void set_output_active(bool active);

#if A2DP_I2S_AUDIOTOOLS
  void set_output(AudioOutput &output) {}
  /// Output AudioStream using AudioTools library
  void set_output(AudioStream &output) {}
#endif

#ifdef ARDUINO
  /// Output to Arduino Print
  void set_output(Print &output) {}
#endif

#if A2DP_LEGACY_I2S_SUPPORT
  /// Define the pins (Legacy I2S: OBSOLETE!)
  virtual void set_pin_config(i2s_pin_config_t pin_config) {}
  /// Define an alternative i2s port other then 0 (Legacy I2S: OBSOLETE!)
  virtual void set_i2s_port(i2s_port_t i2s_num) {}

  /// Define the i2s configuration (Legacy I2S: OBSOLETE!)
  virtual void set_i2s_config(i2s_config_t i2s_config) {}

  /// Defines the bits per sample for output (if > 16 output will be expanded)
  /// (Legacy I2S: OBSOLETE!)
  virtual void set_bits_per_sample(int bps) {}

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 1)
  /// Obsolete: define the master clock pin via the pins
  virtual esp_err_t i2s_mclk_pin_select(const uint8_t pin) { return ESP_FAIL; };
#endif

#endif
};

/***
 * @brief Audio Output using the Arduino Audio Tools library.
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class BluetoothOutputAudioTools : public BluetoothOutput {
 public:
  BluetoothOutputAudioTools() = default;
  bool begin() override;
  size_t write(const uint8_t *data, size_t len) override;
  void end() override;
  void set_sample_rate(int rate) override;
  void set_output_active(bool active) override;

  operator bool() { return p_print != nullptr; }

#if A2DP_I2S_AUDIOTOOLS

  /// Output AudioStream using AudioTools library
  void set_output(AudioOutput &output) {
    p_print = &output;
    p_audio_print = &output;
  }

  /// Output AudioStream using AudioTools library
  void set_output(AudioStream &output) {
    static AdapterAudioStreamToAudioOutput adapter(output);
    p_print = &output;
    p_audio_print = &adapter;
  }
#endif

#ifdef ARDUINO
  /// Output to Arduino Print
  void set_output(Print &output) { p_print = &output; }
#endif

 protected:
#if defined(ARDUINO) || defined(A2DP_I2S_AUDIOTOOLS)
  Print *p_print = nullptr;
#endif

#if A2DP_I2S_AUDIOTOOLS
  AudioOutput *p_audio_print = nullptr;
#endif
};

/***
 * @brief We use the legicy I2S API to output audio
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class BluetoothOutputLegacy : public BluetoothOutput {
 public:
  BluetoothOutputLegacy();
  bool begin() override;
  size_t write(const uint8_t *data, size_t len) override;
  void end() override;
  void set_sample_rate(int rate) override;
  void set_output_active(bool active) override;

#if A2DP_LEGACY_I2S_SUPPORT
  /// Define the pins (Legacy I2S: OBSOLETE!)
  virtual void set_pin_config(i2s_pin_config_t pin_config) {
    this->pin_config = pin_config;
  }
  /// Define an alternative i2s port other then 0 (Legacy I2S: OBSOLETE!)
  virtual void set_i2s_port(i2s_port_t i2s_num) { i2s_port = i2s_num; }

  /// Define the i2s configuration (Legacy I2S: OBSOLETE!)
  virtual void set_i2s_config(i2s_config_t i2s_config) {
    this->i2s_config = i2s_config;
  }

  /// Defines the bits per sample for output (if > 16 output will be expanded)
  /// (Legacy I2S: OBSOLETE!)
  virtual void set_bits_per_sample(int bps) {
    i2s_config.bits_per_sample = (i2s_bits_per_sample_t)bps;
  }

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 1)
  virtual esp_err_t i2s_mclk_pin_select(const uint8_t pin);
#endif

 protected:
  i2s_config_t i2s_config;
  i2s_pin_config_t pin_config;
  i2s_channel_t i2s_channels = I2S_CHANNEL_STEREO;
  i2s_port_t i2s_port = I2S_NUM_0;
#endif
};

/***
 * @brief Audio Output: Default implementation.
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class BluetoothOutputDefault : public BluetoothOutput {
 public:
  BluetoothOutputDefault() = default;
  bool begin() {
    bool rc = true;
    if (out_tools)
      rc = out_tools.begin();
    else
      rc = out_legacy.begin();
    return rc;
  }
  size_t write(const uint8_t *data, size_t len) {
    size_t result = 0;
    if (out_tools)
      result = out_tools.write(data, len);
    else
      result = out_legacy.write(data, len);
    return result;
  }
  void end() override {
    if (out_tools)
      out_tools.end();
    else
      out_legacy.end();
  }
  void set_sample_rate(int rate) override {
    if (out_tools)
      out_tools.set_sample_rate(rate);
    else
      out_legacy.set_sample_rate(rate);
  }
  void set_output_active(bool active) override {
    if (out_tools)
      out_tools.set_output_active(active);
    else
      out_legacy.set_output_active(active);
  }

#if A2DP_I2S_AUDIOTOOLS

  void set_output(AudioOutput &output) { out_tools.set_output(output); }
  /// Output AudioStream using AudioTools library
  void set_output(AudioStream &output) { out_tools.set_output(output); }
#endif

#ifdef ARDUINO
  /// Output to Arduino Print
  void set_output(Print &output) { out_tools.set_output(output); }
#endif

#if A2DP_LEGACY_I2S_SUPPORT
  /// Define the pins (Legacy I2S: OBSOLETE!)
  virtual void set_pin_config(i2s_pin_config_t pin_config) {
    out_legacy.set_pin_config(pin_config);
  }
  /// Define an alternative i2s port other then 0 (Legacy I2S: OBSOLETE!)
  virtual void set_i2s_port(i2s_port_t i2s_num) {
    out_legacy.set_i2s_port(i2s_num);
  }

  /// Define the i2s configuration (Legacy I2S: OBSOLETE!)
  virtual void set_i2s_config(i2s_config_t i2s_config) {
    out_legacy.set_i2s_config(i2s_config);
  }

  /// Defines the bits per sample for output (if > 16 output will be expanded)
  /// (Legacy I2S: OBSOLETE!)
  virtual void set_bits_per_sample(int bps) {
    out_legacy.set_bits_per_sample(bps);
  }

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 1, 1)
  virtual esp_err_t i2s_mclk_pin_select(const uint8_t pin) {
    return out_legacy.i2s_mclk_pin_select(pin);
  }
#endif
#endif  // A2DP_LEGACY_I2S_SUPPORT
 protected:
  BluetoothOutputAudioTools out_tools;
  BluetoothOutputLegacy out_legacy;
};