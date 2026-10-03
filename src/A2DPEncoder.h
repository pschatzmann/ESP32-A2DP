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

#include "config.h"

#if A2DP_MANAGED_ENCODER_SUPPORTED

#include <vector>

#include "AudioTools.h"
#include "esp_a2dp_api.h"
#include "sdkconfig.h"

/**
 * @brief Receives the encoded frames produced by an A2DPEncoder (implemented
 * by A2DPAudioEncoder, which packs them into A2DP media packets).
 * @ingroup codecs
 */
class A2DPFrameSink {
 public:
  virtual ~A2DPFrameSink() = default;
  /// one complete encoded frame covering the indicated number of samples
  /// (per channel)
  virtual void write_frame(const uint8_t* data, size_t len,
                           uint32_t samples) = 0;
};

/**
 * @brief Base class for a single A2DP codec's encode support: pairs an A2DP
 * codec type and its SEP capability advertisement with an
 * audio_tools::AudioEncoder that performs the actual encoding. Provide one
 * concrete subclass per codec (see A2DPEncoderSBC, A2DPEncoderAAC) and
 * register instances via BluetoothA2DPSource::add_encoder() - stream
 * endpoint registration is driven entirely by what gets registered here.
 *
 * The PCM which is provided by the data callbacks of the source is always
 * 16 bit stereo (as in the legacy source API) at the sample rate defined
 * with set_sample_rate() (default 44100). The sample rate is the only one
 * advertised to the sink. If the sink negotiates mono, the PCM is downmixed
 * before it is encoded.
 * @ingroup codecs
 * @ingroup a2dp
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class A2DPEncoder {
 public:
  A2DPEncoder(audio_tools::AudioEncoder& encoder)
      : p_encoder(&encoder), frame_output(*this) {}
  virtual ~A2DPEncoder() = default;

  /// A2DP codec type this encoder handles (e.g. ESP_A2D_MCT_SBC)
  virtual esp_a2d_mct_t codec_type() = 0;

  /// MIME type of the codec this encoder produces (e.g. "audio/sbc")
  virtual const char* mime() = 0;

  /// Fills in the SEP capability advertised for this codec at registration
  virtual void build_capability(esp_a2d_mcc_t& mcc) = 0;

  /// Number of samples (per channel) covered by one encoded frame: used for
  /// the media packet timestamps
  virtual uint32_t samples_per_frame() = 0;

  /// Max number of encoded frames that can be combined in one media packet
  virtual int max_frames_per_packet() { return 1; }

  /// Defines the sample rate of the PCM provided by the data callbacks
  /// (default 44100). Call before BluetoothA2DPSource::start().
  void set_sample_rate(int rate) { input_sample_rate = rate; }

  /// Sample rate of the PCM provided by the data callbacks
  int sample_rate() { return input_sample_rate; }

  /// Opens the wrapped audio_tools encoder for the negotiated SEP
  /// configuration: calls parse_audio_info(mcc) and configure(mcc)
  /// (implemented per codec), then wires up the frame output and starts the
  /// encoder.
  virtual bool begin(const esp_a2d_mcc_t& mcc) {
    sample_rate_cfg = input_sample_rate;
    channels_cfg = 2;
    parse_audio_info(mcc);
    if (sample_rate_cfg != input_sample_rate) {
      ESP_LOGW("A2DPEncoder",
               "negotiated sample rate %d differs from the PCM sample rate %d",
               sample_rate_cfg, input_sample_rate);
    }
    configure(mcc);
    p_encoder->setAudioInfo(
        audio_tools::AudioInfo(sample_rate_cfg, channels_cfg, 16));
    p_encoder->setOutput(frame_output);
    return p_encoder->begin();
  }

  /// Feeds 16 bit stereo PCM to the wrapped encoder (downmixed to mono if
  /// this was negotiated); the encoded frames are forwarded to the frame
  /// sink
  virtual size_t write(const uint8_t* pcm, size_t len) {
    if (channels_cfg != 1) return p_encoder->write(pcm, len);
    // stereo -> mono
    int16_t mono[64];
    const int16_t* stereo = (const int16_t*)pcm;
    size_t frames = len / 4;
    size_t pos = 0;
    while (pos < frames) {
      size_t n = std::min(frames - pos, sizeof(mono) / sizeof(int16_t));
      for (size_t j = 0; j < n; j++) {
        mono[j] = ((int32_t)stereo[(pos + j) * 2] +
                   stereo[(pos + j) * 2 + 1]) / 2;
      }
      p_encoder->write((const uint8_t*)mono, n * sizeof(int16_t));
      pos += n;
    }
    return len;
  }

  /// Closes the wrapped audio_tools encoder
  virtual void end() { p_encoder->end(); }

  /// Defines the target of the encoded frames
  void set_frame_sink(A2DPFrameSink* sink) { p_sink = sink; }

  /// Negotiated sample rate/channels (the PCM is always provided as stereo)
  audio_tools::AudioInfo get_audio_info() {
    return audio_tools::AudioInfo(sample_rate_cfg, channels_cfg, 16);
  }

 protected:
  /// Forwards the encoder output to write_encoded()
  class FrameOutput : public Print {
   public:
    FrameOutput(A2DPEncoder& encoder) : encoder(encoder) {}
    size_t write(uint8_t ch) override { return write(&ch, 1); }
    size_t write(const uint8_t* data, size_t len) override {
      encoder.write_encoded(data, len);
      return len;
    }
    int availableForWrite() override { return 1024; }

   protected:
    A2DPEncoder& encoder;
  };

  audio_tools::AudioEncoder* p_encoder;
  FrameOutput frame_output;
  A2DPFrameSink* p_sink = nullptr;
  int input_sample_rate = 44100;
  // negotiated configuration
  int sample_rate_cfg = 44100;
  int channels_cfg = 2;

  /// determines sample_rate_cfg/channels_cfg from the negotiated
  /// configuration
  virtual void parse_audio_info(const esp_a2d_mcc_t& mcc) = 0;

  /// optional codec specific configuration of the wrapped encoder
  virtual void configure(const esp_a2d_mcc_t& mcc) {}

  /// Receives the output of the wrapped encoder: by default each write is
  /// expected to be exactly one encoded frame
  virtual void write_encoded(const uint8_t* data, size_t len) {
    if (p_sink != nullptr && len > 0)
      p_sink->write_frame(data, len, samples_per_frame());
  }
};

/**
 * @brief Manages the set of A2DPEncoder instances registered via
 * BluetoothA2DPSource::add_encoder(), delegates encode operations to
 * whichever one matches the codec negotiated with the connected sink and
 * packs the encoded frames into A2DP media packets that are sent with
 * esp_a2d_source_audio_data_send().
 * @ingroup codecs
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class A2DPAudioEncoder : public A2DPFrameSink {
 public:
  /// Registers an encoder for its codec_type(); returns false if an encoder
  /// for that codec type is already registered
  bool add_encoder(A2DPEncoder& encoder);

  /// True if at least one encoder has been registered
  bool has_encoders() { return !encoders.empty(); }

  /// All registered encoders, in registration order - used to drive SEP
  /// registration (one stream endpoint per entry)
  std::vector<A2DPEncoder*>& all_encoders() { return encoders; }

  /// Selects and opens the encoder matching mcc->type. Returns false if no
  /// matching encoder is registered or it failed to open.
  bool apply_mcc(const esp_a2d_mcc_t* mcc);

  /// Defines the connection handle used for sending
  void set_connection_handle(esp_a2d_conn_hdl_t hdl) { conn_hdl = hdl; }

  /// Defines the MTU of the audio connection
  void set_mtu(uint16_t mtu) { this->mtu = mtu; }

  /// Restarts the media packet timestamp and drops any unsent data
  void reset();

  /// Encodes 16 bit stereo PCM with the active encoder (no-op if none is
  /// active) and sends the resulting media packets
  size_t process(const uint8_t* pcm, size_t len);

  /// Closes the active encoder (if any) and drops any unsent data
  void close();

  /// True if an encoder has been selected and opened
  bool is_active() { return active != nullptr; }

  /// Sample rate of the PCM expected by the active (or else first
  /// registered) encoder
  int sample_rate();

  /// Negotiated sample rate/channels of the active encoder
  audio_tools::AudioInfo get_audio_info();

  /// MIME type of the currently active encoder ("" if none is active)
  const char* mime() { return active != nullptr ? active->mime() : ""; }

  void write_frame(const uint8_t* data, size_t len, uint32_t samples) override;

 protected:
  std::vector<A2DPEncoder*> encoders;
  A2DPEncoder* active = nullptr;
  esp_a2d_conn_hdl_t conn_hdl = 0;
  uint16_t mtu = 0;
  uint32_t timestamp = 0;
  esp_a2d_audio_buff_t* pending = nullptr;
  bool is_mtu_warning_logged = false;

  A2DPEncoder* find(esp_a2d_mct_t type);
  /// max payload of a media packet
  uint16_t max_payload();
  /// sends the pending media packet
  void send_pending();
  void free_pending();
};

#endif  // A2DP_MANAGED_ENCODER_SUPPORTED
