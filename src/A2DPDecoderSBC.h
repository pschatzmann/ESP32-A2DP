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

#include "A2DPDecoder.h"

#if A2DP_MANAGED_DECODER_SUPPORTED

// Fallback defines for older IDF versions that don't declare these
#ifndef ESP_A2D_SBC_CIE_SF_16K
#  define ESP_A2D_SBC_CIE_SF_16K (0x8)
#endif
#ifndef ESP_A2D_SBC_CIE_SF_32K
#  define ESP_A2D_SBC_CIE_SF_32K (0x4)
#endif

/**
 * @brief SBC decode support for the managed A2DP decoder framework: wraps
 * any audio_tools::AudioDecoder that decodes SBC - e.g.
 * audio_tools::SBCDecoder (arduino-libsbc) - and advertises the SBC SEP
 * capability. Construct your decoder of choice, include its codec header
 * yourself (e.g. "AudioTools/AudioCodecs/CodecSBC.h"), and register an
 * A2DPDecoderSBC wrapping it via BluetoothA2DPSink::add_decoder().
 * @ingroup a2dp
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class A2DPDecoderSBC : public A2DPDecoder {
 public:
  A2DPDecoderSBC(audio_tools::AudioDecoder& decoder) : A2DPDecoder(decoder) {}

  esp_a2d_mct_t codec_type() override { return ESP_A2D_MCT_SBC; }

  const char* mime() override { return "audio/sbc"; }

  void build_capability(esp_a2d_mcc_t& mcc) override {
    mcc.type = ESP_A2D_MCT_SBC;
    mcc.cie.sbc_info.samp_freq = ESP_A2D_SBC_CIE_SF_16K | ESP_A2D_SBC_CIE_SF_32K |
                                  ESP_A2D_SBC_CIE_SF_44K | ESP_A2D_SBC_CIE_SF_48K;
    mcc.cie.sbc_info.ch_mode = ESP_A2D_SBC_CIE_CH_MODE_MONO |
                                ESP_A2D_SBC_CIE_CH_MODE_DUAL_CHANNEL |
                                ESP_A2D_SBC_CIE_CH_MODE_STEREO |
                                ESP_A2D_SBC_CIE_CH_MODE_JOINT_STEREO;
    mcc.cie.sbc_info.block_len = ESP_A2D_SBC_CIE_BLOCK_LEN_4 | ESP_A2D_SBC_CIE_BLOCK_LEN_8 |
                                  ESP_A2D_SBC_CIE_BLOCK_LEN_12 | ESP_A2D_SBC_CIE_BLOCK_LEN_16;
    mcc.cie.sbc_info.num_subbands =
        ESP_A2D_SBC_CIE_NUM_SUBBANDS_4 | ESP_A2D_SBC_CIE_NUM_SUBBANDS_8;
    mcc.cie.sbc_info.alloc_mthd =
        ESP_A2D_SBC_CIE_ALLOC_MTHD_SNR | ESP_A2D_SBC_CIE_ALLOC_MTHD_LOUDNESS;
    mcc.cie.sbc_info.min_bitpool = 2;
    mcc.cie.sbc_info.max_bitpool = 250;
  }

 protected:
  /// determines sample_rate/channels from the negotiated SBC capability (a
  /// single bit is set per field once negotiation completes); the wrapped
  /// decoder also self-detects the same values from the SBC frame header
  /// itself once frames start flowing (see A2DPDecoder::get_audio_info())
  void parse_audio_info(const esp_a2d_mcc_t& mcc) override {
    const auto& sbc = mcc.cie.sbc_info;
    if (sbc.samp_freq & ESP_A2D_SBC_CIE_SF_48K) {
      sample_rate = 48000;
    } else if (sbc.samp_freq & ESP_A2D_SBC_CIE_SF_44K) {
      sample_rate = 44100;
    } else if (sbc.samp_freq & ESP_A2D_SBC_CIE_SF_32K) {
      sample_rate = 32000;
    } else if (sbc.samp_freq & ESP_A2D_SBC_CIE_SF_16K) {
      sample_rate = 16000;
    }
    channels = (sbc.ch_mode == ESP_A2D_SBC_CIE_CH_MODE_MONO) ? 1 : 2;
  }
};

#endif  // A2DP_MANAGED_DECODER_SUPPORTED
