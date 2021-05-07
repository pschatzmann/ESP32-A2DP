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

//  Example A2DP Receiver which uses I2S to an external DAC 
//  - Automatic shut down if no music afer 5 minutes 
//  - The LED indicates if the ESP32 is on or off

#include "BluetoothA2DPSink.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // pin number is specific to your esp32 board
#endif


BluetoothA2DPSink a2dp_sink;
esp_a2d_connection_state_t last_state;
uint16_t minutes = 5;
unsigned long shutdown_ms = millis() + 1000 * 60 * minutes;

// move shutdown time to future
void on_data() {
  shutdown_ms = millis() + 1000 * 60 * minutes; 
}

void setup() {
  Serial.begin(115200);

  // LED
  pinMode(LED_BUILTIN, OUTPUT);

  // startup sink
  a2dp_sink.set_on_data_received(on_data);
  a2dp_sink.start("MyMusic");  
}


void loop() {
  // check timeout
  if (millis()>shutdown_ms){
    // stop the processor
    Serial.println("Shutting down");
    esp_deep_sleep_start();
  }
  // check state
  esp_a2d_connection_state_t state = a2dp_sink.get_connection_state();
  if (last_state != state){
    bool is_connected = state == ESP_A2D_CONNECTION_STATE_CONNECTED;
    Serial.println(is_connected ? "Connected" : "Not connected");    
    digitalWrite(LED_BUILTIN, is_connected);
    last_state = state;
  }
  delay(1000);

}
