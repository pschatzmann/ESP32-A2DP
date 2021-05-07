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

// ==> Example A2DP Receiver which uses I2S to an external DAC / LED is on when connected

#include "BluetoothA2DPSink.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // pin number is specific to your esp32 board
#endif


BluetoothA2DPSink a2dp_sink;
esp_a2d_connection_state_t last_state;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  a2dp_sink.start("MyMusic");  
}


void loop() {
  esp_a2d_connection_state_t state = a2dp_sink.get_connection_state();
  if (last_state != state){
    digitalWrite(LED_BUILTIN, state == ESP_A2D_CONNECTION_STATE_CONNECTED);
    last_state = state;
  }
  delay(1000);
}
