#pragma once

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2020 Phil Schatzmann

#include "A2DPEncoder.h"

#if A2DP_MANAGED_ENCODER_SUPPORTED

/**
 * @brief AAC-LC encode support for the managed A2DP encoder framework: wraps
 * any audio_tools::AudioEncoder that produces AAC-LC - e.g.
 * audio_tools::AACEncoderFDK (codec-fdk-aac) or AACEncoderVO
 * (codec-vo-aacenc) - and advertises the AAC SEP capability. Construct and
 * configure (e.g. bitrate) your encoder of choice, include its codec header
 * yourself, and register an A2DPEncoderAAC wrapping it via
 * BluetoothA2DPSource::add_encoder().
 *
 * The stack expects raw AAC access units (it adds the LATM framing itself),
 * so if the wrapped encoder produces ADTS, the ADTS headers are removed.
 * Output that does not start with an ADTS sync word is treated as one raw
 * access unit per write.
 *
 * NOTE: registering a non-SBC stream endpoint is only supported by the
 * underlying Bluedroid stack on ESP-IDF >= 6.1 (with
 * CONFIG_BT_A2DP_CODEC_AAC_ENABLED=y).
 *
 * @ingroup a2dp
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class A2DPEncoderAAC : public A2DPEncoder {
 public:
  /// bitrate: the (max) bitrate that is advertised to the sink: this should
  /// match the bitrate configured in the wrapped encoder
  A2DPEncoderAAC(audio_tools::AudioEncoder& encoder, uint32_t bitrate = 256000)
      : A2DPEncoder(encoder), bitrate(bitrate) {}

  esp_a2d_mct_t codec_type() override { return ESP_A2D_MCT_M24; }

  const char* mime() override { return "audio/aac"; }

  uint32_t samples_per_frame() override { return 1024; }

  bool begin(const esp_a2d_mcc_t& mcc) override {
    buffer.clear();
    mode = Undefined;
    return A2DPEncoder::begin(mcc);
  }

  void build_capability(esp_a2d_mcc_t& mcc) override {
    mcc.type = ESP_A2D_MCT_M24;
#ifdef ESP_A2D_M24_CIE_OBJ_TYPE_2_AAC_LC
    mcc.cie.m24_info.drc = ESP_A2D_M24_CIE_DRC_NS;
    mcc.cie.m24_info.obj_type =
        ESP_A2D_M24_CIE_OBJ_TYPE_2_AAC_LC | ESP_A2D_M24_CIE_OBJ_TYPE_4_AAC_LC;
    // we only advertise the sample rate of the provided PCM
    mcc.cie.m24_info.samp_freq1 = 0;
    mcc.cie.m24_info.samp_freq2 = 0;
    switch (input_sample_rate) {
      case 8000:
        mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_8K;
        break;
      case 11025:
        mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_11K;
        break;
      case 12000:
        mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_12K;
        break;
      case 16000:
        mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_16K;
        break;
      case 22050:
        mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_22K;
        break;
      case 24000:
        mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_24K;
        break;
      case 32000:
        mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_32K;
        break;
      case 48000:
        mcc.cie.m24_info.samp_freq2 = ESP_A2D_M24_CIE_SF2_48K;
        break;
      default:
        if (input_sample_rate != 44100) {
          ESP_LOGE("A2DPEncoderAAC",
                   "unsupported sample rate %d - advertising 44100",
                   input_sample_rate);
        }
        mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_44K;
        break;
    }
    mcc.cie.m24_info.ch = ESP_A2D_M24_CIE_CH_1 | ESP_A2D_M24_CIE_CH_2;
    mcc.cie.m24_info.vbr = ESP_A2D_M24_CIE_VBR_NS;
    mcc.cie.m24_info.br1 = (bitrate >> 16) & ESP_A2D_M24_CIE_BR1_MSK;
    mcc.cie.m24_info.br2 = (bitrate >> 8) & ESP_A2D_M24_CIE_BR2_MSK;
    mcc.cie.m24_info.br3 = bitrate & ESP_A2D_M24_CIE_BR3_MSK;
#else
    // ESP-IDF < 6.1: the named bit constants aren't defined yet, and AAC
    // stream endpoints are not supported anyway
    ESP_LOGW("A2DPEncoderAAC", "AAC requires ESP-IDF >= 6.1");
#endif
  }

 protected:
  enum Mode { Undefined, ADTS, Raw };
  static const size_t kAdtsMinHeaderSize = 7;
  uint32_t bitrate;
  Mode mode = Undefined;
  std::vector<uint8_t> buffer;

  /// determines sample_rate/channels from the negotiated AAC configuration
  /// (a single bit is set per field once negotiation completes)
  void parse_audio_info(const esp_a2d_mcc_t& mcc) override {
#ifdef ESP_A2D_M24_CIE_OBJ_TYPE_2_AAC_LC
    const auto& m24 = mcc.cie.m24_info;
    if (m24.samp_freq2 & ESP_A2D_M24_CIE_SF2_48K) {
      sample_rate_cfg = 48000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_44K) {
      sample_rate_cfg = 44100;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_32K) {
      sample_rate_cfg = 32000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_24K) {
      sample_rate_cfg = 24000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_22K) {
      sample_rate_cfg = 22050;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_16K) {
      sample_rate_cfg = 16000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_12K) {
      sample_rate_cfg = 12000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_11K) {
      sample_rate_cfg = 11025;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_8K) {
      sample_rate_cfg = 8000;
    }
    channels_cfg = (m24.ch & ESP_A2D_M24_CIE_CH_2) ? 2 : 1;
#endif
  }

  static bool is_adts_sync(const uint8_t* data) {
    return data[0] == 0xFF && (data[1] & 0xF6) == 0xF0;
  }

  /// removes the ADTS headers and forwards the raw access units
  void write_encoded(const uint8_t* data, size_t len) override {
    if (len == 0) return;
    if (mode == Undefined) {
      mode = (len >= 2 && is_adts_sync(data)) ? ADTS : Raw;
    }
    if (mode == Raw) {
      A2DPEncoder::write_encoded(data, len);
      return;
    }

    buffer.insert(buffer.end(), data, data + len);
    size_t pos = 0;
    while (buffer.size() - pos >= kAdtsMinHeaderSize) {
      const uint8_t* hdr = buffer.data() + pos;
      if (!is_adts_sync(hdr)) {
        // resync
        pos++;
        continue;
      }
      size_t header_size = (hdr[1] & 0x01) ? 7 : 9;  // protection_absent
      size_t frame_len = ((hdr[3] & 0x03) << 11) | (hdr[4] << 3) |
                         ((hdr[5] >> 5) & 0x07);
      if (frame_len <= header_size) {
        pos++;
        continue;
      }
      if (buffer.size() - pos < frame_len) break;  // incomplete frame
      A2DPEncoder::write_encoded(hdr + header_size, frame_len - header_size);
      pos += frame_len;
    }
    buffer.erase(buffer.begin(), buffer.begin() + pos);
  }
};

#endif  // A2DP_MANAGED_ENCODER_SUPPORTED
