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

#include "BluetoothA2DPCommon.h"
#include "esp_log.h"

// the SBC media payload header has a 4 bit frame counter
static const int kMaxSbcFramesPerPacket = 15;

bool A2DPAudioEncoder::add_encoder(A2DPEncoder& encoder) {
  if (find(encoder.codec_type()) != nullptr) {
    ESP_LOGW(BT_AV_TAG,
             "A2DPAudioEncoder: an encoder for codec type %d is already "
             "registered",
             (int)encoder.codec_type());
    return false;
  }
  encoder.set_frame_sink(this);
  encoders.push_back(&encoder);
  return true;
}

A2DPEncoder* A2DPAudioEncoder::find(esp_a2d_mct_t type) {
  for (A2DPEncoder* enc : encoders) {
    if (enc->codec_type() == type) return enc;
  }
  return nullptr;
}

bool A2DPAudioEncoder::apply_mcc(const esp_a2d_mcc_t* mcc) {
  close();
  if (mcc == nullptr) return false;
  active = find(mcc->type);
  if (active == nullptr) {
    ESP_LOGW(BT_AV_TAG,
             "A2DPAudioEncoder: no encoder registered for codec type %d",
             (int)mcc->type);
    return false;
  }
  if (!active->begin(*mcc)) {
    ESP_LOGE(BT_AV_TAG,
             "A2DPAudioEncoder: encoder begin() failed for codec type %d",
             (int)mcc->type);
    active = nullptr;
    return false;
  }
  [[maybe_unused]] audio_tools::AudioInfo info = active->get_audio_info();
  ESP_LOGI(BT_AV_TAG,
           "A2DPAudioEncoder: using encoder for codec type %d (%d Hz, %d "
           "channels)",
           (int)mcc->type, (int)info.sample_rate, (int)info.channels);
  return true;
}

void A2DPAudioEncoder::reset() {
  free_pending();
  timestamp = 0;
}

size_t A2DPAudioEncoder::process(const uint8_t* pcm, size_t len) {
  if (active == nullptr) return 0;
  size_t result = active->write(pcm, len);
  // send what we have, so that we do not add latency
  send_pending();
  return result;
}

void A2DPAudioEncoder::close() {
  if (active != nullptr) {
    active->end();
    active = nullptr;
  }
  reset();
}

int A2DPAudioEncoder::sample_rate() {
  if (active != nullptr) return active->sample_rate();
  if (!encoders.empty()) return encoders[0]->sample_rate();
  return 44100;
}

audio_tools::AudioInfo A2DPAudioEncoder::get_audio_info() {
  if (active == nullptr) return audio_tools::AudioInfo();
  return active->get_audio_info();
}

uint16_t A2DPAudioEncoder::max_payload() {
  // the MTU must also hold the media payload header added by the stack
  uint16_t reserve = active != nullptr ? active->payload_header_size() : 0;
  if (mtu <= reserve) return 0;
  return mtu - reserve;
}

void A2DPAudioEncoder::write_frame(const uint8_t* data, size_t len,
                                   uint32_t samples) {
  if (active == nullptr || len == 0) return;
  uint16_t max_len = max_payload();
  if (max_len == 0) {
    ESP_LOGW(BT_AV_TAG, "A2DPAudioEncoder: MTU not known - frame dropped");
    return;
  }
  if (len > max_len) {
    if (!is_mtu_warning_logged) {
      ESP_LOGE(BT_AV_TAG,
               "A2DPAudioEncoder: frame of %d bytes does not fit into MTU %d "
               "- reduce the bitrate",
               (int)len, (int)mtu);
      is_mtu_warning_logged = true;
    }
    timestamp += samples;
    return;
  }

  int max_frames = active->max_frames_per_packet();
  if (max_frames > kMaxSbcFramesPerPacket) max_frames = kMaxSbcFramesPerPacket;

  // flush if the frame does not fit any more
  if (pending != nullptr && pending->data_len + len > max_len) {
    send_pending();
  }

  if (pending == nullptr) {
    // the buffer layout depends on the negotiated codec, and for AAC the
    // reserved LATM header depends on the size: so for single frame packets
    // we allocate exactly the frame size
    uint16_t size = max_frames > 1 ? max_len : len;
    pending = esp_a2d_audio_buff_alloc(size);
    if (pending == nullptr) {
      ESP_LOGE(BT_AV_TAG, "A2DPAudioEncoder: esp_a2d_audio_buff_alloc failed");
      timestamp += samples;
      return;
    }
    pending->data_len = 0;
    pending->number_frame = 0;
    pending->timestamp = timestamp;
  }

  memcpy(pending->data + pending->data_len, data, len);
  pending->data_len += len;
  pending->number_frame++;
  timestamp += samples;

  if (pending->number_frame >= max_frames) {
    send_pending();
  }
}

void A2DPAudioEncoder::send_pending() {
  if (pending == nullptr) return;
  if (pending->data_len == 0) {
    free_pending();
    return;
  }
  // the stack queue might be full: retry for a while
  int retries = A2DP_MANAGED_ENCODE_SEND_RETRY_MS;
  while (true) {
    esp_err_t rc = esp_a2d_source_audio_data_send(conn_hdl, pending);
    if (rc == ESP_OK) {
      // the buffer is now owned by the stack
      pending = nullptr;
      return;
    }
    if (rc != ESP_FAIL || retries <= 0) {
      ESP_LOGW(BT_AV_TAG,
               "A2DPAudioEncoder: esp_a2d_source_audio_data_send: %d - "
               "packet dropped",
               (int)rc);
      free_pending();
      return;
    }
    vTaskDelay(1);
    retries -= portTICK_PERIOD_MS;
  }
}

void A2DPAudioEncoder::free_pending() {
  if (pending != nullptr) {
    esp_a2d_audio_buff_free(pending);
    pending = nullptr;
  }
}

#endif  // A2DP_MANAGED_ENCODER_SUPPORTED
