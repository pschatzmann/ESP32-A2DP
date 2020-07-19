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


#include "esp32_bt_music_receiver.h"

/**
 * Some data that must be avaliable for C calls
 */
// to support static callback functions
BlootoothA2DSink* actualBlootoothA2DSink;
i2s_port_t i2s_port; 

// Forward declarations for C Callback functions for ESP32 Framework
extern "C" void app_task_handler_2(void *arg);
extern "C" void audio_data_callback_2(const uint8_t *data, uint32_t len);
extern "C" void app_a2d_callback_2(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);
extern "C" void app_rc_ct_callback_2(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
static void av_hdl_stack_evt_2(uint16_t event, void *p_param);
static void av_hdl_a2d_evt_2(uint16_t event, void *p_param);
static void av_hdl_avrc_evt_2(uint16_t event, void *p_param);


/**
 * Constructor
 */
BlootoothA2DSink::BlootoothA2DSink() {
  actualBlootoothA2DSink = this;
  // default i2s port is 0
  i2s_port = (i2s_port_t) 0;

  // setup default i2s config
  i2s_config = {
      .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = 44100,
      .bits_per_sample = (i2s_bits_per_sample_t)16,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t) (I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = 0, // default interrupt priority
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false
  };

  // setup default pins
  pin_config = {
      .bck_io_num = 26,
      .ws_io_num = 25,
      .data_out_num = 22,
      .data_in_num = I2S_PIN_NO_CHANGE
  };

}

BlootoothA2DSink::~BlootoothA2DSink() {
    app_task_shut_down();

    if (i2s_driver_uninstall(i2s_port)!= ESP_OK){
	    ESP_LOGE(BT_AV_TAG,"Failed to uninstall i2s");
    } 
    
    if (esp_a2d_sink_deinit()!=ESP_OK){
	    ESP_LOGE(BT_AV_TAG,"Failed to deinit a2d sink");
    }
    
    if (esp_a2d_sink_disconnect!= ESP_OK){
	    ESP_LOGE(BT_AV_TAG,"Failed to disconnect a2d sink");
    }

    if (esp_bluedroid_deinit()!= ESP_OK){
	    ESP_LOGE(BT_AV_TAG,"Failed to deinit bluetooth");
    }
    
    if (esp_bluedroid_disable()!= ESP_OK){
	    ESP_LOGE(BT_AV_TAG,"Failed to disable bluetooth");
    }

}


void BlootoothA2DSink::set_pin_config(i2s_pin_config_t pin_config){
  this->pin_config = pin_config;
}

void BlootoothA2DSink::set_i2s_port(i2s_port_t i2s_num) {
  i2s_port = i2s_num;
}

void BlootoothA2DSink::set_i2s_config(i2s_config_t i2s_config){
  this->i2s_config = i2s_config;
}


/**
 * Main function to start the Bluetooth Processing
 */
void BlootoothA2DSink::start(char* name)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    //store parameters
    if (name) {
      this->bt_name = name;
    }
    ESP_LOGI(BT_AV_TAG,"Device name will be set to '%s'",this->bt_name);
    
    // setup bluetooth
    init_bluetooth();
    
    // create application task 
    app_task_start_up();

    // Bluetooth device name, connection mode and profile set up 
    app_work_dispatch(av_hdl_stack_evt_2, BT_APP_EVT_STACK_UP, NULL, 0);

    // setup i2s
    i2s_driver_install(i2s_port, &i2s_config, 0, NULL);

    // pins are only relevant when music is not sent to internal DAC
    if (i2s_config.mode & I2S_MODE_DAC_BUILT_IN) {
      ESP_LOGI(BT_AV_TAG, "Output will go to DAC pins");
      i2s_set_pin(i2s_port, NULL);      
    } else {
      i2s_set_pin(i2s_port, &pin_config);
    }

}


esp_a2d_audio_state_t BlootoothA2DSink::get_audio_state() {
  return audio_state;
}

esp_a2d_mct_t BlootoothA2DSink::get_audio_type() {
  return audio_type;
}


int BlootoothA2DSink::init_bluetooth()
{
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (!btStart()) {
    ESP_LOGE(BT_AV_TAG,"Failed to initialize controller");
    return false;
  }
  ESP_LOGI(BT_AV_TAG,"controller initialized");
 
  if (esp_bluedroid_init() != ESP_OK) {
    ESP_LOGE(BT_AV_TAG,"Failed to initialize bluedroid");
    return false;
  }
  ESP_LOGI(BT_AV_TAG,"bluedroid initialized");
 
  if (esp_bluedroid_enable() != ESP_OK) {
    ESP_LOGE(BT_AV_TAG,"Failed to enable bluedroid");
    return false;
  }
  ESP_LOGI(BT_AV_TAG,"bluedroid enabled");
 
}

bool BlootoothA2DSink::app_work_dispatch(app_callback_t p_cback, uint16_t event, void *p_params, int param_len)
{
    ESP_LOGD(BT_APP_CORE_TAG, "%s event 0x%x, param len %d", __func__, event, param_len);
    
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

void BlootoothA2DSink::app_work_dispatched(app_msg_t *msg)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    if (msg->cb) {
        msg->cb(msg->event, msg->param);
    }
}


bool BlootoothA2DSink::app_send_msg(app_msg_t *msg)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    if (msg == NULL) {
        return false;
    }

    if (xQueueSend(app_task_queue, msg, 10 / portTICK_RATE_MS) != pdTRUE) {
        ESP_LOGE(BT_APP_CORE_TAG, "%s xQueue send failed", __func__);
        return false;
    }
    return true;
}


void BlootoothA2DSink::app_task_handler()
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    app_msg_t msg;
    for (;;) {
        if (pdTRUE == xQueueReceive(app_task_queue, &msg, (portTickType)portMAX_DELAY)) {
            ESP_LOGD(BT_APP_CORE_TAG, "%s, sig 0x%x, 0x%x", __func__, msg.sig, msg.event);
            switch (msg.sig) {
            case APP_SIG_WORK_DISPATCH:
                ESP_LOGW(BT_APP_CORE_TAG, "%s, APP_SIG_WORK_DISPATCH sig: %d", __func__, msg.sig);
                app_work_dispatched(&msg);
                break;
            default:
                ESP_LOGW(BT_APP_CORE_TAG, "%s, unhandled sig: %d", __func__, msg.sig);
                break;
            } // switch (msg.sig)

            if (msg.param) {
                free(msg.param);
            }
        }
    }
}

void BlootoothA2DSink::app_task_start_up(void)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    app_task_queue = xQueueCreate(10, sizeof(app_msg_t));
    xTaskCreate(app_task_handler_2, "BtAppT", 2048, NULL, configMAX_PRIORITIES - 3, &app_task_handle);
    return;
}

void  BlootoothA2DSink::app_task_shut_down(void)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    if (app_task_handle) {
        vTaskDelete(app_task_handle);
        app_task_handle = NULL;
    }
    if (app_task_queue) {
        vQueueDelete(app_task_queue);
        app_task_queue = NULL;
    }
}


void  BlootoothA2DSink::app_alloc_meta_buffer(esp_avrc_ct_cb_param_t *param)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(param);
    uint8_t *attr_text = (uint8_t *) malloc (rc->meta_rsp.attr_length + 1);
    memcpy(attr_text, rc->meta_rsp.attr_text, rc->meta_rsp.attr_length);
    attr_text[rc->meta_rsp.attr_length] = 0;

    rc->meta_rsp.attr_text = attr_text;
}

void  BlootoothA2DSink::app_rc_ct_callback(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);

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
    default:
        ESP_LOGE(BT_AV_TAG, "Invalid AVRC event: %d", event);
        break;
    }
}

void  BlootoothA2DSink::av_hdl_a2d_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    esp_a2d_cb_param_t *a2d = NULL;
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT: {
        ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_CONNECTION_STATE_EVT", __func__);
        a2d = (esp_a2d_cb_param_t *)(p_param);
        uint8_t *bda = a2d->conn_stat.remote_bda;
        ESP_LOGI(BT_AV_TAG, "A2DP connection state: %s, [%02x:%02x:%02x:%02x:%02x:%02x]",
             m_a2d_conn_state_str[a2d->conn_stat.state], bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        break;
    }
    case ESP_A2D_AUDIO_STATE_EVT: {
        ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_AUDIO_STATE_EVT", __func__);
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_LOGI(BT_AV_TAG, "A2DP audio state: %s", m_a2d_audio_state_str[a2d->audio_stat.state]);
        m_audio_state = a2d->audio_stat.state;
        if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state) {
            m_pkt_cnt = 0;
        }
        break;
    }
    case ESP_A2D_AUDIO_CFG_EVT: {
        ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_AUDIO_CFG_EVT", __func__);
        esp_a2d_cb_param_t *esp_a2d_callback_param = (esp_a2d_cb_param_t *)(p_param);
        audio_type = esp_a2d_callback_param->audio_cfg.mcc.type;
        a2d = (esp_a2d_cb_param_t *)(p_param);
        ESP_LOGI(BT_AV_TAG, "a2dp audio_cfg_cb , codec type %d", a2d->audio_cfg.mcc.type);
        // for now only SBC stream is supported
        if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            i2s_config.sample_rate = 16000;
            char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
            if (oct0 & (0x01 << 6)) {
                i2s_config.sample_rate = 32000;
            } else if (oct0 & (0x01 << 5)) {
                i2s_config.sample_rate = 44100;
            } else if (oct0 & (0x01 << 4)) {
                i2s_config.sample_rate = 48000;
            }
            
            i2s_set_clk(i2s_port, i2s_config.sample_rate, i2s_config.bits_per_sample, (i2s_channel_t)2);

            ESP_LOGI(BT_AV_TAG, "configure audio player %x-%x-%x-%x\n",
                     a2d->audio_cfg.mcc.cie.sbc[0],
                     a2d->audio_cfg.mcc.cie.sbc[1],
                     a2d->audio_cfg.mcc.cie.sbc[2],
                     a2d->audio_cfg.mcc.cie.sbc[3]);
            ESP_LOGI(BT_AV_TAG, "audio player configured, samplerate=%d", i2s_config.sample_rate);
        }
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}

void  BlootoothA2DSink::av_new_track()
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
    //Register notifications and request metadata
    esp_avrc_ct_send_metadata_cmd(0, ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST | ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_GENRE);
    esp_avrc_ct_send_register_notification_cmd(1, ESP_AVRC_RN_TRACK_CHANGE, 0);
}

void  BlootoothA2DSink::av_notify_evt_handler(uint8_t event_id, uint32_t event_parameter)
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

void  BlootoothA2DSink::av_hdl_avrc_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
        uint8_t *bda = rc->conn_stat.remote_bda;
        ESP_LOGI(BT_AV_TAG, "AVRC conn_state evt: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        if (rc->conn_stat.connected) {
            av_new_track();
        }
        break;
    }
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC passthrough rsp: key_code 0x%x, key_state %d", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
        break;
    }
    case ESP_AVRC_CT_METADATA_RSP_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC metadata rsp: attribute id 0x%x, %s", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
        free(rc->meta_rsp.attr_text);
        break;
    }
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC event notification: %d, param: %d", rc->change_ntf.event_id, rc->change_ntf.event_parameter);
        av_notify_evt_handler(rc->change_ntf.event_id, rc->change_ntf.event_parameter);
        break;
    }
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT: {
        ESP_LOGI(BT_AV_TAG, "AVRC remote features %x", rc->rmt_feats.feat_mask);
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}


void  BlootoothA2DSink::av_hdl_stack_evt(uint16_t event, void *p_param)
{
    switch (event) {
    case BT_APP_EVT_STACK_UP: {
        ESP_LOGD(BT_AV_TAG, "%s av_hdl_stack_evt %s", __func__, "BT_APP_EVT_STACK_UP");
        /* set up device name */
        esp_bt_dev_set_device_name(bt_name);

        /* initialize A2DP sink */
        esp_a2d_register_callback(app_a2d_callback_2);
        esp_a2d_sink_register_data_callback(audio_data_callback_2);
        esp_a2d_sink_init();

        /* initialize AVRCP controller */
        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(app_rc_ct_callback_2);

        /* set discoverable and connectable mode, wait to be connected */
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}


/* callback for A2DP sink */
void  BlootoothA2DSink::app_a2d_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    ESP_LOGD(BT_AV_TAG, "%s", __func__);
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
    default:
        ESP_LOGE(BT_AV_TAG, "Invalid A2DP event: %d", event);
        break;
    }
}

void  BlootoothA2DSink::audio_data_callback(const uint8_t *data, uint32_t len) {
   ESP_LOGD(BT_AV_TAG, "%s", __func__);

   size_t i2s_bytes_written;
   if (i2s_write(i2s_port,(void*) data, len, &i2s_bytes_written, portMAX_DELAY)!=ESP_OK){
      ESP_LOGE(BT_AV_TAG, "i2s_write has failed");    
   }

   if (i2s_bytes_written<len){
      ESP_LOGE(BT_AV_TAG, "Timeout: not all bytes were written to I2S");
   } 
}


/**
 * Static methods as internal callbacks
 */
void av_hdl_stack_evt_2(uint16_t event, void *p_param) {
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actualBlootoothA2DSink)
    actualBlootoothA2DSink->av_hdl_stack_evt(event,p_param);
}
void av_hdl_a2d_evt_2(uint16_t event, void *p_param){
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actualBlootoothA2DSink)
    actualBlootoothA2DSink->av_hdl_a2d_evt(event,p_param);  
}
void av_hdl_avrc_evt_2(uint16_t event, void *p_param){
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actualBlootoothA2DSink)
    actualBlootoothA2DSink->av_hdl_avrc_evt(event,p_param);    
}


/**
 * C Callback Functions needed for the ESP32 API
 */
extern "C" void app_task_handler_2(void *arg) {
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actualBlootoothA2DSink)
    actualBlootoothA2DSink->app_task_handler();
}

extern "C" void audio_data_callback_2(const uint8_t *data, uint32_t len) {
  //ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actualBlootoothA2DSink)
    actualBlootoothA2DSink->audio_data_callback(data,len);
}

extern "C" void app_a2d_callback_2(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param){
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actualBlootoothA2DSink)
    actualBlootoothA2DSink->app_a2d_callback(event, param);
}

extern "C" void app_rc_ct_callback_2(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param){
  ESP_LOGD(BT_AV_TAG, "%s", __func__);
  if (actualBlootoothA2DSink)
    actualBlootoothA2DSink->app_rc_ct_callback(event, param);
}
