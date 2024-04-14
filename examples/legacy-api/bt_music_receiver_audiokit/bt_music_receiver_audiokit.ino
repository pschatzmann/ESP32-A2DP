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

// ==> Example A2DP Receiver which uses the A2DP I2S output to an AudioKit board

#include "AudioKitHAL.h" // https://github.com/pschatzmann/arduino-audiokit
#include "BluetoothA2DPSink.h"

AudioKit kit;
BluetoothA2DPSink a2dp_sink;

void setup() {
  //LOGLEVEL_AUDIOKIT = AudioKitInfo;
  Serial.begin(115200);

  // setup codec chip
  auto cfg = kit.defaultConfig(AudioOutput);
  cfg.i2s_active = false;
  kit.begin(cfg);
  kit.setVolume(100);

  Serial.println("Starting A2DP...");
  // define custom pins pins
  i2s_pin_config_t my_pin_config = {
      .mck_io_num = PIN_I2S_AUDIO_KIT_MCLK,
      .bck_io_num = PIN_I2S_AUDIO_KIT_BCK,
      .ws_io_num = PIN_I2S_AUDIO_KIT_WS,
      .data_out_num = PIN_I2S_AUDIO_KIT_DATA_OUT,
      .data_in_num = I2S_PIN_NO_CHANGE
  };
  a2dp_sink.set_pin_config(my_pin_config);
  // start a2dp
  a2dp_sink.start("AudioKit");  

}


void loop() {
  delay(1000); // do nothing
}