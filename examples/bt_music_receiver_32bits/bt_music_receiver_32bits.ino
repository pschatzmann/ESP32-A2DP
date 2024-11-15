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

// ==> Example A2DP Receiver which uses the A2DP I2S output with 32 bits

#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

AudioInfo info(44100, 2, 32);
I2SStream out;
NumberFormatConverterStream convert(out);
BluetoothA2DPSink a2dp_sink(convert);

void setup() {
  Serial.begin(115200);
  // AudioToolsLogger.begin(Serial, AudioLogger::Info);

  // Configure i2s to use 32 bits
  auto cfg = out.defaultConfig();
  cfg.copyFrom(info);
  out.begin(cfg);

  // Convert from 16 to 32 bits
  convert.begin(16, 32);

  // start a2dp
  a2dp_sink.start("AudioKit");  
}


void loop() {
  delay(1000); // do nothing
}