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

// Example that demonstrates how to handle button presses on a bluetooth speaker

#include "BluetoothA2DPSource.h"

BluetoothA2DPSource a2dp_source;

// The supported audio codec in ESP32 A2DP is SBC. SBC audio stream is encoded
// from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data
int32_t get_data_channels(Frame *frame, int32_t channel_len) {
    // fill the channel silence data
    for (int sample = 0; sample < channel_len; ++sample) {
        frame[sample].channel1 = 0;
        frame[sample].channel2 = 0;
    }
    return channel_len;
}

// gets called when button on bluetooth speaker is pressed
void button_handler(uint8_t id, bool isReleased){
  if (isReleased) {
    Serial.print("button id ");
    Serial.print(id);
    Serial.println(" released");
  }
}

void setup() {
  Serial.begin(115200);

  a2dp_source.set_avrc_passthru_command_callback(button_handler);
  a2dp_source.start("My vision", get_data_channels);  
}


void loop() {
  delay(1000);
}