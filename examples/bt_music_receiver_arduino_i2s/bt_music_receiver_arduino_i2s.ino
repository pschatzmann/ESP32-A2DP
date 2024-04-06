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

// ==> Example which shows how to use the built in ESP32 I2S 

#include "I2S.h"
#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink(I2S);

void setup() {
    I2S.setSckPin(14);
    I2S.setFsPin(15);
    I2S.setDataPin(22); 
    if (!I2S.begin(I2S_PHILIPS_MODE, 44100, 16)) {
      Serial.println("Failed to initialize I2S!");
      while (1); // do nothing
    }

    a2dp_sink.start("MyMusic");
}

void loop() {
}
