/*
  Streaming of sound data with Bluetooth to other Bluetooth device.
  We generate 2 tones which will be sent to the 2 channels.
  
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
#include "BluetoothA2DPSource.h"
#include <math.h> 

#define c3_frequency  130.81
const float pi_2 = PI * 2.0;
const float angular_frequency = pi_2 * c3_frequency;

// angular_frequency (radians per second) / sampling rate (samples per second)
// gives radians changed per sample period.
const float deltaAngle = angular_frequency / 44100.0;

BluetoothA2DPSource a2dp_source;

// The supported audio codec in ESP32 A2DP is SBC. SBC audio stream is encoded
// from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data
int32_t get_data_frames(Frame *frame, int32_t frame_count) {
    static float m_angle = 0.0;
    float m_amplitude = 10000.0;  // -32,768 to 32,767
    float m_phase = 0.0;
    // fill the channel data
    for (int sample = 0; sample < frame_count; ++sample) {
        frame[sample].channel1 = m_amplitude * sin(m_angle + m_phase);
        frame[sample].channel2 = frame[sample].channel1;
        m_angle += deltaAngle;
        if (m_angle > pi_2) m_angle -= pi_2;
    }
    // to prevent watchdog
    delay(1);

    return frame_count;
}


void setup() {
  a2dp_source.set_auto_reconnect(false);
  a2dp_source.set_data_callback_in_frames(get_data_frames);
  a2dp_source.set_volume(30);
  a2dp_source.start("LEXON MINO L");  
}

void loop() {
  // to prevent watchdog in release > 1.0.6
  delay(1000);
}
