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

#ifdef __cplusplus
extern "C" {
#endif

#include "BluetoothA2DPCommon.h"

#define APP_SIG_WORK_DISPATCH (0x01)

#ifndef AUTOCONNECT_TRY_NUM
#define AUTOCONNECT_TRY_NUM 1
#endif

#ifndef CURRENT_ESP_IDF
#define I2S_COMM_FORMAT_STAND_I2S (I2S_COMM_FORMAT_I2S |I2S_COMM_FORMAT_I2S_MSB)
#endif

#ifndef BT_AV_TAG
#define BT_AV_TAG               "BT_AV"
#endif

/* @brief event for handler "bt_av_hdl_stack_up */
enum {
    BT_APP_EVT_STACK_UP = 0,
};

/**
* @brief Public callback methods. We use a separate class so that we can protect the called methods in BluetoothA2DPSink
*/
class BluetoothA2DPSinkCallbacks {
  public:
    /// handle esp_a2d_cb_event_t 
    static void app_a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    /// handle esp_avrc_ct_cb_event_t
    static void app_rc_ct_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
	
#ifdef CURRENT_ESP_IDF

	 /// handle esp_avrc_tg_cb_event_t
    static void app_rc_tg_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
	/* avrc TG event handler */
	static void av_hdl_avrc_tg_evt(uint16_t event, void *p_param);

#endif	
	
    /// task handler
    static void app_task_handler(void *arg);
    /// Callback for music stream 
    static void audio_data_callback(const uint8_t *data, uint32_t len);
    /// av event handler
    static void av_hdl_stack_evt(uint16_t event, void *p_param);
    /// a2dp event handler 
    static void av_hdl_a2d_evt(uint16_t event, void *p_param);
    /// avrc event handler 
    static void av_hdl_avrc_evt(uint16_t event, void *p_param);


};

/**
 * @brief A2DP Bluethooth Sink - We initialize and start the Bluetooth A2DP Sink. 
 * The example https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/a2dp_sink
 * was refactered into a C++ class 
 */

class BluetoothA2DPSink {
  public: 
    /// Constructor
    BluetoothA2DPSink();
    /// Destructor - stops the playback and releases all resources
    ~BluetoothA2DPSink();
    /// Define the pins
    virtual void set_pin_config(i2s_pin_config_t pin_config);
   
    /// Define an alternative i2s port other then 0 
    virtual void set_i2s_port(i2s_port_t i2s_num);
   
    /// Define the i2s configuration
    virtual void set_i2s_config(i2s_config_t i2s_config);

    /// starts the I2S bluetooth sink with the inidicated name
    virtual void start(const char* name, bool auto_reconect=true);

    /// ends the I2S bluetooth sink with the indicated name - if you release the memory a future start is not possible
    virtual void end(bool release_memory=false);
    
    /// Disconnects the current a2d connection, allowing for another device to connect
    virtual void disconnect();

    /// Determine the actual audio state
    virtual esp_a2d_audio_state_t get_audio_state();
   
    /// Determine the connection state
    virtual esp_a2d_connection_state_t get_connection_state();

    /// Returns true if the state is connected
    virtual bool isConnected();

    /// Determine the actuall audio type
    virtual esp_a2d_mct_t get_audio_type();

    /// Define a callback method which provides the meta data
    virtual void set_avrc_metadata_callback(void (*callback)(uint8_t, const uint8_t*)) {
      this->avrc_metadata_callback = callback;
    }

    /// Define callback which is called when we receive data: This callback provides access to the data
    virtual void set_stream_reader(void (*callBack)(const uint8_t*, uint32_t), bool i2s_output=true);

    /// Define callback which is called when we receive data
    virtual void set_on_data_received(void (*callBack)());
	
	/// Set the callback that is called when the BT device is connected
    virtual void set_on_connected2BT(void (*callBack)());
	
	/// Set the callback that is called when the BT device is disconnected
    virtual void set_on_disconnected2BT(void (*callBack)());

	/// Set the callback that is called when they change the volume
    virtual void set_on_volumechange(void (*callBack)(int));

    /// Starts to play music using AVRC
    virtual void play();
    /// AVRC pause
    virtual void pause();
    /// AVRC stop
    virtual void stop();
    /// AVRC next
    virtual void next();
    /// AVRC previouse
    virtual void previous();
    /// mix stereo into single mono signal
    virtual void set_mono_downmix(bool enabled) { mono_downmix = enabled; }
    /// Defines the bits per sample for output (if > 16 output will be expanded)
    virtual void set_bits_per_sample(int bps) { i2s_config.bits_per_sample = (i2s_bits_per_sample_t) bps; }
    
    /// Provides the actually set data rate (in samples per second)
    uint16_t sample_rate();
	
	esp_err_t i2s_mclk_pin_select(const uint8_t pin);
	
    /// Changes the volume
	virtual void set_volume(uint8_t volume);
	
    /// Determines the volume
	virtual int get_volume();

    /// We need to confirm a new seesion by calling confirm_pin_code()
    virtual void activate_pin_code(bool active);

    /// confirms the connection request by returning the receivedn pin code
	virtual void confirm_pin_code();

    /// confirms the connection request by returning the indicated pin code
	virtual void confirm_pin_code(int code);


#ifdef CURRENT_ESP_IDF

    /// Bluetooth discoverability
    virtual void set_discoverability(esp_bt_discovery_mode_t d);
#endif		
	
    /// Make sure that BluetoothA2DPCallbacks can call protected event handlers
    friend BluetoothA2DPSinkCallbacks;
	
    
  protected:
    // protected data
    xQueueHandle app_task_queue;
    xTaskHandle app_task_handle;
    i2s_config_t i2s_config;
    i2s_pin_config_t pin_config;    
    const char * bt_name;
    uint32_t m_pkt_cnt = 0;
    esp_a2d_audio_state_t m_audio_state = ESP_A2D_AUDIO_STATE_STOPPED;
    const char *m_a2d_conn_state_str[4] = {"Disconnected", "Connecting", "Connected", "Disconnecting"};
    const char *m_a2d_audio_state_str[3] = {"Suspended", "Stopped", "Started"};
    esp_a2d_audio_state_t audio_state;
    esp_a2d_connection_state_t connection_state;
    esp_a2d_mct_t audio_type;
    char* pin_code = nullptr;
	
	void (*bt_volumechange)(int) = nullptr;
	void (*bt_disconnected)() = nullptr;
	void (*bt_connected)() = nullptr;
    void (*data_received)() = nullptr;
    void (*stream_reader)(const uint8_t*, uint32_t) = nullptr;
    void (*avrc_metadata_callback)(uint8_t, const uint8_t*) = nullptr;
    bool is_auto_reconnect;
    esp_bd_addr_t last_connection = {0,0,0,0,0,0};
    bool is_i2s_output = true;
    bool player_init = false;
    bool mono_downmix = false;


#ifdef CURRENT_ESP_IDF
    esp_bt_discovery_mode_t discoverability = ESP_BT_GENERAL_DISCOVERABLE;
#endif

    // protected methods
    virtual int init_bluetooth();
    virtual void app_task_start_up(void);
    virtual void app_task_shut_down(void);
    virtual bool app_send_msg(app_msg_t *msg);
    virtual bool app_work_dispatch(app_callback_t p_cback, uint16_t event, void *p_params, int param_len);
    virtual void app_work_dispatched(app_msg_t *msg);
    virtual void app_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param);
    virtual void av_new_track();
#ifdef CURRENT_ESP_IDF
	virtual void volume_set_by_local_host(uint8_t volume);
	virtual void volume_set_by_controller(uint8_t volume);
    virtual void av_notify_evt_handler(uint8_t& event_id, esp_avrc_rn_param_t& event_parameter);
	virtual void app_rc_tg_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
	virtual void av_hdl_avrc_tg_evt(uint16_t event, void *p_param);
#else
    virtual void av_notify_evt_handler(uint8_t event_id, uint32_t event_parameter);
#endif    
    
    virtual void init_nvs();
    virtual void get_last_connection();
    virtual void set_last_connection(esp_bd_addr_t bda, size_t size);
    virtual void clean_last_connection();

    // execute AVRC command
    virtual void execute_avrc_command(int cmd);

    /**
     * Wrappbed methods called from callbacks
     */
    virtual void app_a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
    virtual void app_rc_ct_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
	
    virtual void app_task_handler(void *arg);
    // Callback for music stream 
    virtual void audio_data_callback(const uint8_t *data, uint32_t len);
    // av event handler
    virtual void av_hdl_stack_evt(uint16_t event, void *p_param);
    // a2dp event handler 
    virtual void av_hdl_a2d_evt(uint16_t event, void *p_param);
    // avrc event handler 
    virtual void av_hdl_avrc_evt(uint16_t event, void *p_param);
	

	void connect_to_last_device();
    // change the scan mode
    void set_scan_mode_connectable(bool connectable);
    // check if last connectioin is defined
    bool has_last_connection();
	
		
};


#ifdef __cplusplus
}
#endif


