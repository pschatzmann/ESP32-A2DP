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
#include "VolumeControl.h"

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
        /// Destructor
        virtual ~BluetoothA2DPCommon() = default;

        /// Closes the connection
        virtual void disconnect();

        /// Closes the connection and stops A2DP
        virtual void end(bool releaseMemory=false);

        /// Checks if A2DP is connected
        virtual  bool is_connected() = 0;

        /// obsolete: please use is_connected
        virtual bool isConnected(){
            return is_connected();
        }

        /// Changes the volume (use the range 0-100)
        virtual void set_volume(uint8_t volume) = 0;
        
        /// Determines the volume
        virtual int get_volume() = 0;

        /// you can define a custom VolumeControl implementation
        virtual void set_volume_control(VolumeControl *ptr){
            volume_control_ptr = ptr;
        }

        /// Determine the actual audio state
        virtual esp_a2d_audio_state_t get_audio_state();
    
        /// Determine the connection state
        virtual esp_a2d_connection_state_t get_connection_state();

        /// Set the callback that is called when the connection state is changed
        virtual void set_on_connection_state_changed(void (*callBack)(esp_a2d_connection_state_t state, void *), void *obj=nullptr);

        /// Set the callback that is called when the audio state is changed
        virtual void set_on_audio_state_changed(void (*callBack)(esp_a2d_audio_state_t state, void*), void* obj=nullptr);

       /// Prevents that the same method is executed multiple times within the indicated time limit
        virtual void debounce(void(*cb)(void),int ms);

        /// Logs the free heap
        void log_free_heap();

        /// converts esp_a2d_connection_state_t to a string
        const char* to_str(esp_a2d_connection_state_t state);

        /// converts a esp_a2d_audio_state_t to a string
        const char* to_str(esp_a2d_audio_state_t state);

        /// converts a esp_bd_addr_t to a string - the string is 18 characters long! 
        const char* to_str(esp_bd_addr_t bda);


    protected:
        uint32_t debounce_ms = 0;
        DefaultVolumeControl default_volume_control;
        VolumeControl *volume_control_ptr;
        esp_bd_addr_t last_connection = {0,0,0,0,0,0};
        bool is_start_disabled = false;
        void (*connection_state_callback)(esp_a2d_connection_state_t state, void* obj) = nullptr;
        void (*audio_state_callback)(esp_a2d_audio_state_t state, void* obj) = nullptr;
        void *connection_state_obj = nullptr;
        void *audio_state_obj = nullptr;
        const char *m_a2d_conn_state_str[4] = {"Disconnected", "Connecting", "Connected", "Disconnecting"};
        const char *m_a2d_audio_state_str[3] = {"Suspended", "Stopped", "Started"};
        esp_a2d_audio_state_t audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
        esp_a2d_connection_state_t connection_state = ESP_A2D_CONNECTION_STATE_DISCONNECTED;

        virtual void get_last_connection();
        virtual void set_last_connection(esp_bd_addr_t bda);
        virtual void clean_last_connection();
        virtual void connect_to_last_device();
        virtual bool has_last_connection();
        // change the scan mode
        virtual void set_scan_mode_connectable(bool connectable);

        /// provides access to the VolumeControl object
        virtual VolumeControl* volume_control() {
            return volume_control_ptr !=nullptr ? volume_control_ptr : &default_volume_control;
        }


};

