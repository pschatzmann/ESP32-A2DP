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
// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD

#pragma once

#include "BluetoothA2DPCommon.h"
#include "freertos/ringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_SIG_WORK_DISPATCH (0x01)

#ifndef BT_AV_TAG
#define BT_AV_TAG               "BT_AV"
#endif

/* @brief event for handler "bt_av_hdl_stack_up */
enum {
    BT_APP_EVT_STACK_UP = 0,
};

extern "C" void ccall_app_a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
extern "C" void ccall_app_rc_ct_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
extern "C" void ccall_app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
extern "C" void ccall_app_task_handler(void *arg);
extern "C" void ccall_i2s_task_handler(void *arg);
extern "C" void ccall_audio_data_callback(const uint8_t *data, uint32_t len);
extern "C" void ccall_av_hdl_stack_evt(uint16_t event, void *p_param);
extern "C" void ccall_av_hdl_a2d_evt(uint16_t event, void *p_param);
extern "C" void ccall_av_hdl_avrc_evt(uint16_t event, void *p_param);

#ifdef ESP_IDF_4
extern "C" void ccall_app_rc_tg_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
extern "C" void ccall_av_hdl_avrc_tg_evt(uint16_t event, void *p_param);
#endif    

// defines the mechanism to confirm a pin request
enum PinCodeRequest {Undefined, Confirm, Reply};

/**
 * @brief A2DP Bluethooth Sink - We initialize and start the Bluetooth A2DP Sink. 
 * The example https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/a2dp_sink
 * was refactered into a C++ class 
 * @ingroup a2dp
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */

class BluetoothA2DPSink : public BluetoothA2DPCommon {

    /// handle esp_a2d_cb_event_t 
    friend void ccall_app_a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    /// handle esp_avrc_ct_cb_event_t
    friend void ccall_app_rc_ct_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
    /// GAP callback
    friend void ccall_app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    /// task handler
    friend void ccall_app_task_handler(void *arg);
    /// task hander for i2s 
    friend void ccall_i2s_task_handler(void *arg);
    /// Callback for music stream 
    friend void ccall_audio_data_callback(const uint8_t *data, uint32_t len);
    /// av event handler
    friend void ccall_av_hdl_stack_evt(uint16_t event, void *p_param);
    /// a2dp event handler 
    friend void ccall_av_hdl_a2d_evt(uint16_t event, void *p_param);
    /// avrc event handler 
    friend void ccall_av_hdl_avrc_evt(uint16_t event, void *p_param);

#ifdef ESP_IDF_4

    /// handle esp_avrc_tg_cb_event_t
    friend void ccall_app_rc_tg_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
    /* avrc TG event handler */
    friend void ccall_av_hdl_avrc_tg_evt(uint16_t event, void *p_param);

#endif    

  public: 
    /// Constructor
    BluetoothA2DPSink();
    /// Destructor - stops the playback and releases all resources
    virtual ~BluetoothA2DPSink();

#if A2DP_I2S_SUPPORT
    /// Define the pins
    virtual void set_pin_config(i2s_pin_config_t pin_config);
   
    /// Define an alternative i2s port other then 0 
    virtual void set_i2s_port(i2s_port_t i2s_num);
   
    /// Define the i2s configuration
    virtual void set_i2s_config(i2s_config_t i2s_config);

    /// set output to I2S_CHANNEL_STEREO (default) or I2S_CHANNEL_MONO
    virtual void set_channels(i2s_channel_t channels) {
        set_mono_downmix(channels==I2S_CHANNEL_MONO);
    }

    /// Defines the bits per sample for output (if > 16 output will be expanded)
    virtual void set_bits_per_sample(int bps) { i2s_config.bits_per_sample = (i2s_bits_per_sample_t) bps; }

    virtual esp_err_t i2s_mclk_pin_select(const uint8_t pin);

#endif

    /// starts the I2S bluetooth sink with the inidicated name
    virtual void start(const char* name, bool auto_reconect);

    /// starts the I2S bluetooth sink with the inidicated name
    virtual void start(const char* name);

    /// ends the I2S bluetooth sink with the indicated name - if you release the memory a future start is not possible
    virtual void end(bool release_memory=false);
    
    /// Returns true if the state is connected
    virtual bool is_connected();

    /// Determine the actual audio type
    virtual esp_a2d_mct_t get_audio_type();

    /// Define a callback method which provides connection state of AVRC service
    virtual void set_avrc_connection_state_callback(void (*callback)(bool)) {
      this->avrc_connection_state_callback = callback;
    }

    /// Define a callback method which provides the meta data
    virtual void set_avrc_metadata_callback(void (*callback)(uint8_t, const uint8_t*)) {
      this->avrc_metadata_callback = callback;
    }

#ifdef ESP_IDF_4
    /// Define a callback method which provides the avrc notifications
    virtual void set_avrc_rn_playstatus_callback(void (*callback)(esp_avrc_playback_stat_t playback)) {
      this->avrc_rn_playstatus_callback = callback;
    }
#endif

    /// Defines the method which will be called with the sample rate is updated
    virtual void set_sample_rate_callback(void (*callback)(uint16_t rate)) {
      this->sample_rate_callback = callback;
    }

    /// Define callback which is called when we receive data: This callback provides access to the data
    virtual void set_stream_reader(void (*callBack)(const uint8_t*, uint32_t), bool i2s_output=true);

    /// Define a callback that is called before the volume changes: this callback provides access to the data
    virtual void set_raw_stream_reader(void (*callBack)(const uint8_t*, uint32_t));

    /// Define callback which is called when we receive data
    virtual void set_on_data_received(void (*callBack)());
    
    /// Allows you to reject unauthorized addresses
    virtual void set_address_validator(bool (*callBack)(esp_bd_addr_t remote_bda)){
        address_validator = callBack;
    }

    /// returns true if the avrc service is connected
    virtual bool is_avrc_connected();

    /// Changes the volume
    virtual void set_volume(uint8_t volume);
    
    /// Determines the volume
    virtual int get_volume();

    /// Set the callback that is called when they change the volume (kept for compatibility)
    virtual void set_on_volumechange(void (*callBack)(int));

	/// Set the callback that is called when remote changes the volume
    virtual void set_avrc_rn_volumechange(void (*callBack)(int));

    /// set the callback that the local volume change is notification is received and complete
    virtual void set_avrc_rn_volumechange_completed(void (*callBack)(int));

    /// Starts to play music using AVRC
    virtual void play();
    /// AVRC pause
    virtual void pause();
    /// AVRC stop
    virtual void stop();
    /// AVRC next
    virtual void next();
    /// AVRC previous
    virtual void previous();
    /// AVRC fast_forward
    virtual void fast_forward();
    /// AVRC rewind
    virtual void rewind();
    
    /// mix stereo into single mono signal
    virtual void set_mono_downmix(bool enabled) {
        volume_control()->set_mono_downmix(enabled);
    }
    
    /// Provides the actually set data rate (in samples per second)
    virtual uint16_t sample_rate() { return m_sample_rate;}
        
    /// We need to confirm a new seesion by calling confirm_pin_code()
    virtual void activate_pin_code(bool active);

    /// confirms the connection request by returning the receivedn pin code
    virtual void confirm_pin_code();

    /// confirms the connection request by returning the indicated pin code
    virtual void confirm_pin_code(int code);

    /// provides the requested pin code (0 = undefined)
    virtual int pin_code() {
        return pin_code_int;
    }

    /// defines the requested metadata: eg. ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_TRACK_NUM | ESP_AVRC_MD_ATTR_NUM_TRACKS | ESP_AVRC_MD_ATTR_GENRE | ESP_AVRC_MD_ATTR_PLAYING_TIME
    virtual void set_avrc_metadata_attribute_mask(int flags){
        avrc_metadata_flags = flags;
    }

    /// swaps the left and right channel
    virtual void set_swap_lr_channels(bool swap){
        swap_left_right = swap;
    }

    /// Defines the number of times that the system tries to automatically reconnect to the last system
    virtual void set_auto_reconnect(bool reconnect, int count=AUTOCONNECT_TRY_NUM ){
        reconnect_status = reconnect ? AutoReconnect : NoReconnect;
        try_reconnect_max_count = count;
    }

    /// Provides the address of the connected device
    virtual esp_bd_addr_t* get_current_peer_address() {
        return &peer_bd_addr;
    }


    /// Activates the rssi reporting
    void set_rssi_active(bool active){
        rssi_active = active;
    }

    /// provides the last rssi parameters
    esp_bt_gap_cb_param_t::read_rssi_delta_param get_last_rssi() {
        return last_rssi_delta;
    }

    /// Defines the callback that is called when we get an new rssi value
    void set_rssi_callback(void (*callback)(esp_bt_gap_cb_param_t::read_rssi_delta_param &rssi)){
        rssi_callbak = callback;
    }

    /// Defines the delay that is added to delay the startup when we automatically reconnect
    void set_reconnect_delay(int delay){
        reconnect_delay = delay;
    }

    /// Activates SSP (Serial protocol)
    void set_spp_active(bool flag){
        spp_active = flag;
    }

 #ifdef ESP_IDF_4
    /// Get the name of the connected source device (obsolete)
    virtual const char* get_connected_source_name();
    /// Provides the result of the last result for the esp_avrc_tg_get_rn_evt_cap() callback (Available from ESP_IDF_4)
    bool is_avrc_peer_rn_cap(esp_avrc_rn_event_ids_t cmd) {
        return esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap, cmd);
    }
    /// Returns true if the is_avrc_peer_rn_cap() method can be called
    bool is_avrc_peer_rn_cap_available() {
        return s_avrc_peer_rn_cap.bits != 0;
    }

    /// Get the name of the connected source device 
    virtual const char* get_peer_name() {
        return get_connected_source_name();
    }

 #endif

  protected:
    // protected data
    xQueueHandle app_task_queue = nullptr;
    xTaskHandle app_task_handle = nullptr;

    bool is_i2s_output = A2DP_I2S_SUPPORT;
#if A2DP_I2S_SUPPORT
    i2s_config_t i2s_config;
    i2s_pin_config_t pin_config;    
    i2s_channel_t i2s_channels = I2S_CHANNEL_STEREO;
    i2s_port_t i2s_port = I2S_NUM_0; 
    bool is_i2s_active = false;
#endif
    uint16_t m_sample_rate = 0; 
    uint32_t m_pkt_cnt = 0;
    //esp_a2d_audio_state_t m_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
    esp_a2d_mct_t audio_type;
    char pin_code_str[20] = {0};
    bool player_init = false;
    int connection_rety_count = 0;
    bool spp_active = false;
    esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
    _lock_t s_volume_lock;
    uint8_t s_volume = 0;
    bool s_volume_notify;
    int pin_code_int = 0;
    PinCodeRequest pin_code_request = Undefined;
    bool is_pin_code_active = false;
    bool avrc_connection_state = false;
    int avrc_metadata_flags = ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_TRACK_NUM | ESP_AVRC_MD_ATTR_NUM_TRACKS | ESP_AVRC_MD_ATTR_GENRE;
    void (*bt_volumechange)(int) = nullptr;
    void (*bt_dis_connected)() = nullptr;
    void (*bt_connected)() = nullptr;
    void (*data_received)() = nullptr;
    void (*stream_reader)(const uint8_t*, uint32_t) = nullptr;
    void (*raw_stream_reader)(const uint8_t*, uint32_t) = nullptr;
    void (*avrc_connection_state_callback)(bool connected) = nullptr;
    void (*avrc_metadata_callback)(uint8_t, const uint8_t*) = nullptr;
#ifdef ESP_IDF_4
    void (*avrc_rn_playstatus_callback)(esp_avrc_playback_stat_t) = nullptr;
#endif
    void (*avrc_rn_volchg_complete_callback)(int) = nullptr;
    bool (*address_validator)(esp_bd_addr_t remote_bda) = nullptr;
    void (*sample_rate_callback)(uint16_t rate)=nullptr;
    bool swap_left_right = false;
    int try_reconnect_max_count = AUTOCONNECT_TRY_NUM;
   
    // RSSI support
    esp_bt_gap_cb_param_t::read_rssi_delta_param last_rssi_delta;
    bool rssi_active = false;
    void (*rssi_callbak)(esp_bt_gap_cb_param_t::read_rssi_delta_param &rssi) = nullptr;
    int reconnect_delay = 1000;

#ifdef ESP_IDF_4
    esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
    char remote_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
#endif

    // protected methods
    virtual int init_bluetooth();
    virtual void app_task_start_up(void);
    virtual void app_task_shut_down(void);
    virtual bool app_send_msg(bt_app_msg_t *msg);
    virtual bool app_work_dispatch(app_callback_t p_cback, uint16_t event, void *p_params, int param_len);
    virtual void app_work_dispatched(bt_app_msg_t *msg);
    virtual void app_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param);
    virtual void av_new_track();
    virtual void av_playback_changed();
    virtual void init_nvs();
    // execute AVRC command
    virtual void execute_avrc_command(int cmd);

    virtual const char* last_bda_nvs_name() {
        return "last_bda";
    }

    virtual bool is_reconnect(esp_a2d_disc_rsn_t type) {
        bool result = is_autoreconnect_allowed && (reconnect_status==AutoReconnect || reconnect_status==IsReconnecting) && has_last_connection();
        ESP_LOGI(BT_AV_TAG,"is_reconnect: %s", result ? "true":"false");
        return result;
    }

    /**
     * Wrappbed methods called from callbacks
     */
    // task handler
    virtual void app_task_handler(void *arg);
    // a2d callback
    virtual void app_a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    // GAP callback
    virtual void app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    // avrc callback
    virtual void app_rc_ct_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
    // Callback for music stream 
    virtual void audio_data_callback(const uint8_t *data, uint32_t len);
    // av event handler
    virtual void av_hdl_stack_evt(uint16_t event, void *p_param);
    // a2dp event handler 
    virtual void av_hdl_a2d_evt(uint16_t event, void *p_param);
    // avrc event handler 
    virtual void av_hdl_avrc_evt(uint16_t event, void *p_param);

    // split up long handlers
    virtual void handle_connection_state(uint16_t event, void *p_param);
    virtual void handle_audio_state(uint16_t event, void *p_param);
    virtual void handle_audio_cfg(uint16_t event, void *p_param);
    virtual void handle_avrc_connection_state(bool connected);


#ifdef ESP_IDF_4
    virtual void volume_set_by_local_host(uint8_t volume);
    virtual void volume_set_by_controller(uint8_t volume);
    virtual void av_notify_evt_handler(uint8_t event_id, esp_avrc_rn_param_t* event_parameter);
    virtual void app_rc_tg_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
    virtual void av_hdl_avrc_tg_evt(uint16_t event, void *p_param);
#else
    virtual void av_notify_evt_handler(uint8_t event_id, uint32_t event_parameter);
#endif    

#if A2DP_I2S_SUPPORT
    virtual void init_i2s();

    /// output audio data e.g. to i2s or to queue
    virtual size_t write_audio(const uint8_t *data, size_t size){
        return i2s_write_data(data, size);
    }

    /// writes the data to i2s
    size_t i2s_write_data(const uint8_t* data, size_t item_size);


    /// dummy functions needed for BluetoothA2DPSinkQueued
    virtual void i2s_task_handler(void *arg) {}
    virtual void bt_i2s_task_start_up(void) {}
    virtual void bt_i2s_task_shut_down(void) {}

#endif

    virtual esp_err_t esp_a2d_connect(esp_bd_addr_t peer) {
        return esp_a2d_sink_connect(peer);
    }
    void set_scan_mode_connectable_default() override {
        set_scan_mode_connectable(true);
    }

};



#ifdef __cplusplus
}
#endif


