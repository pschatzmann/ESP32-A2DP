
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

#include "BluetoothA2DPCommon.h"

esp_a2d_audio_state_t BluetoothA2DPCommon::get_audio_state() {
  return audio_state;
}

esp_a2d_connection_state_t BluetoothA2DPCommon::get_connection_state() {
    return connection_state;
}


/// activate / deactivate the automatic reconnection to the last address (per default this is on)
void BluetoothA2DPCommon::set_auto_reconnect(bool active){
    this->reconnect_status = active ? AutoReconnect:NoReconnect;
}

/// Reconnects to the last device
bool BluetoothA2DPCommon::reconnect() {
    if (has_last_connection()) {
        is_autoreconnect_allowed = true;
        reconnect_status = IsReconnecting;
        reconnect_timout = millis() + default_reconnect_timout;
        return connect_to(last_connection);
    }

    return false;
}

bool BluetoothA2DPCommon::connect_to(esp_bd_addr_t peer){
    ESP_LOGW(BT_AV_TAG, "connect_to to %s", to_str(last_connection));
    set_scan_mode_connectable_default();
    esp_err_t err = esp_a2d_connect(peer);
    if (err!=ESP_OK){
        ESP_LOGE(BT_AV_TAG, "esp_a2d_source_connect:%d", err);
    }
    return err==ESP_OK;
}

/// Calls disconnect or reconnect
void BluetoothA2DPCommon::set_connected(bool active){
    if (active){
        reconnect();
    } else {
        disconnect();
    }
}


/// Closes the connection
void BluetoothA2DPCommon::disconnect()
{
    ESP_LOGI(BT_AV_TAG, "disconect a2d: %s", to_str(last_connection));

    // Prevent automatic reconnect
    is_autoreconnect_allowed = false;

    esp_err_t status = esp_a2d_sink_disconnect(last_connection);
    if (status == ESP_FAIL) {
        ESP_LOGE(BT_AV_TAG, "Failed disconnecting to device!");
    }
}


void BluetoothA2DPCommon::end(bool release_memory) {
    // reconnect should not work after end
    is_start_disabled = false;
    clean_last_connection();
    log_free_heap();

    // Disconnect
    disconnect();
    while(is_connected()){
        delay(100);
    }

    // deinit AVRC
    ESP_LOGI(BT_AV_TAG,"deinit avrc");
    if (esp_avrc_ct_deinit() != ESP_OK){
         ESP_LOGE(BT_AV_TAG,"Failed to deinit avrc");
    }
    log_free_heap();

    if (release_memory) {

        ESP_LOGI(BT_AV_TAG,"disable bluetooth");
        if (esp_bluedroid_disable() != ESP_OK){
            ESP_LOGE(BT_AV_TAG,"Failed to disable bluetooth");
        }
        log_free_heap();

    
        ESP_LOGI(BT_AV_TAG,"deinit bluetooth");
        if (esp_bluedroid_deinit() != ESP_OK){
            ESP_LOGE(BT_AV_TAG,"Failed to deinit bluetooth");
        }
        log_free_heap();


        ESP_LOGI(BT_AV_TAG,"esp_bt_controller_disable");
        if (esp_bt_controller_disable()!=ESP_OK){
            ESP_LOGE(BT_AV_TAG,"esp_bt_controller_disable failed");
        }
        log_free_heap();

        // waiting for status change
        while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED)
            delay(50);

        if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED){
            ESP_LOGI(BT_AV_TAG,"esp_bt_controller_deinit");
            if (esp_bt_controller_deinit()!= ESP_OK){
                ESP_LOGE(BT_AV_TAG,"esp_bt_controller_deinit failed");
            }
            log_free_heap();
        }
    
        // after a release memory - a restart will not be possible
        ESP_LOGI(BT_AV_TAG,"esp_bt_controller_mem_release");
        if (esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)!= ESP_OK){
            ESP_LOGE(BT_AV_TAG,"esp_bt_controller_mem_release failed");
        }
        log_free_heap();
        is_start_disabled = true;

    } 

    log_free_heap();
}

bool BluetoothA2DPCommon::has_last_connection() {  
    esp_bd_addr_t empty_connection = {0,0,0,0,0,0};
    int result = memcmp(last_connection, empty_connection, ESP_BD_ADDR_LEN);
    return result!=0;
}

void BluetoothA2DPCommon::get_last_connection(){
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    nvs_handle my_handle;
    esp_err_t err;
    
    err = nvs_open("connected_bda", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
         ESP_LOGE(BT_AV_TAG,"NVS OPEN ERROR");
    }

    esp_bd_addr_t bda;
    size_t size = sizeof(bda);
    err = nvs_get_blob(my_handle, last_bda_nvs_name(), bda, &size);
    if ( err != ESP_OK) { 
        if ( err == ESP_ERR_NVS_NOT_FOUND ) {
            ESP_LOGI(BT_AV_TAG, "nvs_blob does not exist");
        } else {
            ESP_LOGE(BT_AV_TAG, "nvs_get_blob failed");
        }
    }
    nvs_close(my_handle);
    if (err == ESP_OK) {
        memcpy(last_connection,bda,size);
    } 
    ESP_LOGD(BT_AV_TAG, "=> %s", to_str(last_connection));

}

void BluetoothA2DPCommon::set_last_connection(esp_bd_addr_t bda){
    ESP_LOGD(BT_AV_TAG, "set_last_connection: %s", to_str(bda));

    //same value, nothing to store
    if ( memcmp(bda, last_connection, ESP_BD_ADDR_LEN) == 0 ) {
        ESP_LOGD(BT_AV_TAG, "no change!");
        return; 
    }
    nvs_handle my_handle;
    esp_err_t err;
    
    err = nvs_open("connected_bda", NVS_READWRITE, &my_handle);
    if (err != ESP_OK){
         ESP_LOGE(BT_AV_TAG, "NVS OPEN ERROR");
    }
    err = nvs_set_blob(my_handle, last_bda_nvs_name(), bda, ESP_BD_ADDR_LEN);
    if (err == ESP_OK) {
        err = nvs_commit(my_handle);
    } else {
        ESP_LOGE(BT_AV_TAG, "NVS WRITE ERROR");
    }
    if (err != ESP_OK) {
        ESP_LOGE(BT_AV_TAG, "NVS COMMIT ERROR");
    }
    nvs_close(my_handle);
    memcpy(last_connection, bda, ESP_BD_ADDR_LEN);
}

void BluetoothA2DPCommon::clean_last_connection() {
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    esp_bd_addr_t cleanBda = { 0 };
    set_last_connection(cleanBda);
}


/// Set the callback that is called when the connection state is changed
void BluetoothA2DPCommon::set_on_connection_state_changed(void (*callBack)(esp_a2d_connection_state_t state, void*), void* obj){
    connection_state_callback = callBack;
    connection_state_obj = obj;
}

/// Set the callback that is called when the audio state is changed
/// This callback is called before the I2S bus is changed.
void BluetoothA2DPCommon::set_on_audio_state_changed(void (*callBack)(esp_a2d_audio_state_t state, void*), void* obj){
    audio_state_callback = callBack;
    audio_state_obj = obj;
}

/// Set the callback that is called after the audio state has changed.
/// This callback is called after the I2S bus has changed.
void BluetoothA2DPCommon::set_on_audio_state_changed_post(void (*callBack)(esp_a2d_audio_state_t state, void*), void* obj){
    audio_state_callback_post = callBack;
    audio_state_obj_post = obj;
}


/// Prevents that the same method is executed multiple times within the indicated time limit
void BluetoothA2DPCommon::debounce(void(*cb)(void),int ms){
    if (debounce_ms < millis()){
        cb();
        // new time limit
        debounce_ms = millis()+ms;
    }
}

/// Logs the free heap
void BluetoothA2DPCommon::log_free_heap() {
    ESP_LOGI(BT_AV_TAG, "Available Heap: %zu", esp_get_free_heap_size());
}

/// converts esp_a2d_connection_state_t to a string
const char* BluetoothA2DPCommon::to_str(esp_a2d_connection_state_t state){
    return m_a2d_conn_state_str[state];
}

/// converts a esp_a2d_audio_state_t to a string
const char* BluetoothA2DPCommon::to_str(esp_a2d_audio_state_t state){
    return m_a2d_audio_state_str[state];
}

/// converts a esp_bd_addr_t to a string - the string is 18 characters long! 
const char* BluetoothA2DPCommon::to_str(esp_bd_addr_t bda){
    static char bda_str[18];
    sprintf(bda_str, "%02x:%02x:%02x:%02x:%02x:%02x", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
    return (const char*)bda_str;
}

#ifdef ESP_IDF_4

/// converts a esp_a2d_audio_state_t to a string
const char* BluetoothA2DPCommon::to_str(esp_avrc_playback_stat_t state){
    if (state == esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_ERROR)
        return "error";
    else {
        return m_avrc_playback_state_str[state];
    }
}


/// Defines if the bluetooth is discoverable
void BluetoothA2DPCommon::set_discoverability(esp_bt_discovery_mode_t d) {
  discoverability = d;
  if (get_connection_state() == ESP_A2D_CONNECTION_STATE_DISCONNECTED || d != ESP_BT_NON_DISCOVERABLE) {
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, discoverability);
  }
}


/// Defines if the bluetooth is connectable
void BluetoothA2DPCommon::set_scan_mode_connectable(bool connectable) {
    ESP_LOGI(BT_AV_TAG,"set_scan_mode_connectable %s", connectable ? "true":"false" );            
    if (connectable){
        if (esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, discoverability)!=ESP_OK) {
            ESP_LOGE(BT_AV_TAG,"esp_bt_gap_set_scan_mode");            
        } 
    } else {
        if (esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE)) {
            ESP_LOGE(BT_AV_TAG,"esp_bt_gap_set_scan_mode");            
        }    
    }
}
#else 

void BluetoothA2DPCommon::set_scan_mode_connectable(bool connectable) {
    ESP_LOGI(BT_AV_TAG,"set_scan_mode_connectable %s", connectable ? "true":"false" );            
    if (connectable){
        if (esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE)!=ESP_OK){
            ESP_LOGE(BT_AV_TAG,"esp_bt_gap_set_scan_mode");            
        } 
    } else {
        if (esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_NONE)!=ESP_OK){
            ESP_LOGE(BT_AV_TAG,"esp_bt_gap_set_scan_mode");            
        }    
    }
}

#endif


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





