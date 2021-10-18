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

#pragma once

#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>    
#include "freertos/FreeRTOS.h" // needed for ESP Arduino < 2.0    
#include "freertos/timers.h"
#include "freertos/xtensa_api.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "driver/i2s.h"
#include "esp_avrc_api.h"
#include "esp_spp_api.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "SoundData.h"

#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#include "esp32-hal-bt.h"
#else
#include "esp_log.h"

extern "C" bool btStart();
extern "C" void delay(long millis);
extern "C" unsigned long millis();

#endif

// Support for old and new IDF version
#if !defined(CURRENT_ESP_IDF) && !defined(I2S_COMM_FORMAT_STAND_I2S)
// support for old idf releases
#define I2S_COMM_FORMAT_STAND_I2S (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB)
#define I2S_COMM_FORMAT_STAND_MSB (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB)
#endif

// prevent compile errors for ESP32C3
#ifdef ESP32C3
// DAC mode not supported!
#define I2S_MODE_DAC_BUILT_IN 0
#endif

/**
 * @brief     handler for the dispatched work
 */
typedef void (* app_callback_t) (uint16_t event, void *param);

/** @brief Internal message to be sent for BluetoothA2DPSink and BluetoothA2DPSource */
typedef struct {
    uint16_t             sig;      /*!< signal to app_task */
    uint16_t             event;    /*!< message event id */
    app_callback_t       cb;       /*!< context switch callback */
    void                 *param;   /*!< parameter area needs to be last */
} app_msg_t;


#define BT_AV_TAG        "BT_AV"
#define BT_RC_CT_TAG     "RCCT"
#define BT_APP_TAG       "BT_API"
#define APP_RC_CT_TL_GET_CAPS   (0)



/** 
 * @brief Common Bluetooth A2DP functions 
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
*/
class BluetoothA2DPCommon {
    public:
        virtual ~BluetoothA2DPCommon() = default;

        virtual  bool is_connected() = 0;

        /// obsolete: please use is_connected
        virtual bool isConnected(){
            return is_connected();
        }

       /// Prevents that the same method is executed multiple times within the indicated time limit
        virtual void debounce(void(*cb)(void),int ms){
            if (debounce_ms < millis()){
                cb();
                // new time limit
                debounce_ms = millis()+ms;
            }
        }

        /// converts a esp_bd_addr_t to a string - the string must be min 18 characters long! 
        void addr_to_str(esp_bd_addr_t bda, char *str){
            sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        }

        /// Logs the free heap
        void log_free_heap() {
            ESP_LOGI(BT_AV_TAG, "Available Heap: %zu", esp_get_free_heap_size());
        }


        
    protected:
        uint32_t debounce_ms = 0;

};

