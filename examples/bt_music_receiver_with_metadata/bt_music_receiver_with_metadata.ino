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
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);
bool is_active = true;

void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  Serial.printf("==> AVRC metadata rsp: attribute id 0x%x, %s\n", id, text);
  if (id == ESP_AVRC_MD_ATTR_PLAYING_TIME) {
    uint32_t playtime = String((char*)text).toInt();
    Serial.printf("==> Playing time is %d ms (%d seconds)\n", playtime, (int)round(playtime/1000.0));
  }
}

void setup() {
  Serial.begin(115200);
  a2dp_sink.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_PLAYING_TIME );
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.start("MyMusic");
}

void loop() {
  // pause / play ever 10 seconds
  if (a2dp_sink.get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) {
    delay(10000);
    Serial.println("changing state...");
    is_active = !is_active;
    if (is_active) {
      Serial.println("play");
      a2dp_sink.play();
    } else {
      Serial.println("pause");
      a2dp_sink.pause();
    }
  }
}
