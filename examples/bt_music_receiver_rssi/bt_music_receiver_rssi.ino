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

// ==> Example A2DP which uses the RSSI callback

#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

/// callback which is notified on update
void rssi(esp_bt_gap_cb_param_t::read_rssi_delta_param &rssiParam) {
  Serial.print("rssi value: ");
  Serial.println(rssiParam.rssi_delta);
}

void setup() {
  Serial.begin(115200);
  a2dp_sink.set_rssi_active(true);
  a2dp_sink.set_rssi_callback(rssi);
  a2dp_sink.start("MyMusic");
}

void loop() {
  delay(5000);
  // we can also display the last value
  Serial.print("last rssi value: ");
  Serial.println(a2dp_sink.get_last_rssi().rssi_delta);
}
