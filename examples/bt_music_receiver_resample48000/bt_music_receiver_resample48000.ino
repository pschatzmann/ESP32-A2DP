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

// ==> Example A2DP Receiver which uses I2S to an external DAC with 48000 samples/sec

#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

AudioInfo from_info(44100, 2, 16);
AudioInfo to_info(48000, 2, 16);
I2SStream out;
ResampleStream resample(out);
BluetoothA2DPSink a2dp_sink(resample);

void setup() {
  // setup i2s with 48000
  auto cfg = out.defaultConfig();
  cfg.copyFrom(to_info);
  out.begin(cfg);

  // setup resample from 42100 to 48000
  resample.begin(from_info, to_info.sample_rate);
  
  // start a2dp
  a2dp_sink.start("MyMusic");  
}


void loop() {
  delay(1000); // do nothing
}
