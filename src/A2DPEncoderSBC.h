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

#include "AudioTools/AudioCodecs/CodecSBC.h"

/**
 * @brief SBC encode support for the managed A2DP encoder framework: wraps an
 * audio_tools::SBCEncoder (codec-sbc / arduino-libsbc), advertises the SBC
 * SEP capability and configures the encoder (subbands, blocks, allocation
 * method and bitpool) with the parameters negotiated with the sink.
 * Register it via BluetoothA2DPSource::add_encoder().
 * @ingroup codecs
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class A2DPEncoderSBC : public A2DPEncoder {
 public:
  /// max_bitpool: upper limit for the bitpool (53 is the "high quality"
  /// setting for 44.1/48 kHz stereo)
  A2DPEncoderSBC(audio_tools::SBCEncoder& encoder, int max_bitpool = 53)
      : A2DPEncoder(encoder), p_sbc(&encoder), max_bitpool(max_bitpool) {}

  esp_a2d_mct_t codec_type() override { return ESP_A2D_MCT_SBC; }

  const char* mime() override { return "audio/sbc"; }

  uint32_t samples_per_frame() override { return blocks * subbands; }

  int max_frames_per_packet() override { return 15; }

  /// the stack adds a 1 byte SBC media payload header
  uint16_t payload_header_size() override { return 1; }

  void build_capability(esp_a2d_mcc_t& mcc) override {
    mcc.type = ESP_A2D_MCT_SBC;
    // we only advertise the sample rate of the provided PCM
    mcc.cie.sbc_info.samp_freq = sample_rate_bit(input_sample_rate);
    // audio_tools::SBCEncoder supports mono and stereo (no joint stereo)
    mcc.cie.sbc_info.ch_mode =
        ESP_A2D_SBC_CIE_CH_MODE_MONO | ESP_A2D_SBC_CIE_CH_MODE_STEREO;
    mcc.cie.sbc_info.block_len =
        ESP_A2D_SBC_CIE_BLOCK_LEN_4 | ESP_A2D_SBC_CIE_BLOCK_LEN_8 |
        ESP_A2D_SBC_CIE_BLOCK_LEN_12 | ESP_A2D_SBC_CIE_BLOCK_LEN_16;
    mcc.cie.sbc_info.num_subbands =
        ESP_A2D_SBC_CIE_NUM_SUBBANDS_4 | ESP_A2D_SBC_CIE_NUM_SUBBANDS_8;
    mcc.cie.sbc_info.alloc_mthd =
        ESP_A2D_SBC_CIE_ALLOC_MTHD_SNR | ESP_A2D_SBC_CIE_ALLOC_MTHD_LOUDNESS;
    mcc.cie.sbc_info.min_bitpool = 2;
    mcc.cie.sbc_info.max_bitpool = max_bitpool;
  }

 protected:
  audio_tools::SBCEncoder* p_sbc;
  int max_bitpool;
  int blocks = 16;
  int subbands = 8;

  static uint8_t sample_rate_bit(int rate) {
    switch (rate) {
      case 16000:
        return ESP_A2D_SBC_CIE_SF_16K;
      case 32000:
        return ESP_A2D_SBC_CIE_SF_32K;
      case 44100:
        return ESP_A2D_SBC_CIE_SF_44K;
      case 48000:
        return ESP_A2D_SBC_CIE_SF_48K;
      default:
        ESP_LOGE("A2DPEncoderSBC",
                 "unsupported sample rate %d - advertising 44100", rate);
        return ESP_A2D_SBC_CIE_SF_44K;
    }
  }

  /// determines sample_rate/channels from the negotiated SBC configuration
  /// (a single bit is set per field once negotiation completes)
  void parse_audio_info(const esp_a2d_mcc_t& mcc) override {
    const auto& sbc = mcc.cie.sbc_info;
    if (sbc.samp_freq & ESP_A2D_SBC_CIE_SF_48K) {
      sample_rate_cfg = 48000;
    } else if (sbc.samp_freq & ESP_A2D_SBC_CIE_SF_44K) {
      sample_rate_cfg = 44100;
    } else if (sbc.samp_freq & ESP_A2D_SBC_CIE_SF_32K) {
      sample_rate_cfg = 32000;
    } else if (sbc.samp_freq & ESP_A2D_SBC_CIE_SF_16K) {
      sample_rate_cfg = 16000;
    }
    channels_cfg = (sbc.ch_mode == ESP_A2D_SBC_CIE_CH_MODE_MONO) ? 1 : 2;
  }

  /// applies the negotiated SBC parameters to the encoder
  void configure(const esp_a2d_mcc_t& mcc) override {
    const auto& sbc = mcc.cie.sbc_info;
    if (sbc.block_len & ESP_A2D_SBC_CIE_BLOCK_LEN_16) {
      blocks = 16;
    } else if (sbc.block_len & ESP_A2D_SBC_CIE_BLOCK_LEN_12) {
      blocks = 12;
    } else if (sbc.block_len & ESP_A2D_SBC_CIE_BLOCK_LEN_8) {
      blocks = 8;
    } else {
      blocks = 4;
    }
    subbands = (sbc.num_subbands & ESP_A2D_SBC_CIE_NUM_SUBBANDS_8) ? 8 : 4;
    int allocation = (sbc.alloc_mthd & ESP_A2D_SBC_CIE_ALLOC_MTHD_LOUDNESS)
                         ? SBC_AM_LOUDNESS
                         : SBC_AM_SNR;
    int bitpool = std::min((int)sbc.max_bitpool, max_bitpool);
    bitpool = std::max(bitpool, (int)sbc.min_bitpool);

    p_sbc->setBlocks(blocks);
    p_sbc->setSubbands(subbands);
    p_sbc->setAllocationMethod(allocation);
    p_sbc->setBitpool(bitpool);
    ESP_LOGI("A2DPEncoderSBC",
             "blocks: %d, subbands: %d, allocation: %d, bitpool: %d", blocks,
             subbands, allocation, bitpool);
  }
};

#endif  // A2DP_MANAGED_ENCODER_SUPPORTED
