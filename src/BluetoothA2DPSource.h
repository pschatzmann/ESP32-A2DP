// C++ Class Implementation for a A2DP Source to be used as Arduino Library
// The original ESP32 implementation can be found at https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/a2dp_source
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2020 Phil Schatzmann
// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD

#pragma once

#include <vector> 
#include "BluetoothA2DPCommon.h"

typedef void (* bt_app_cb_t) (uint16_t event, void *param);
typedef  int32_t (* music_data_cb_t) (uint8_t *data, int32_t len);
typedef  int32_t (* music_data_channels_cb_t) (Frame *data, int32_t len);
typedef void (* bt_app_copy_cb_t) (app_msg_t *msg, void *p_dest, void *p_src);

extern "C" void ccall_bt_av_hdl_stack_evt(uint16_t event, void *p_param);
extern "C" void ccall_bt_app_task_handler(void *arg);
extern "C" void ccall_bt_app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
extern "C" void ccall_bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
extern "C" void ccall_a2d_app_heart_beat(void *arg) ;
extern "C" void ccall_bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
extern "C" void ccall_bt_app_av_sm_hdlr(uint16_t event, void *param);
extern "C" void ccall_bt_av_hdl_avrc_ct_evt(uint16_t event, void *param) ;
extern "C" int32_t ccall_bt_app_a2d_data_cb(uint8_t *data, int32_t len);
extern "C" int32_t ccall_get_channel_data_wrapper(uint8_t *data, int32_t len) ;
extern "C" int32_t ccall_get_data_default(uint8_t *data, int32_t len) ;


/**
 * @brief A2DP Bluetooth Source
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */

class BluetoothA2DPSource : public BluetoothA2DPCommon {
  friend void ccall_bt_av_hdl_stack_evt(uint16_t event, void *p_param);
  friend void ccall_bt_app_task_handler(void *arg);
  friend void ccall_bt_app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
  friend void ccall_bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
  friend void ccall_a2d_app_heart_beat(void *arg) ;
  friend void ccall_bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
  friend void ccall_bt_app_av_sm_hdlr(uint16_t event, void *param);
  friend void ccall_bt_av_hdl_avrc_ct_evt(uint16_t event, void *param) ;
  friend int32_t ccall_bt_app_a2d_data_cb(uint8_t *data, int32_t len);
  friend int32_t ccall_get_channel_data_wrapper(uint8_t *data, int32_t len) ;
  friend int32_t ccall_get_data_default(uint8_t *data, int32_t len) ;


  public: 
    /**
     * Constructor
     */
    BluetoothA2DPSource();

    /**
     * name: Bluetooth name of the device to connect to
     * callback: function that provides the audio stream as array of Frame
     */
    virtual void start(const char* name, music_data_channels_cb_t callback = NULL, bool is_ssp_enabled = false);
    virtual void start(std::vector<const char*> names, music_data_channels_cb_t callback = NULL, bool is_ssp_enabled = false);

    /**
     * name: Bluetooth name of the device to connect to
     * callback: function that provides the audio stream -
     * The supported audio codec in ESP32 A2DP is SBC. SBC audio stream is encoded
     * from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data
     * is_ssp_enabled: Flag to activate Secure Simple Pairing 
     */ 
    virtual void start_raw(const char* name, music_data_cb_t callback = NULL, bool is_ssp_enabled = false);
    virtual void start_raw(std::vector<const char*> names, music_data_cb_t callback = NULL, bool is_ssp_enabled = false);

    /**
     * Defines the pin code. If nothing is defined we use "1234"
     */ 
    virtual  void set_pin_code(const char* pin_code, esp_bt_pin_type_t pin_type=ESP_BT_PIN_TYPE_VARIABLE);

    /**
     * In some cases it is very difficult to use the callback function. As an alternative we provide
     * this method where you can just send the data to a queue. It is your responsibility however that
     * you handle the situation if the queue is full.
     */
    virtual bool write_data(SoundData *data);

    /**
     *  Returns true if the bluetooth device is connected
     */
    virtual  bool is_connected();

    /**
     *  Returns true if write_dataRaw has been called with any valid data
     */
    virtual  bool has_sound_data();

    /**
     *  Defines if the Flash NVS should be reset on start
     */
    virtual  void set_nvs_init(bool doInit);

    /**
     *  Defines if the BLE should be reset on start
     */
    virtual void set_reset_ble(bool doInit);


    // callback for data
    virtual int32_t get_data_default(uint8_t *data, int32_t len);
    music_data_channels_cb_t data_stream_channels_callback;

  protected:
  
    bool ssp_enabled;
    const char* bt_name;
    std::vector<const char*> bt_names;

    esp_bt_pin_type_t pin_type;
    esp_bt_pin_code_t pin_code;
    uint32_t pin_code_len;

    esp_bd_addr_t s_peer_bda;
    uint8_t s_peer_bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
    int s_a2d_state;
    int s_media_state;
    int s_intv_cnt;
    int s_connecting_intv;
    uint32_t s_pkt_cnt;
    TimerHandle_t s_tmr;
    xQueueHandle s_bt_app_task_queue;
    xTaskHandle s_bt_app_task_handle;
    // support for raw data
    SoundData *sound_data;
    int32_t sound_data_current_pos;
    bool hasSoundData;

    // initialization
    bool nvs_init = true;
    bool reset_ble = true;
    music_data_cb_t data_stream_callback;

#ifdef CURRENT_ESP_IDF
    esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
#endif


    virtual bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback);
    virtual void bt_app_task_start_up(void);
    virtual void bt_app_task_shut_down(void);
    virtual void bt_app_av_media_proc(uint16_t event, void *param);

    /* A2DP application state machine handler for each state */
    virtual void bt_app_av_state_unconnected(uint16_t event, void *param);
    virtual void bt_app_av_state_connecting(uint16_t event, void *param);
    virtual void bt_app_av_state_connected(uint16_t event, void *param);
    virtual void bt_app_av_state_disconnecting(uint16_t event, void *param);


    virtual bool bt_app_send_msg(app_msg_t *msg);
    virtual void bt_app_work_dispatched(app_msg_t *msg);

    virtual char *bda2str(esp_bd_addr_t bda, char *str, size_t size);
    virtual bool get_name_from_eir(uint8_t *eir, uint8_t *bdname, uint8_t *bdname_len);
    virtual void filter_inquiry_scan_result(esp_bt_gap_cb_param_t *param);

    /**
     *  The following mthods are called by the framework. They are public so that they can
     *  be executed from a extern "C" function.
     */
     // handler for bluetooth stack enabled events
    virtual void bt_av_hdl_stack_evt(uint16_t event, void *p_param);
    virtual void bt_app_task_handler(void *arg);
    virtual void bt_app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    /// callback function for AVRCP controller
    virtual void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
    virtual void a2d_app_heart_beat(void *arg);
    /// callback function for A2DP source
    virtual void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    /// A2DP application state machine
    virtual void bt_app_av_sm_hdlr(uint16_t event, void *param);
    /// avrc CT event handler
    virtual void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param);

#ifdef CURRENT_ESP_IDF
    void bt_av_notify_evt_handler(uint8_t event, esp_avrc_rn_param_t *param);
    void bt_av_volume_changed(void);
#endif

};
