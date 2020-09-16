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
#define c4_frequency  261.63 

BluetoothA2DPSource a2dp_source;

// The supported audio codec in ESP32 A2DP is SBC. SBC audio stream is encoded
// from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data
int32_t get_data(uint8_t *data, int32_t len) {
    if (len < 0 || data == NULL) {
        return 0;
    }

    static double m_time = 0.0;
    double m_amplitude = 10000;  // max -32,768 to 32,767
    double m_deltaTime = 1.0 / 44100;
    double m_phase = 0.0;
    double double_Pi = PI * 2.0;
    Channels *channel_ptr;
    for (int sample = 0; sample < len; ++sample) {
        channel_ptr = (Channels*) &data[sample];
        channel_ptr->channel1 = m_amplitude * sin(double_Pi * c3_frequency * m_time + m_phase);
        channel_ptr->channel2 = m_amplitude * sin(double_Pi * c4_frequency * m_time + m_phase);
        m_time += m_deltaTime;
    }
    return len;
}

void setup() {
  a2dp_source.start("MyMusic", get_data);  
}

void loop() {
}