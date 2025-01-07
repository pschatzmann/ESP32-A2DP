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

// ==> Example A2DP Receiver which uses the A2DP I2S output to an Audio board which is 
//     supported by the arduino-audio-driver library e.g.
//     - AudioKitAC101 
//     - AudioKitEs8388V1 
//     - AudioKitEs8388V2 
//     - LyratMini
//     - LyratV42 
//     - LyratV43 


#include "AudioTools.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h" // install https://github.com/pschatzmann/arduino-audio-driver
#include "BluetoothA2DPSink.h"

AudioBoardStream kit(AudioKitEs8388V1); 
BluetoothA2DPSink a2dp_sink(kit);

void setup() {
  //LOGLEVEL_AUDIOKIT = AudioKitInfo;
  Serial.begin(115200);

  // use max volume
  kit.setVolume(1.0);

  // start a2dp
  a2dp_sink.start("AudioKit");  

}


void loop() {
  delay(1000); // do nothing
}