/*
  Streaming Music from Bluetooth
  
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

// ==> Example A2DP Receiver which uses I2S to an external DAC
// This creates a new Bluetooth device with the name “MyMusic” and the output will be sent to the following default I2S pins which need to be connected to an external DAC:
//
// bck = 14
// ws = 15
// data_out = 22

#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream out;
BluetoothA2DPSink a2dp_sink(out);

void setup() {
  a2dp_sink.start("MyMusic");  
}


void loop() {
  delay(1000); // do nothing
}
