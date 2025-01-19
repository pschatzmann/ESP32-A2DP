/*
  Example that demonstrates how to handle button presses on a bluetooth speaker.
  
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

BluetoothA2DPSource a2dp_source;

// The supported audio codec in ESP32 A2DP is SBC. SBC audio stream is encoded
// from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data
int32_t get_data(uint8_t *data, int32_t bytes) {
    // fill the channel silence data
    memset(data, 0, bytes);
    return bytes;
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
  a2dp_source.set_data_callback(get_data);
  a2dp_source.set_avrc_passthru_command_callback(button_handler);
  a2dp_source.start("My vision");  
}

void loop() {
  delay(1000);
}