/*
  Streaming of sound data with Bluetooth to an other Bluetooth device.
  We provide the complete sound data as a simple c array which 
  can be prepared e.g. in the following way

  - Open any sound file in Audacity. Make sure that it contains 2 channels
    - Select Tracks -> Resample and select 44100
    - Export -> Export Audio -> Header Raw ; Signed 16 bit PCM
  - Convert to c file e.g. with "xxd -i file_example_WAV_1MG.raw file_example_WAV_1MG.c"
    - add the const qualifier to the array definition. E.g const unsigned char file_example_WAV_1MG_raw[] = {
  
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

// ==> complie with Partition Scheme - Huge APP !

#include "BluetoothA2DPSource.h"
#include "StarWars30.h"

BluetoothA2DPSource a2dp_source;
SoundData *data = new OneChannelSoundData((int16_t*)StarWars30_raw, StarWars30_raw_len/2);

void setup() {
  a2dp_source.start("RadioPlayer");  
  a2dp_source.write_data(data);
}

void loop() {
}