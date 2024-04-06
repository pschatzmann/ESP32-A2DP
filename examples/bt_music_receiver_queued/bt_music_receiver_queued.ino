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

// ==> Example A2DP Receiver which uses I2S to an external DAC. The I2S output is managed via a separate Queue which might resolve popping sounds using the volume control on some IOS devices

#include "AudioTools.h"
#include "BluetoothA2DPSinkQueued.h"

I2SStream out;
BluetoothA2DPSinkQueued a2dp_sink(out);

void setup() {
  a2dp_sink.start("MyMusicQueued");  
}


void loop() {
  delay(1000); // do nothing
}
