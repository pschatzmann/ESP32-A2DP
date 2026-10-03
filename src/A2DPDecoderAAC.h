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

#include <string.h>

#include "A2DPDecoder.h"

#if A2DP_MANAGED_DECODER_SUPPORTED

/**
 * @brief AAC decode support for the managed A2DP decoder framework: wraps
 * any audio_tools::AudioDecoder that decodes AAC - e.g.
 * audio_tools::AACDecoderHelix (arduino-libhelix), AACDecoderFDK
 * (arduino-fdk-aac) or AACDecoderFAAD (arduino-libfaad) - and advertises
 * the AAC SEP capability. Construct your decoder of choice, include its
 * codec header yourself (e.g. "AudioTools/AudioCodecs/CodecAACHelix.h"),
 * and register an A2DPDecoderAAC wrapping it via
 * BluetoothA2DPSink::add_decoder().
 *
 * A2DP delivers raw AAC access units (ISO/IEC 13818-7 / 14496-3
 * raw_data_block()) with no ADTS header, but most general-purpose AAC
 * decoders (including audio_tools::AACDecoderHelix) expect ADTS framing to
 * find their sync word - so this class synthesizes a 7-byte ADTS header
 * (from the sample rate/channel count negotiated via SEP configuration)
 * and prepends it to every frame before handing it to the wrapped decoder.
 *
 * NOTE: registering a non-SBC stream endpoint is only supported by the
 * underlying Bluedroid stack on ESP-IDF >= 6.1 - earlier versions document
 * esp_a2d_sink_register_stream_endpoint() as SBC-only and are expected to
 * reject this registration (see the ESP_A2D_SEP_REG_STATE_EVT log). This
 * class still works on older IDF versions; it just won't get negotiated.
 *
 * @ingroup a2dp
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class A2DPDecoderAAC : public A2DPDecoder {
 public:
  A2DPDecoderAAC(audio_tools::AudioDecoder& decoder) : A2DPDecoder(decoder) {}

  esp_a2d_mct_t codec_type() override { return ESP_A2D_MCT_M24; }

  const char* mime() override { return "audio/aac"; }

  /// prepends a synthesized ADTS header to each frame, since A2DP AAC
  /// frames arrive as raw (headerless) access units but the wrapped
  /// decoder expects ADTS framing to find its sync word
  size_t write(const uint8_t* data, size_t len) override {
    if (len == 0) return 0;
    size_t adts_frame_len = len + kAdtsHeaderSize;
    if (adts_frame_len > kAdtsMaxFrameLen) {
      // 13-bit ADTS frame-length field can't represent this frame; A2DP
      // AAC frames are always far smaller than this in practice
      return 0;
    }
    write_adts_header(adts_frame_len);
    return A2DPDecoder::write(data, len);
  }

  void build_capability(esp_a2d_mcc_t& mcc) override {
    mcc.type = ESP_A2D_MCT_M24;
#ifdef ESP_A2D_M24_CIE_OBJ_TYPE_2_AAC_LC
    // the named MPEG-2/4 AAC CIE bit constants (and real SEP support for
    // this codec type) are only available starting with ESP-IDF >= 6.1
    mcc.cie.m24_info.drc = ESP_A2D_M24_CIE_DRC_NS;
    mcc.cie.m24_info.obj_type = ESP_A2D_M24_CIE_OBJ_TYPE_2_AAC_LC |
                                 ESP_A2D_M24_CIE_OBJ_TYPE_4_AAC_LC |
                                 ESP_A2D_M24_CIE_OBJ_TYPE_4_HE_AAC |
                                 ESP_A2D_M24_CIE_OBJ_TYPE_4_HE_AAC_V2;
    mcc.cie.m24_info.samp_freq1 = ESP_A2D_M24_CIE_SF1_8K | ESP_A2D_M24_CIE_SF1_11K |
                                   ESP_A2D_M24_CIE_SF1_12K | ESP_A2D_M24_CIE_SF1_16K |
                                   ESP_A2D_M24_CIE_SF1_22K | ESP_A2D_M24_CIE_SF1_24K |
                                   ESP_A2D_M24_CIE_SF1_32K | ESP_A2D_M24_CIE_SF1_44K;
    mcc.cie.m24_info.samp_freq2 = ESP_A2D_M24_CIE_SF2_48K | ESP_A2D_M24_CIE_SF2_64K |
                                   ESP_A2D_M24_CIE_SF2_88K | ESP_A2D_M24_CIE_SF2_96K;
    mcc.cie.m24_info.ch = ESP_A2D_M24_CIE_CH_1 | ESP_A2D_M24_CIE_CH_2;
    mcc.cie.m24_info.vbr = ESP_A2D_M24_CIE_VBR_SUPPORT;
    mcc.cie.m24_info.br1 = 0x7F & ESP_A2D_M24_CIE_BR1_MSK;
    mcc.cie.m24_info.br2 = 0xFF & ESP_A2D_M24_CIE_BR2_MSK;
    mcc.cie.m24_info.br3 = 0xFF & ESP_A2D_M24_CIE_BR3_MSK;
#else
    // ESP-IDF < 6.1: the named bit constants aren't defined yet, and
    // esp_a2d_sink_register_stream_endpoint() documents this codec type
    // as unsupported anyway - advertise an empty capability rather than
    // claim capabilities we can't back up
#endif
  }

 protected:
  static const int kAdtsHeaderSize = 7;
  static const int kAdtsMaxFrameLen = 8191;  // 13-bit ADTS frame-length field

  uint8_t freq_idx = 4;  // index of 44100 in the table below

  /// determines sample_rate/channels/freq_idx from the negotiated AAC
  /// capability (a single bit is set per field once negotiation completes) -
  /// needed to synthesize the ADTS header, since AAC's raw frames don't
  /// carry this info themselves
  void parse_audio_info(const esp_a2d_mcc_t& mcc) override {
#ifdef ESP_A2D_M24_CIE_OBJ_TYPE_2_AAC_LC
    const auto& m24 = mcc.cie.m24_info;
    if (m24.samp_freq2 & ESP_A2D_M24_CIE_SF2_96K) {
      sample_rate = 96000;
    } else if (m24.samp_freq2 & ESP_A2D_M24_CIE_SF2_88K) {
      sample_rate = 88200;
    } else if (m24.samp_freq2 & ESP_A2D_M24_CIE_SF2_64K) {
      sample_rate = 64000;
    } else if (m24.samp_freq2 & ESP_A2D_M24_CIE_SF2_48K) {
      sample_rate = 48000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_44K) {
      sample_rate = 44100;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_32K) {
      sample_rate = 32000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_24K) {
      sample_rate = 24000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_22K) {
      sample_rate = 22050;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_16K) {
      sample_rate = 16000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_12K) {
      sample_rate = 12000;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_11K) {
      sample_rate = 11025;
    } else if (m24.samp_freq1 & ESP_A2D_M24_CIE_SF1_8K) {
      sample_rate = 8000;
    } else {
      sample_rate = 44100;
    }
    channels = (m24.ch & ESP_A2D_M24_CIE_CH_1) ? 1 : 2;
#else
    // ESP-IDF < 6.1: the named bit constants aren't defined and this codec
    // type never actually gets negotiated - keep the defaults
#endif
    freq_idx = adts_freq_idx(sample_rate);
  }

  /// maps a sample rate to its ADTS sampling_frequency_index; falls back
  /// to 44100's index if not one of the 13 well-known rates
  static uint8_t adts_freq_idx(int rate) {
    static const int table[] = {96000, 88200, 64000, 48000, 44100, 32000,
                                24000, 22050, 16000, 12000, 11025, 8000, 7350};
    for (uint8_t i = 0; i < 13; i++) {
      if (table[i] == rate) return i;
    }
    return 4;  // 44100
  }

  /// synthesizes a 7-byte ADTS header (no CRC) for an AAC-LC frame of the
  /// given total length (header + payload) at the negotiated sample
  /// rate/channel count.
  /// NOTE: the ADTS profile field is hardcoded to AAC-LC (profile=1) since
  /// it has no room to represent SBR/PS - if the source actually negotiates
  /// HE-AAC/HE-AAC v2 (advertised as negotiable in build_capability()),
  /// whether the wrapped decoder still recovers the SBR/PS extension from
  /// an LC-labeled ADTS stream depends on that decoder's implicit-signaling
  /// support and is untested here.
  void write_adts_header(size_t adts_frame_len) {
    uint8_t out[kAdtsHeaderSize];
    out[0] = 0xFF;
    out[1] = 0xF1;  // MPEG-4, layer 0, no CRC
    out[2] = (1 << 6) | (freq_idx << 2) | ((channels >> 2) & 0x1);  // profile=AAC-LC
    out[3] = ((channels & 0x3) << 6) | ((adts_frame_len >> 11) & 0x3);
    out[4] = (adts_frame_len >> 3) & 0xFF;
    out[5] = ((adts_frame_len & 0x7) << 5) | 0x1F;
    out[6] = 0xFC;
    A2DPDecoder::write(out, sizeof(out));
  }
};

#endif  // A2DP_MANAGED_DECODER_SUPPORTED
