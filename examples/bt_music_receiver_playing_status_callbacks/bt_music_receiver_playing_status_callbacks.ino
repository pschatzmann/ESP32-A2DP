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

I2SStream out;
BluetoothA2DPSink a2dp_sink(i2s);

void avrc_rn_play_pos_callback(uint32_t play_pos) {
  Serial.printf("Play position is %d (%d seconds)\n", play_pos, (int)round(play_pos/1000.0));
}

void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback) {
  switch (playback) {
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_STOPPED:
      Serial.println("Stopped.");
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PLAYING:
      Serial.println("Playing.");
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PAUSED:
      Serial.println("Paused.");
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_FWD_SEEK:
      Serial.println("Forward seek.");
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_REV_SEEK:
      Serial.println("Reverse seek.");
      break;
    case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_ERROR:
      Serial.println("Error.");
      break;
    default:
      Serial.printf("Got unknown playback status %d\n", playback);
  }
}

void setup() {
  Serial.begin(115200);
  a2dp_sink.set_avrc_rn_playstatus_callback(avrc_rn_playstatus_callback);
  a2dp_sink.set_avrc_rn_play_pos_callback(avrc_rn_play_pos_callback);
  a2dp_sink.start("MyMusic");
}

void loop() {
  Serial.println("Play or pause music to test callbacks.");
  delay(10000);
}
