
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2020 Phil Schatzmann

#ifndef ARDUINO_ARCH_ESP32

#include "BluetoothA2DPCommon.h"

/**
 * @brief Startup logic as implemented by Arduino - This is not available if we use this library outside of Arduino
 * 
 * @return true 
 * @return false 
 */
bool btStart(){
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        return true;
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){
        esp_bt_controller_init(&cfg);
        while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){}
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED){
        if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) {
            ESP_LOGE(BT_APP_TAG, "BT Enable failed");
            return false;
        }
    }
    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED){
        return true;
    }
    ESP_LOGE(BT_APP_TAG, "BT Start failed");
    return false;
}

/**
 * @brief call vTaskDelay to deley for the indicated number of milliseconds
 * 
 */
void delay(long millis) {
    const TickType_t xDelay = millis / portTICK_PERIOD_MS; 
    vTaskDelay(xDelay);
}

unsigned long millis() {
    return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

#endif 






