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

// ==> Example which shows how to disconnect and reconnect

#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;
bool connected = true;

void setup() {
  Serial.begin(115200);
  //a2dp_sink.set_auto_reconnect(false);
  a2dp_sink.start("MyMusic");  

}


void loop() {
  delay(60000); // do nothing
  connected = !connected;
  Serial.print("==> setting connected to ");
  Serial.println(connected?"true":"false");

  a2dp_sink.set_connected(connected);
}
