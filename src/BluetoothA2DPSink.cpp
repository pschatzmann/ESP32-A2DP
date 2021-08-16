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

#include "BluetoothA2DPSink.h"

/**
 * Some data that must be avaliable for C calls
 */
// to support static callback functions
BluetoothA2DPSink* actual_bluetooth_a2dp_sink;
i2s_port_t i2s_port; 
int connection_rety_count = 0;
esp_bd_addr_t peer_bd_addr = {0};
static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

static _lock_t s_volume_lock;
static uint8_t s_volume = 0;
static bool is_volume_used = false;
static bool s_volume_notify;
static int pin_code_int=0;
static bool is_pin_code_active = false;

// Forward declarations for C Callback functions for ESP32 Framework
extern "C" void app_task_handler_2(void *arg);
extern "C" void audio_data_callback_2(const uint8_t *data, uint32_t len);
extern "C" void app_a2d_callback_2(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
extern "C" void app_rc_ct_callback_2(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

#ifdef CURRENT_ESP_IDF
static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;
extern "C" void app_rc_tg_callback_2(esp_avrc_tg_cb_event_t  event, esp_avrc_tg_cb_param_t *param);
#endif

#define APP_RC_CT_TL_GET_CAPS            (0)


/**
 * Constructor
 */
BluetoothA2DPSink::BluetoothA2DPSink() {
  actual_bluetooth_a2dp_sink = this;
  if (is_i2s_output) {
        // default i2s port is 0
        i2s_port = (i2s_port_t) 0;

        // setup default i2s config
        i2s_config = {
            .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = 44100,
            .bits_per_sample = (i2s_bits_per_sample_t)16,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_STAND_I2S),
            .intr_alloc_flags = 0, // default interrupt priority
            .dma_buf_count = 8,
            .dma_buf_len = 64,
            .use_apll = false,
            .tx_desc_auto_clear = true // avoiding noise in case of data unavailability
        };

        // setup default pins
        pin_config = {
            .bck_io_num = 26,
            .ws_io_num = 25,
            .data_out_num = 22,
            .data_in_num = I2S_PIN_NO_CHANGE
        };
    }
}

BluetoothA2DPSink::~BluetoothA2DPSink() {
    if (app_task_queue!=NULL){
        end();
    }
}

void BluetoothA2DPSink::disconnect()
{
    ESP_LOGI(BT_AV_TAG, "discconect a2d");
    esp_err_t status = esp_a2d_sink_disconnect(last_connection);
    if (status == ESP_FAIL)
    {
        ESP_LOGE(BT_AV_TAG, "Failed disconnecting to device!");
    }
    // reconnect should not work after end
    clean_last_connection();
}

void BluetoothA2DPSink::end(bool release_memory) {
    // reconnect does not work after end
    clean_last_connection();

    ESP_LOGI(BT_AV_TAG,"deinit avrc");
    if (esp_avrc_ct_deinit() != ESP_OK){
         ESP_LOGE(BT_AV_TAG,"Failed to deinit avrc");
    }

    ESP_LOGI(BT_AV_TAG,"disable bluetooth");
    if (esp_bluedroid_disable() != ESP_OK){
        ESP_LOGE(BT_AV_TAG,"Failed to disable bluetooth");
    }
    
    ESP_LOGI(BT_AV_TAG,"deinit bluetooth");
    if (esp_bluedroid_deinit() != ESP_OK){
        ESP_LOGE(BT_AV_TAG,"Failed to deinit bluetooth");
    }

    if (is_i2s_output){
        ESP_LOGI(BT_AV_TAG,"uninstall i2s");
        if (i2s_driver_uninstall(i2s_port) != ESP_OK){
            ESP_LOGE(BT_AV_TAG,"Failed to uninstall i2s");
        }
        else {
            player_init = false;
        }
    }

    ESP_LOGI(BT_AV_TAG,"esp_bt_controller_disable");
    if (esp_bt_controller_disable()!=ESP_OK){
     	ESP_LOGE(BT_AV_TAG,"esp_bt_controller_disable failed");
    }

    // waiting for status change
    while(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);

    if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED){
        ESP_LOGI(BT_AV_TAG,"esp_bt_controller_deinit");
        if (esp_bt_controller_deinit()!= ESP_OK){
            ESP_LOGE(BT_AV_TAG,"esp_bt_controller_deinit failed");
        }
    }
    
    // after a release memory - a restart will not be possible
    if (release_memory) {
        ESP_LOGI(BT_AV_TAG,"esp_bt_controller_mem_release");
        if (esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)!= ESP_OK){
            ESP_LOGE(BT_AV_TAG,"esp_bt_controller_mem_release failed");
        }
    }

    // shutdown tasks
    app_task_shut_down();
}


void BluetoothA2DPSink::set_pin_config(i2s_pin_config_t pin_config){
  this->pin_config = pin_config;
}

void BluetoothA2DPSink::set_i2s_port(i2s_port_t i2s_num) {
  i2s_port = i2s_num;
}

void BluetoothA2DPSink::set_i2s_config(i2s_config_t i2s_config){
  this->i2s_config = i2s_config;
}

void BluetoothA2DPSink::set_stream_reader(void (*callBack)(const uint8_t*, uint32_t), bool is_i2s){
  this->stream_reader = callBack;
  this->is_i2s_output = is_i2s;
}

void BluetoothA2DPSink::set_on_data_received(void (*callBack)()){
  this->data_received = callBack;
}

void BluetoothA2DPSink::set_on_connected2BT(void (*callBack)()){
  this->bt_connected = callBack;
}

void BluetoothA2DPSink::set_on_disconnected2BT(void (*callBack)()){
  this->bt_disconnected = callBack;
}

void BluetoothA2DPSink::set_on_volumechange(void (*callBack)(int)){
  this->bt_volumechange = callBack;
}

/** 
 * Main function to start the Bluetooth Processing
 */
void BluetoothA2DPSink::start(const char* name, bool auto_reconnect)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    //store parameters
    if (name) {
      this->bt_name = name;
    }
    ESP_LOGI(BT_AV_TAG,"Device name will be set to '%s'",this->bt_name);
    
	// Initialize NVS
    is_auto_reconnect = auto_reconnect;
	init_nvs();
    if (is_auto_reconnect){
	    get_last_connection();
	}

    // setup bluetooth
    init_bluetooth();
    
    // create application task 
    app_task_start_up();

    //Lambda for callback
    auto av_hdl_stack_evt_2 = [](uint16_t event, void *p_param) {
        ESP_LOGD(BT_AV_TAG, "av_hdl_stack_evt_2");
        if (actual_bluetooth_a2dp_sink) {
            actual_bluetooth_a2dp_sink->av_hdl_stack_evt(event,p_param);
        }
    };

    // Bluetooth device name, connection mode and profile set up 
    app_work_dispatch(av_hdl_stack_evt_2, BT_APP_EVT_STACK_UP, NULL, 0);

    if (is_i2s_output){
        // setup i2s
        if (i2s_driver_install(i2s_port, &i2s_config, 0, NULL) != ESP_OK) {
            ESP_LOGE(BT_AV_TAG,"i2s_driver_install failed");
        } else {
	    player_init = false; //reset player
	}

        // pins are only relevant when music is not sent to internal DAC
        if (i2s_config.mode & I2S_MODE_DAC_BUILT_IN) {
            ESP_LOGI(BT_AV_TAG, "Output will go to DAC pins");
            i2s_set_pin(i2s_port, NULL);      
        } else {
            i2s_set_pin(i2s_port, &pin_config);
        }
    }
	
    if (is_pin_code_active) {
        /* Set default parameters for Secure Simple Pairing */
        esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
        esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
        esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

        esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
        esp_bt_pin_code_t pin_code;
        esp_bt_gap_set_pin(pin_type, 0, pin_code);

    } else {
        /* Set default parameters for Secure Simple Pairing */
        esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
        esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
        esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

        esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
        esp_bt_pin_code_t pin_code;
        esp_bt_gap_set_pin(pin_type, 0, pin_code);

    }
    
	/*
     * Set default parameters for Legacy Pairing
     * ESP_BT_PIN_TYPE_VARIABLE will trigger callbacks - pin_code and len is ignored
     */

	
}

esp_err_t BluetoothA2DPSink::i2s_mclk_pin_select(const uint8_t pin) {
    if(pin != 0 && pin != 1 && pin != 3) {
        ESP_LOGE(BT_APP_TAG, "Only support GPIO0/GPIO1/GPIO3, gpio_num:%d", pin);
        return ESP_ERR_INVALID_ARG;
    }
    switch(pin){
        case 0:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
            WRITE_PERI_REG(PIN_CTRL, 0xFFF0);
            break;
        case 1:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD_CLK_OUT3);
            WRITE_PERI_REG(PIN_CTRL, 0xF0F0);
            break;
        case 3:
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD_CLK_OUT2);
            WRITE_PERI_REG(PIN_CTRL, 0xFF00);
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_a2d_audio_state_t BluetoothA2DPSink::get_audio_state() {
  return audio_state;
}

esp_a2d_connection_state_t BluetoothA2DPSink::get_connection_state() {
    return connection_state;
}

bool BluetoothA2DPSink::isConnected() {
    return connection_state == ESP_A2D_CONNECTION_STATE_CONNECTED;
}

esp_a2d_mct_t BluetoothA2DPSink::get_audio_type() {
    return audio_type;
}

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
		case ESP_BT_GAP_AUTH_CMPL_EVT: {
			if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
				ESP_LOGI(BT_AV_TAG, "authentication success: %s", param->auth_cmpl.device_name);
			  //  esp_log_buffer_hex(BT_AV_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
			} else {
				ESP_LOGE(BT_AV_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
			}
			break;
		}
		
		case ESP_BT_GAP_CFM_REQ_EVT: {
                ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please confirm the passkey: %d", param->cfm_req.num_val);
                pin_code_int = param->key_notif.passkey;
            }
			break;

		case ESP_BT_GAP_KEY_NOTIF_EVT: {
                ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
                pin_code_int = param->key_notif.passkey;
            }
			break;

		case ESP_BT_GAP_KEY_REQ_EVT: {
                ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
                memcpy(peer_bd_addr, param->cfm_req.bda, ESP_BD_ADDR_LEN);
			} 
            break;


		default: {
			ESP_LOGI(BT_AV_TAG, "event: %d", event);
			break;
		}
    }
    return;
}

int BluetoothA2DPSink::init_bluetooth()
{
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (!btStart()) {
    ESP_LOGE(BT_AV_TAG,"Failed to initialize controller");
    return false;
  }
  ESP_LOGI(BT_AV_TAG,"controller initialized");

  esp_bluedroid_status_t bt_stack_status = esp_bluedroid_get_status();

  if(bt_stack_status == ESP_BLUEDROID_STATUS_UNINITIALIZED){
    if (esp_bluedroid_init() != ESP_OK) {
        ESP_LOGE(BT_AV_TAG,"Failed to initialize bluedroid");
        return false;
    }
    ESP_LOGI(BT_AV_TAG,"bluedroid initialized");
  }
 
  if(bt_stack_status != ESP_BLUEDROID_STATUS_ENABLED){
    if (esp_bluedroid_enable() != ESP_OK) {
        ESP_LOGE(BT_AV_TAG,"Failed to enable bluedroid");
        return false;
    }
    ESP_LOGI(BT_AV_TAG,"bluedroid enabled"); 
  }
  
  if (esp_bt_gap_register_callback(bt_app_gap_cb) != ESP_OK) {
        ESP_LOGE(BT_AV_TAG,"gap register failed");
        return false;
    }
	
   if ((esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(BT_AV_TAG,"esp_spp_init failed");
        return false;
    }
  
  
  return true;
}



bool BluetoothA2DPSink::app_work_dispatch(app_callback_t p_cback, uint16_t event, void *p_params, int param_len)
{
    ESP_LOGD(BT_APP_TAG, "%s event 0x%x, param len %d", __func__, event, param_len);
    
    app_msg_t msg;
    memset(&msg, 0, sizeof(app_msg_t));

    msg.sig = APP_SIG_WORK_DISPATCH;
    msg.event = event;
    msg.cb = p_cback;

    if (param_len == 0) {
        return app_send_msg(&msg);
    } else if (p_params && param_len > 0) {
        if ((msg.param = malloc(param_len)) != NULL) {
            memcpy(msg.param, p_params, param_len);
            return app_send_msg(&msg);
        }
    }

    return false;
}

void BluetoothA2DPSink::app_work_dispatched(app_msg_t *msg)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    if (msg->cb) {
        msg->cb(msg->event, msg->param);
    }
}


bool BluetoothA2DPSink::app_send_msg(app_msg_t *msg)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    if (msg == NULL || app_task_queue == NULL) {
        ESP_LOGE(BT_APP_TAG, "%s app_send_msg failed", __func__);
        return false;
    }

    if (xQueueSend(app_task_queue, msg, 10 / portTICK_RATE_MS) != pdTRUE) {
        ESP_LOGE(BT_APP_TAG, "%s xQueue send failed", __func__);
        return false;
    }
    return true;
}


void BluetoothA2DPSink::app_task_handler(void *arg)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    app_msg_t msg;
    while (true) {
        if (!app_task_queue){
            ESP_LOGE(BT_APP_TAG, "%s, app_task_queue is null", __func__);
            delay(100);
        } else if (pdTRUE == xQueueReceive(app_task_queue, &msg, (portTickType)portMAX_DELAY)) {
            ESP_LOGD(BT_APP_TAG, "%s, sig 0x%x, 0x%x", __func__, msg.sig, msg.event);
            switch (msg.sig) {
            case APP_SIG_WORK_DISPATCH:
                ESP_LOGW(BT_APP_TAG, "%s, APP_SIG_WORK_DISPATCH sig: %d", __func__, msg.sig);
                app_work_dispatched(&msg);
                break;
            default:
                ESP_LOGW(BT_APP_TAG, "%s, unhandled sig: %d", __func__, msg.sig);
                break;
            } // switch (msg.sig)

            if (msg.param) {
                free(msg.param);
            }
        }
    }
}

void BluetoothA2DPSink::app_task_start_up(void)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    app_task_queue = xQueueCreate(10, sizeof(app_msg_t));
    if (xTaskCreate(app_task_handler_2, "BtAppT", 2048, NULL, configMAX_PRIORITIES - 3, &app_task_handle) != pdPASS){
        ESP_LOGE(BT_APP_TAG, "%s failed", __func__);
    }
}

void BluetoothA2DPSink::app_task_shut_down(void)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    if (app_task_handle!=NULL) {
        vTaskDelete(app_task_handle);
        app_task_handle = NULL;
    }
    if (app_task_queue!=NULL) {
        vQueueDelete(app_task_queue);
        app_task_queue = NULL;
    }
}


void BluetoothA2DPSink::app_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(param);
    uint8_t *attr_text = (uint8_t *) malloc (rc->meta_rsp.attr_length + 1);
    memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);
    attr_text[rc->meta_rsp.attr_length] = 0;

    rc->meta_rsp.attr_text = attr_text;
}



void BluetoothA2DPSink::app_rc_ct_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);

    // lambda for callback
    auto av_hdl_avrc_evt_2 = [](uint16_t event, void *p_param){
        ESP_LOGD(BT_AV_TAG, "av_hdl_avrc_evt_2");
        if (actual_bluetooth_a2dp_sink) {
            actual_bluetooth_a2dp_sink->av_hdl_avrc_evt(event,p_param);    
        }
    };

    switch (event) {
        case ESP_AVRC_CT_METADATA_RSP_EVT:
            ESP_LOGD(BT_AV_TAG, "%s ESP_AVRC_CT_METADATA_RSP_EVT", __func__);
            app_alloc_meta_buffer(param);
            app_work_dispatch(av_hdl_avrc_evt_2, event, param, sizeof(esp_avrc_ct_cb_param_t));
            break;
        case ESP_AVRC_CT_CONNECTION_STATE_EVT:
            ESP_LOGD(BT_AV_TAG, "%s ESP_AVRC_CT_CONNECTION_STATE_EVT", __func__);
            app_work_dispatch(av_hdl_avrc_evt_2, event, param, sizeof(esp_avrc_ct_cb_param_t));
            break;
        case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
            ESP_LOGD(BT_AV_TAG, "%s ESP_AVRC_CT_PASSTHROUGH_RSP_EVT", __func__);
            app_work_dispatch(av_hdl_avrc_evt_2, event, param, sizeof(esp_avrc_ct_cb_param_t));
            break;
        case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
            ESP_LOGD(BT_AV_TAG, "%s ESP_AVRC_CT_CHANGE_NOTIFY_EVT", __func__);
            app_work_dispatch(av_hdl_avrc_evt_2, event, param, sizeof(esp_avrc_ct_cb_param_t));
            break;
        case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
            ESP_LOGD(BT_AV_TAG, "%s ESP_AVRC_CT_REMOTE_FEATURES_EVT", __func__);
            app_work_dispatch(av_hdl_avrc_evt_2, event, param, sizeof(esp_avrc_ct_cb_param_t));
            break;
        }

#ifdef CURRENT_ESP_IDF

		case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
			ESP_LOGD(BT_AV_TAG, "%s ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT", __func__);
			app_work_dispatch(av_hdl_avrc_evt_2, event, param, sizeof(esp_avrc_ct_cb_param_t));
			break;
		}
#endif

        default:
            ESP_LOGE(BT_AV_TAG, "Invalid AVRC event: %d", event);
            break;
        }
}


void  BluetoothA2DPSink::av_hdl_a2d_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    esp_a2d_cb_param_t *a2d = NULL;
    switch (event) {
        case ESP_A2D_CONNECTION_STATE_EVT: {
            ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_CONNECTION_STATE_EVT", __func__);
            a2d = (esp_a2d_cb_param_t *)(p_param);
            uint8_t *bda = a2d->conn_stat.remote_bda;
            connection_state = a2d->conn_stat.state;
            ESP_LOGI(BT_AV_TAG, "A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
            m_a2d_conn_state_str[a2d->conn_stat.state], bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

            if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
                ESP_LOGI(BT_AV_TAG, "ESP_A2D_CONNECTION_STATE_DISCONNECTED");
				
				if (bt_disconnected!=nullptr){
					(*bt_disconnected)();
				}	
				
                if (is_i2s_output) {
                    i2s_stop(i2s_port);
                    i2s_zero_dma_buffer(i2s_port);
                }
                if (is_auto_reconnect && has_last_connection()) {
                    if ( has_last_connection()  && connection_rety_count < AUTOCONNECT_TRY_NUM ){
                        ESP_LOGI(BT_AV_TAG,"Connection try number: %d", connection_rety_count);
                        connect_to_last_device();
                    } else {
                        if ( has_last_connection() && a2d->conn_stat.disc_rsn == ESP_A2D_DISC_RSN_NORMAL ){
                            clean_last_connection();
                        }
                        set_scan_mode_connectable(true);
                    }
                } else {
                    set_scan_mode_connectable(true);   
                }
            } else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED){
                ESP_LOGI(BT_AV_TAG, "ESP_A2D_CONNECTION_STATE_CONNECTED");
				
				if (bt_connected!=nullptr){
					(*bt_connected)();
				}
				
				
                set_scan_mode_connectable(false);   
                connection_rety_count = 0;
                if (is_i2s_output) i2s_start(i2s_port);
                // record current connection
                if (is_auto_reconnect) {
                    set_last_connection(a2d->conn_stat.remote_bda, sizeof(a2d->conn_stat.remote_bda));
                }
            } else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTING){
                ESP_LOGI(BT_AV_TAG, "ESP_A2D_CONNECTION_STATE_CONNECTING");
                connection_rety_count++;
            }
            
            break;
        }
        case ESP_A2D_AUDIO_STATE_EVT: {
            ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_AUDIO_STATE_EVT", __func__);
            a2d = (esp_a2d_cb_param_t *)(p_param);
            ESP_LOGI(BT_AV_TAG, "A2DP audio state: %s", m_a2d_audio_state_str[a2d->audio_stat.state]);
            m_audio_state = a2d->audio_stat.state;
            if (is_i2s_output){
                if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) { 
                    m_pkt_cnt = 0; 
                    i2s_start(i2s_port); 
                } else if ( ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND == a2d->audio_stat.state 
                        || ESP_A2D_AUDIO_STATE_STOPPED == a2d->audio_stat.state ) { 
                    i2s_stop(i2s_port);
                    i2s_zero_dma_buffer(i2s_port);
                }
            }
            break;
        }
        case ESP_A2D_AUDIO_CFG_EVT: {
            ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_AUDIO_CFG_EVT", __func__);
            esp_a2d_cb_param_t *esp_a2d_callback_param = (esp_a2d_cb_param_t *)(p_param);
            audio_type = esp_a2d_callback_param->audio_cfg.mcc.type;
            a2d = (esp_a2d_cb_param_t *)(p_param);
            ESP_LOGI(BT_AV_TAG, "a2dp audio_cfg_cb , codec type %d", a2d->audio_cfg.mcc.type);

            // determine sample rate
            i2s_config.sample_rate = 16000;
            char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
            if (oct0 & (0x01 << 6)) {
                i2s_config.sample_rate = 32000;
            } else if (oct0 & (0x01 << 5)) {
                i2s_config.sample_rate = 44100;
            } else if (oct0 & (0x01 << 4)) {
                i2s_config.sample_rate = 48000;
            }
            ESP_LOGI(BT_AV_TAG, "a2dp audio_cfg_cb , sample_rate %d", i2s_config.sample_rate );

            // for now only SBC stream is supported
            if (player_init == false && is_i2s_output && a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
                
                i2s_set_clk(i2s_port, i2s_config.sample_rate, i2s_config.bits_per_sample, (i2s_channel_t)2);

                ESP_LOGI(BT_AV_TAG, "configure audio player %x-%x-%x-%x\n",
                        a2d->audio_cfg.mcc.cie.sbc[0],
                        a2d->audio_cfg.mcc.cie.sbc[1],
                        a2d->audio_cfg.mcc.cie.sbc[2],
                        a2d->audio_cfg.mcc.cie.sbc[3]);
                ESP_LOGI(BT_AV_TAG, "audio player configured, samplerate=%d", i2s_config.sample_rate);
		player_init = true; //init finished
            }
            break;
        }

#ifdef CURRENT_ESP_IDF

		case ESP_A2D_PROF_STATE_EVT: {
			a2d = (esp_a2d_cb_param_t *)(p_param);
			if (ESP_A2D_INIT_SUCCESS == a2d->a2d_prof_stat.init_state) {
				ESP_LOGI(BT_AV_TAG,"A2DP PROF STATE: Init Compl\n");
			} else {
				ESP_LOGI(BT_AV_TAG,"A2DP PROF STATE: Deinit Compl\n");
			}
			break;
		}

#endif

	    default:
            ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
            break;
    }
}

uint16_t BluetoothA2DPSink::sample_rate(){
    return i2s_config.sample_rate;
}


void BluetoothA2DPSink::av_new_track()
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    //Register notifications and request metadata
    esp_avrc_ct_send_metadata_cmd(0, ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_TRACK_NUM | ESP_AVRC_MD_ATTR_NUM_TRACKS | ESP_AVRC_MD_ATTR_GENRE);
    esp_avrc_ct_send_register_notification_cmd(1, ESP_AVRC_RN_TRACK_CHANGE, 0);
}

#ifdef CURRENT_ESP_IDF
void BluetoothA2DPSink::av_notify_evt_handler(uint8_t& event_id, esp_avrc_rn_param_t& event_parameter)
#else
void BluetoothA2DPSink::av_notify_evt_handler(uint8_t event_id, uint32_t event_parameter)
#endif
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    switch (event_id) {
    case ESP_AVRC_RN_TRACK_CHANGE:
        ESP_LOGD(BT_AV_TAG, "%s ESP_AVRC_RN_TRACK_CHANGE %d", __func__, event_id);
        av_new_track();
        break;
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event_id);
        break;
    }
}

void BluetoothA2DPSink::av_hdl_avrc_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
        uint8_t *bda = rc->conn_stat.remote_bda;
        ESP_LOGI(BT_AV_TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

#ifdef CURRENT_ESP_IDF

        if (rc->conn_stat.connected) {
            av_new_track();
			 // get remote supported event_ids of peer AVRCP Target
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        } else {
			// clear peer notification capability record
            s_avrc_peer_rn_cap.bits = 0;
		}		
        break;
#endif

    }
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC passthrough rsp: key_code 0x%x, key_state %d", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
        break;
    }
    case ESP_AVRC_CT_METADATA_RSP_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC metadata rsp: attribute id 0x%x, %s", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
        // call metadata callback if available
        if (avrc_metadata_callback != nullptr){
            avrc_metadata_callback(rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
        }

        free(rc->meta_rsp.attr_text);
        break;
    }
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT: {
        //ESP_LOGI(BT_AV_TAG, "AVRC event notification: %d, param: %d", (int)rc->change_ntf.event_id, (int)rc->change_ntf.event_parameter);
        av_notify_evt_handler(rc->change_ntf.event_id, rc->change_ntf.event_parameter);
        break;
    }
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC remote features %x", rc->rmt_feats.feat_mask);
        break;
    }

#ifdef CURRENT_ESP_IDF

	case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT: {
        ESP_LOGI(BT_AV_TAG, "remote rn_cap: count %d, bitmask 0x%x", rc->get_rn_caps_rsp.cap_count,
                 rc->get_rn_caps_rsp.evt_set.bits);
        s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;
        av_new_track();
        //bt_av_playback_changed();
        //bt_av_play_pos_changed();
        break;
    }

#endif

    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}


void BluetoothA2DPSink::av_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    esp_err_t result;

    switch (event) {
        case BT_APP_EVT_STACK_UP: {
            ESP_LOGD(BT_AV_TAG, "%s av_hdl_stack_evt %s", __func__, "BT_APP_EVT_STACK_UP");
            /* set up device name */
            esp_bt_dev_set_device_name(bt_name);
			
			

            // initialize AVRCP controller 
            result = esp_avrc_ct_init();
            if (result == ESP_OK){
                result = esp_avrc_ct_register_callback(app_rc_ct_callback_2);
                if (result == ESP_OK){
                    ESP_LOGD(BT_AV_TAG, "AVRCP controller initialized!");
                } else {
                    ESP_LOGE(BT_AV_TAG,"esp_avrc_ct_register_callback: %d",result);
                }
            } else {
                ESP_LOGE(BT_AV_TAG,"esp_avrc_ct_init: %d",result);
            }
			
#ifdef CURRENT_ESP_IDF
			
			/* initialize AVRCP target */
			assert (esp_avrc_tg_init() == ESP_OK);
			esp_avrc_tg_register_callback(app_rc_tg_callback_2);


			esp_avrc_rn_evt_cap_mask_t evt_set = {0};
			esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
			assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);
#endif
			
		
            /* initialize A2DP sink */
            if (esp_a2d_register_callback(app_a2d_callback_2)!=ESP_OK){
                ESP_LOGE(BT_AV_TAG,"esp_a2d_register_callback");
            }
            if (esp_a2d_sink_register_data_callback(audio_data_callback_2)!=ESP_OK){
                ESP_LOGE(BT_AV_TAG,"esp_a2d_sink_register_data_callback");
            }
            if (esp_a2d_sink_init()!=ESP_OK){
                ESP_LOGE(BT_AV_TAG,"esp_a2d_sink_init");            
            }
            if (is_auto_reconnect && has_last_connection() ) {
                ESP_LOGD(BT_AV_TAG, "connect_to_last_device");
                connect_to_last_device();
            }

            /* set discoverable and connectable mode, wait to be connected */
            ESP_LOGD(BT_AV_TAG, "esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE)");
            set_scan_mode_connectable(true);

            break;
        }

        default:
            ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
            break;

    }
}


/* callback for A2DP sink */
void BluetoothA2DPSink::app_a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);

    // lambda for callback
    auto av_hdl_a2d_evt_2=[](uint16_t event, void *p_param){
        ESP_LOGD(BT_AV_TAG, "av_hdl_a2d_evt_2");
        if (actual_bluetooth_a2dp_sink) {
            actual_bluetooth_a2dp_sink->av_hdl_a2d_evt(event,p_param);  
        }
    };

    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT:
        ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_CONNECTION_STATE_EVT", __func__);
        app_work_dispatch(av_hdl_a2d_evt_2, event, param, sizeof(esp_a2d_cb_param_t));
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_AUDIO_STATE_EVT", __func__);
        audio_state = param->audio_stat.state;
        app_work_dispatch(av_hdl_a2d_evt_2,event, param, sizeof(esp_a2d_cb_param_t));
        break;
    case ESP_A2D_AUDIO_CFG_EVT: {
        ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_AUDIO_CFG_EVT", __func__);
        app_work_dispatch(av_hdl_a2d_evt_2, event, param, sizeof(esp_a2d_cb_param_t));
        break;
    }
	
#ifdef CURRENT_ESP_IDF
	case ESP_A2D_PROF_STATE_EVT: {
		ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_AUDIO_CFG_EVT", __func__);
        app_work_dispatch(av_hdl_a2d_evt_2, event, param, sizeof(esp_a2d_cb_param_t));
        break;
    }
#endif	
	
    default:
        ESP_LOGE(BT_AV_TAG, "Invalid A2DP event: %d", event);
        break;
    }
}

void BluetoothA2DPSink::audio_data_callback(const uint8_t *data, uint32_t len) {
    ESP_LOGD(BT_AV_TAG, "%s", __func__);

    if (mono_downmix) {
        uint8_t* corr_data = (uint8_t*) data;
        for (int i=0; i<len/4; i++) {
            int16_t pcmLeft = ((uint16_t)data[i*4 + 1] << 8) | data[i*4];
            int16_t pcmRight = ((uint16_t)data[i*4 + 3] << 8) | data[i*4 + 2];
            int16_t mono = ((int32_t)pcmLeft + pcmRight) >> 1;
            corr_data[i*4+1] = mono >> 8;
            corr_data[i*4] = mono;
            corr_data[i*4+3] = mono >> 8;
            corr_data[i*4+2] = mono;
        }
    }
    
    if (stream_reader!=nullptr){
        ESP_LOGD(BT_AV_TAG, "stream_reader");
 	    (*stream_reader)(data, len);
    }

    if (is_i2s_output) {
        // special case for internal DAC output, the incomming PCM buffer needs 
        // to be converted from signed 16bit to unsigned
        if (this->i2s_config.mode & I2S_MODE_DAC_BUILT_IN) {
    
            //HACK: this is here to remove the const restriction to replace the data in place as per
            //https://github.com/espressif/esp-idf/blob/178b122/components/bt/host/bluedroid/api/include/api/esp_a2dp_api.h
            //the buffer is anyway static block of memory possibly overwritten by next incomming data.

            uint16_t* corr_data = (uint16_t*) data;
            for (int i=0; i<len/2; i++) {
                int16_t sample = data[i*2] | data[i*2+1]<<8;
                corr_data[i]= sample + 0x8000;
            }
        }
		

        // adjust volume if necessary
        if (is_volume_used){
            int16_t * pcmdata = (int16_t *)data;
            for (int i=0; i<len/2; i++) {
                int32_t temp = (int32_t)(*pcmdata);
                temp = temp * s_volume;
                temp = temp/512;
                //*pcmdata = ((*pcmdata)*s_volume)/127;
                *pcmdata = (int16_t)temp;
                pcmdata++;
            }
        }
		

        size_t i2s_bytes_written;
        if (i2s_config.bits_per_sample==I2S_BITS_PER_SAMPLE_16BIT){
            // standard logic with 16 bits
            if (i2s_write(i2s_port,(void*) data, len, &i2s_bytes_written, portMAX_DELAY)!=ESP_OK){
                ESP_LOGE(BT_AV_TAG, "i2s_write has failed");    
            }
        } else {
            if (i2s_config.bits_per_sample>16){
                // expand e.g to 32 bit for dacs which do not support 16 bits
                if (i2s_write_expand(i2s_port,(void*) data, len, I2S_BITS_PER_SAMPLE_16BIT, i2s_config.bits_per_sample, &i2s_bytes_written, portMAX_DELAY) != ESP_OK){
                    ESP_LOGE(BT_AV_TAG, "i2s_write has failed");    
                }
            } else {
                ESP_LOGE(BT_AV_TAG, "invalid bits_per_sample: %d", i2s_config.bits_per_sample);    
            }
        }

        if (i2s_bytes_written<len){
            ESP_LOGE(BT_AV_TAG, "Timeout: not all bytes were written to I2S");
        }
    }

    if (data_received!=nullptr){
        ESP_LOGD(BT_AV_TAG, "data_received");
   	    (*data_received)();
    }
}

void BluetoothA2DPSink::init_nvs(){
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
	}
    ESP_ERROR_CHECK( err );
}

bool BluetoothA2DPSink::has_last_connection() {  
    esp_bd_addr_t empty_connection = {0,0,0,0,0,0};
    int result = memcmp(last_connection, empty_connection, ESP_BD_ADDR_LEN);
    return result!=0;
}

void BluetoothA2DPSink::get_last_connection(){
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    nvs_handle my_handle;
    esp_err_t err;
    
    err = nvs_open("connected_bda", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
         ESP_LOGE(BT_AV_TAG,"NVS OPEN ERROR");
    }

    esp_bd_addr_t bda;
    size_t size = sizeof(bda);
    err = nvs_get_blob(my_handle, "last_bda", bda, &size);
    if ( err != ESP_OK) { 
        ESP_LOGE(BT_AV_TAG, "ERROR GETTING NVS BLOB");
    }
    if ( err == ESP_ERR_NVS_NOT_FOUND ) {
        ESP_LOGE(BT_AV_TAG, "NVS NOT FOUND");
    }
    nvs_close(my_handle);
    if (err == ESP_OK) {
        memcpy(last_connection,bda,size);
    } 
}

void BluetoothA2DPSink::set_last_connection(esp_bd_addr_t bda, size_t size){
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
	if ( memcmp(bda, last_connection, size) == 0 ) return; //same value, nothing to store
	nvs_handle my_handle;
	esp_err_t err;
	
	err = nvs_open("connected_bda", NVS_READWRITE, &my_handle);
	if (err != ESP_OK){
         ESP_LOGE(BT_AV_TAG, "NVS OPEN ERROR");
    }
	err = nvs_set_blob(my_handle, "last_bda", bda, size);
	if (err == ESP_OK) {
        err = nvs_commit(my_handle);
    } else {
        ESP_LOGE(BT_AV_TAG, "NVS WRITE ERROR");
    }
	if (err != ESP_OK) {
        ESP_LOGE(BT_AV_TAG, "NVS COMMIT ERROR");
    }
	nvs_close(my_handle);
	memcpy(last_connection,bda,size);
}

void BluetoothA2DPSink::clean_last_connection() {
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    esp_bd_addr_t cleanBda = { 0 };
    set_last_connection(cleanBda, sizeof(cleanBda));
}

void BluetoothA2DPSink::connect_to_last_device(){
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
	esp_err_t status = esp_a2d_sink_connect(last_connection);
	if ( status == ESP_FAIL ){
        ESP_LOGE(BT_AV_TAG,"Failed connecting to device!");
    } 
}

void BluetoothA2DPSink::execute_avrc_command(int cmd){
    ESP_LOGD(BT_AV_TAG, "execute_avrc_command: %d",cmd);    
    esp_err_t ok = esp_avrc_ct_send_passthrough_cmd(0, cmd, ESP_AVRC_PT_CMD_STATE_PRESSED);
    if (ok==ESP_OK){
        delay(100);
        ok = esp_avrc_ct_send_passthrough_cmd(0, cmd, ESP_AVRC_PT_CMD_STATE_RELEASED);
        if (ok==ESP_OK){
            ESP_LOGD(BT_AV_TAG, "execute_avrc_command: %d -> OK", cmd);    
        } else {
            ESP_LOGE(BT_AV_TAG,"execute_avrc_command ESP_AVRC_PT_CMD_STATE_PRESSED FAILED: %d",ok);
        }
    } else {
        ESP_LOGE(BT_AV_TAG,"execute_avrc_command ESP_AVRC_PT_CMD_STATE_RELEASED FAILED: %d",ok);
    }
}

void BluetoothA2DPSink::play(){
    execute_avrc_command(ESP_AVRC_PT_CMD_PLAY);
}

void BluetoothA2DPSink::pause(){
    execute_avrc_command(ESP_AVRC_PT_CMD_PAUSE);
}

void BluetoothA2DPSink::stop(){
    execute_avrc_command(ESP_AVRC_PT_CMD_STOP);
}

void BluetoothA2DPSink::next(){
    execute_avrc_command(ESP_AVRC_PT_CMD_FORWARD);
}
void BluetoothA2DPSink::previous(){
    execute_avrc_command(ESP_AVRC_PT_CMD_BACKWARD);
}

/**
 * public Callbacks 
 * 
 */
void BluetoothA2DPSinkCallbacks::app_task_handler(void *arg) {
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actual_bluetooth_a2dp_sink)
    actual_bluetooth_a2dp_sink->app_task_handler(arg);
}

void BluetoothA2DPSinkCallbacks::audio_data_callback(const uint8_t *data, uint32_t len) {
  //ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actual_bluetooth_a2dp_sink)
    actual_bluetooth_a2dp_sink->audio_data_callback(data,len);
}

void BluetoothA2DPSinkCallbacks::app_a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param){
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actual_bluetooth_a2dp_sink)
    actual_bluetooth_a2dp_sink->app_a2d_callback(event, param);
}

void BluetoothA2DPSinkCallbacks::app_rc_ct_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param){
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actual_bluetooth_a2dp_sink)
    actual_bluetooth_a2dp_sink->app_rc_ct_callback(event, param);
}

/**
 * C Callback Functions needed for the ESP32 API
 */
extern "C" void app_task_handler_2(void *arg) {
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    BluetoothA2DPSinkCallbacks::app_task_handler(arg);
}

extern "C" void audio_data_callback_2(const uint8_t *data, uint32_t len) {
    //ESP_LOGD(BT_AV_TAG, "%s", __func__);
    BluetoothA2DPSinkCallbacks::audio_data_callback(data,len);
}

extern "C" void app_a2d_callback_2(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param){
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    BluetoothA2DPSinkCallbacks::app_a2d_callback(event, param);
}

extern "C" void app_rc_ct_callback_2(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param){
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    BluetoothA2DPSinkCallbacks::app_rc_ct_callback(event, param);
}

void BluetoothA2DPSink::set_volume(uint8_t volume)
{
  ESP_LOGI(BT_AV_TAG, "set_volume %d", volume);
  is_volume_used = true;
  s_volume =  (volume) & 0x7f;

#ifdef CURRENT_ESP_IDF
  volume_set_by_local_host(s_volume);
#endif

}

int BluetoothA2DPSink::get_volume()
{
  // ESP_LOGI(BT_AV_TAG, "get_volume %d", s_volume);
  return ((s_volume)* 100/ 0x7f);
}

void BluetoothA2DPSink::activate_pin_code(bool active){
    is_pin_code_active = active;
}


void BluetoothA2DPSink::confirm_pin_code()
{
  ESP_LOGI(BT_AV_TAG, "confirm_pin_code %s", pin_code_int);
  esp_bt_gap_ssp_passkey_reply(peer_bd_addr, true, pin_code_int);
}

void BluetoothA2DPSink::confirm_pin_code(int code)
{
  ESP_LOGI(BT_AV_TAG, "confirm_pin_code %s", code);
  esp_bt_gap_ssp_passkey_reply(peer_bd_addr, true, code);
}


//------------------------------------------------------------
// ==> Methods which are only supported in new ESP Release 4

#ifdef CURRENT_ESP_IDF

void BluetoothA2DPSink::app_rc_tg_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param)
{
	ESP_LOGD(BT_AV_TAG, "%s", __func__);
	switch (event) {
		case ESP_AVRC_TG_CONNECTION_STATE_EVT:
		case ESP_AVRC_TG_REMOTE_FEATURES_EVT:
		case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT:
		case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT:
		case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT:
		case ESP_AVRC_TG_SET_PLAYER_APP_VALUE_EVT:{
		
			 //Lambda for callback
				auto av_hdl_avrc_tg_evt_2 = [](uint16_t event, void *p_param) {
					ESP_LOGD(BT_AV_TAG, "av_hdl_avrc_tg_evt_2");
					if (actual_bluetooth_a2dp_sink) {
						actual_bluetooth_a2dp_sink->av_hdl_avrc_tg_evt(event,p_param);
					}
				};
			app_work_dispatch(av_hdl_avrc_tg_evt_2, event, param, sizeof(esp_avrc_tg_cb_param_t));
			break;
		}
		default:
			ESP_LOGE(BT_AV_TAG, "Invalid AVRC event: %d", event);
			break;
    }
	
}

void BluetoothA2DPSink::volume_set_by_controller(uint8_t volume)
{
    ESP_LOGI(BT_AV_TAG, "Volume is set by remote controller %d%%\n", (uint32_t)volume * 100 / 0x7f);
    _lock_acquire(&s_volume_lock);
    s_volume = volume;
    _lock_release(&s_volume_lock);
	
	
	if (bt_volumechange!=nullptr){
		(*bt_volumechange)(s_volume * 100/ 0x7f);
	}	
}

void BluetoothA2DPSink::volume_set_by_local_host(uint8_t volume)
{
    ESP_LOGI(BT_AV_TAG, "Volume is set locally to: %d%%", (uint32_t)volume * 100 / 0x7f);
    is_volume_used = true;

    _lock_acquire(&s_volume_lock);
    s_volume = volume;
    _lock_release(&s_volume_lock);

    if (s_volume_notify) {
        esp_avrc_rn_param_t rn_param;
        rn_param.volume = s_volume;
        esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_CHANGED, &rn_param);
        s_volume_notify = false;
    }
}


void BluetoothA2DPSinkCallbacks::app_rc_tg_callback(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param){
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actual_bluetooth_a2dp_sink)
    actual_bluetooth_a2dp_sink->app_rc_tg_callback(event, param);
}


extern "C" void app_rc_tg_callback_2(esp_avrc_tg_cb_event_t  event, esp_avrc_tg_cb_param_t *param){
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    BluetoothA2DPSinkCallbacks::app_rc_tg_callback(event, param);
}


void BluetoothA2DPSink::set_scan_mode_connectable(bool connectable) {
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

void BluetoothA2DPSink::set_discoverability(esp_bt_discovery_mode_t d) {
  discoverability = d;
  if (get_connection_state() == ESP_A2D_CONNECTION_STATE_DISCONNECTED || d != ESP_BT_NON_DISCOVERABLE) {
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, discoverability);
  }
}

void BluetoothA2DPSink::av_hdl_avrc_tg_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    esp_avrc_tg_cb_param_t *rc = (esp_avrc_tg_cb_param_t *)(p_param);

    switch (event) {

    case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
        uint8_t *bda = rc->conn_stat.remote_bda;
        ESP_LOGI(BT_AV_TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
       
        break;
    }
    case ESP_AVRC_TG_PASSTHROUGH_CMD_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC passthrough cmd: key_code 0x%x, key_state %d", rc->psth_cmd.key_code, rc->psth_cmd.key_state);
        break;
    }
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC set absolute volume: %d%%", (int)rc->set_abs_vol.volume * 100/ 0x7f);
		
		
        volume_set_by_controller(rc->set_abs_vol.volume);
        break;
    }

    case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC register event notification: %d, param: 0x%x", rc->reg_ntf.event_id, rc->reg_ntf.event_parameter);
        if (rc->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
            s_volume_notify = true;
            esp_avrc_rn_param_t rn_param;
            rn_param.volume = s_volume;
            esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
			
        }
        break;
    }

    case ESP_AVRC_TG_REMOTE_FEATURES_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC remote features %x, CT features %x", rc->rmt_feats.feat_mask, rc->rmt_feats.ct_feat_flag);
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}

#else 

void BluetoothA2DPSink::set_scan_mode_connectable(bool connectable) {
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



