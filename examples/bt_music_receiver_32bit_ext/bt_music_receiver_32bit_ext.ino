/*
  Streaming data from Bluetooth to internal DAC of ESP32
  
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

// ==> Example to use external 32 bit DAC - We demonstrate how to create a subclass and override the audio_data_callback method

#include "BluetoothA2DPSink32.h"

BluetoothA2DPSink32 a2dp_sink; // Subclass of BluetoothA2DPSink

void setup() {
  a2dp_sink.set_bits_per_sample(32);  
  a2dp_sink.start("Hifi32bit");  
}


void loop() {
  delay(1000); // do nothing
}
