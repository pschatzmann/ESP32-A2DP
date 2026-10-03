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

#if A2DP_MANAGED_DECODER_SUPPORTED

#include <vector>

#include "AudioTools.h"
#include "esp_a2dp_api.h"

/// Forwards audio-info changes (sample rate/channels) reported by whichever
/// A2DPDecoder is currently active into BluetoothA2DPSink::apply_audio_info()
/// - implemented in BluetoothA2DPSink.cpp (needs friend access to that
/// protected method), declared here so A2DPDecoder can wire it up without
/// depending on BluetoothA2DPSink.h.
void a2dp_decoder_audio_info_changed(audio_tools::AudioInfo info);

/**
 * @brief Base class for a single A2DP codec's decode support: pairs an
 * A2DP codec type and its SEP capability advertisement with an
 * audio_tools::AudioDecoder that performs the actual decoding. Provide
 * one concrete subclass per codec (see A2DPDecoderSBC, A2DPDecoderAAC) and
 * register instances via BluetoothA2DPSink::add_decoder() - stream
 * endpoint registration is driven entirely by what gets registered here.
 * @ingroup codecs
 * @ingroup a2dp
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class A2DPDecoder {
 public:
  A2DPDecoder(audio_tools::AudioDecoder& decoder) : p_decoder(&decoder) {
    // decoded PCM bytes are forwarded into the sink's normal output
    // pipeline (BluetoothA2DPSink::audio_data_callback(), via the
    // ccall_audio_data_callback() trampoline) - so volume control, channel
    // swap, stream_reader callbacks and write_audio() all keep working
    // unchanged, regardless of which codec produced the PCM
    pcm_stream.setWriteCallback(write_pcm);
    // reacts immediately whenever the wrapped decoder reports a sample
    // rate/channel change, instead of the sink having to poll for it
    pcm_stream.setAudioInfoCallback(a2dp_decoder_audio_info_changed);
  }
  virtual ~A2DPDecoder() = default;

  /// A2DP codec type this decoder handles (e.g. ESP_A2D_MCT_SBC)
  virtual esp_a2d_mct_t codec_type() = 0;

  /// MIME type of the codec this decoder handles (e.g. "audio/sbc")
  virtual const char* mime() = 0;

  /// Fills in the SEP capability advertised for this codec at registration
  virtual void build_capability(esp_a2d_mcc_t& mcc) = 0;

  /// Opens the wrapped audio_tools decoder for the negotiated SEP
  /// capability: calls parse_audio_info(mcc) (implemented per codec - most
  /// decoders self-detect sample rate/channels from the encoded stream and
  /// can leave sample_rate/channels at their defaults, but codecs whose
  /// wire format doesn't carry that info per-frame, e.g. AAC, need it to
  /// synthesize the framing their decoder expects - see A2DPDecoderAAC),
  /// then wires up the PCM output and audio-info-change notification and
  /// starts the decoder.
  virtual bool begin(const esp_a2d_mcc_t& mcc) {
    parse_audio_info(mcc);
    pcm_stream.setAudioInfo(
        audio_tools::AudioInfo(sample_rate, channels, bits_per_sample));
    p_decoder->setOutput(pcm_stream);
    p_decoder->addNotifyAudioChange(pcm_stream);
    return p_decoder->begin();
  }

  /// Feeds raw encoded bytes to the wrapped decoder; decoded PCM is
  /// forwarded automatically via pcm_stream
  virtual size_t write(const uint8_t* data, size_t len) {
    return p_decoder->write(data, len);
  }

  /// Closes the wrapped audio_tools decoder
  virtual void end() { p_decoder->end(); }

  /// Returns the most recently detected sample rate/channels (default
  /// AudioInfo values until the wrapped decoder has actually parsed them
  /// from the encoded stream)
  audio_tools::AudioInfo get_audio_info() { return pcm_stream.audioInfo(); }

 protected:
  audio_tools::AudioDecoder* p_decoder;
  // doubles as the PCM output target (setOutput) and the audio-info-change
  // notification target (addNotifyAudioChange): its write callback forwards
  // decoded PCM out, and audioInfo() reports back whatever the wrapped
  // decoder last reported via setAudioInfo()/notifyAudioChange()
  audio_tools::CallbackStream pcm_stream;
  int sample_rate = 44100;
  int channels = 2;
  int bits_per_sample = 16;

  virtual void parse_audio_info(const esp_a2d_mcc_t& mcc) = 0;

 private:
  static size_t write_pcm(const uint8_t* data, size_t len);
};

/**
 * @brief Manages the set of A2DPDecoder instances registered via
 * BluetoothA2DPSink::add_decoder() and delegates decode operations to
 * whichever one matches the codec negotiated by the connected source - no
 * codec is special-cased here, it is all driven by the registered set.
 * @ingroup codecs
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class A2DPAudioDecoder {
 public:
  /// Registers a decoder for its codec_type(); returns false if a decoder
  /// for that codec type is already registered
  bool add_decoder(A2DPDecoder& decoder);

  /// True if at least one decoder has been registered
  bool has_decoders() { return !decoders.empty(); }

  /// All registered decoders, in registration order - used to drive SEP
  /// registration (one stream endpoint per entry)
  std::vector<A2DPDecoder*>& all_decoders() { return decoders; }

  /// Selects and opens the decoder matching mcc->type. Returns false if no
  /// matching decoder is registered or it failed to open.
  bool apply_mcc(const esp_a2d_mcc_t* mcc);

  /// Feeds raw encoded bytes to the currently active decoder (no-op if
  /// none is active)
  void process(const uint8_t* data, size_t len);

  /// Closes the currently active decoder (if any)
  void close();

  /// Returns the active decoder's most recently detected sample
  /// rate/channels (default AudioInfo values if none is active)
  audio_tools::AudioInfo get_audio_info();

  /// MIME type of the currently active decoder ("" if none is active)
  const char* mime() { return active != nullptr ? active->mime() : ""; }

 protected:
  std::vector<A2DPDecoder*> decoders;
  A2DPDecoder* active = nullptr;

  A2DPDecoder* find(esp_a2d_mct_t type);
};

#endif  // A2DP_MANAGED_DECODER_SUPPORTED
