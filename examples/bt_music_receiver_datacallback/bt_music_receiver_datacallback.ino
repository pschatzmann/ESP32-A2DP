/*
  Streaming data from Bluetooth to callback method
  
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

// ==> Example to use built in DAC of ESP32

#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

int seconds = 2;
uint32_t end = 0;

// Then somewhere in your sketch:
void read_data_stream(const uint8_t *data, uint32_t length) {
  if (end==0){
    end = millis()+seconds*1000;
  }
  if (millis()<end){
    // process all data
    int16_t *values = (int16_t*) data;
    for (int j=0; j<length/2; j+=2){
      // print the 2 channel values
      Serial.print(values[j]);
      Serial.print(",");
      Serial.println(values[j+1]);
    }
  }
}


void setup() {
  Serial.begin(115200);

  // output to callback and no I2S 
  a2dp_sink.set_stream_reader(read_data_stream, false);
  // connect to MyMusic with no automatic reconnect
  a2dp_sink.start("MyMusic", false);  
}


void loop() {
  delay(1000);
}
