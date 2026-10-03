/*
  Streaming of sound data with Bluetooth to an other Bluetooth device -
  with SBC (and optionally AAC) encoded via the audio-tools library

  Copyright (C) 2020 Phil Schatzmann
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// ==> Example A2DP Source which encodes the PCM provided by the data callback
// via the audio-tools library instead of the internal ESP-IDF SBC encoder.
//
// Each add_encoder() call registers its own stream endpoint; which codec is
// used depends on what you've registered here plus what the sink and the
// underlying ESP-IDF version support. The data callbacks, Streams and the
// volume control work as with the internal encoder: the PCM is always 16 bit
// stereo at the sample rate defined with set_sample_rate() of the encoder
// (default 44100). If the sink negotiates mono, the PCM is downmixed.
//
// A2DPEncoderAAC wraps whatever audio_tools::AudioEncoder you give it. This
// example uses:
//  - https://github.com/pschatzmann/arduino-libsbc  (audio_tools::SBCEncoder)
//  - https://github.com/pschatzmann/arduino-fdk-aac (audio_tools::AACEncoderFDK)
//
// This requires the following sdkconfig/menuconfig settings, so you need to
// build with ESP-IDF (with arduino-esp32 as a component) or PlatformIO with
// a sdkconfig.defaults:
//   CONFIG_BT_A2DP_USE_EXTERNAL_CODEC=y
//   CONFIG_BT_A2DP_CODEC_AAC_ENABLED=y   (only needed for AAC)
//   CONFIG_BT_A2DP_SEP_NUM_MAX=2         (>= number of add_encoder() calls)
// Registering an AAC stream endpoint requires ESP-IDF >= 6.1. AAC encoding
// needs a bigger task stack: e.g. -DA2DP_MANAGED_ENCODE_TASK_STACK=40000

#define USE_AAC false

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecSBC.h"
#include "BluetoothA2DPSource.h"
#include "A2DPEncoderSBC.h"
#if USE_AAC
#include "AudioTools/AudioCodecs/CodecAACFDK.h"
#include "A2DPEncoderAAC.h"
#endif

#define c3_frequency 130.81
const float pi_2 = PI * 2.0;
const float angular_frequency = pi_2 * c3_frequency;
const float deltaAngle = angular_frequency / 44100.0;

BluetoothA2DPSource a2dp_source;
SBCEncoder sbc_encoder;
A2DPEncoderSBC a2dp_sbc(sbc_encoder);
#if USE_AAC
AACEncoderFDK aac_encoder;
A2DPEncoderAAC a2dp_aac(aac_encoder, 192000);
#endif

int32_t get_data_frames(Frame *frame, int32_t frame_count) {
  static float m_angle = 0.0;
  float m_amplitude = 10000.0;  // -32,768 to 32,767
  for (int sample = 0; sample < frame_count; ++sample) {
    frame[sample].channel1 = m_amplitude * sin(m_angle);
    frame[sample].channel2 = frame[sample].channel1;
    m_angle += deltaAngle;
    if (m_angle > pi_2) m_angle -= pi_2;
  }
  return frame_count;
}

void setup() {
  Serial.begin(115200);

#if USE_AAC
  aac_encoder.setBitrate(192000);
  a2dp_source.add_encoder(a2dp_aac);
#endif
  a2dp_source.add_encoder(a2dp_sbc);

  a2dp_source.set_data_callback_in_frames(get_data_frames);
  a2dp_source.set_volume(30);
  a2dp_source.start("LEXON MINO L");
}

void loop() {
  delay(1000);
}
