/**
 * @brief Buetooth A2DP datapath states
 * @ingroup a2dp
 */ 
enum esp_a2d_audio_state_t {
    ESP_A2D_AUDIO_STATE_SUSPEND = 0,           /*!< audio stream datapath suspended by remote device */
    ESP_A2D_AUDIO_STATE_STARTED,               /*!< audio stream datapath started */
    ESP_A2D_AUDIO_STATE_STOPPED = ESP_A2D_AUDIO_STATE_SUSPEND,          /*!< @note Deprecated */
    ESP_A2D_AUDIO_STATE_REMOTE_SUSPEND = ESP_A2D_AUDIO_STATE_SUSPEND,   /*!< @note Deprecated */
};

/**
 * @brief Buetooth A2DP connection states
 * @ingroup a2dp
 */ 
enum esp_a2d_connection_state_t {
    ESP_A2D_CONNECTION_STATE_DISCONNECTED = 0, /*!< connection released  */
    ESP_A2D_CONNECTION_STATE_CONNECTING,       /*!< connecting remote device */
    ESP_A2D_CONNECTION_STATE_CONNECTED,        /*!< connection established */
    ESP_A2D_CONNECTION_STATE_DISCONNECTING     /*!< disconnecting remote device */
} ;

/**
 * @brief AVRC event notification ids
 * @ingroup a2dp
 */ 
enum esp_avrc_rn_event_ids_t
    ESP_AVRC_RN_PLAY_STATUS_CHANGE = 0x01,        /*!< track status change, eg. from playing to paused */
    ESP_AVRC_RN_TRACK_CHANGE = 0x02,              /*!< new track is loaded */
    ESP_AVRC_RN_TRACK_REACHED_END = 0x03,         /*!< current track reached end */
    ESP_AVRC_RN_TRACK_REACHED_START = 0x04,       /*!< current track reached start position */
    ESP_AVRC_RN_PLAY_POS_CHANGED = 0x05,          /*!< track playing position changed */
    ESP_AVRC_RN_BATTERY_STATUS_CHANGE = 0x06,     /*!< battery status changed */
    ESP_AVRC_RN_SYSTEM_STATUS_CHANGE = 0x07,      /*!< system status changed */
    ESP_AVRC_RN_APP_SETTING_CHANGE = 0x08,        /*!< application settings changed */
    ESP_AVRC_RN_NOW_PLAYING_CHANGE = 0x09,        /*!< now playing content changed */
    ESP_AVRC_RN_AVAILABLE_PLAYERS_CHANGE = 0x0a,  /*!< available players changed */
    ESP_AVRC_RN_ADDRESSED_PLAYER_CHANGE = 0x0b,   /*!< the addressed player changed */
    ESP_AVRC_RN_UIDS_CHANGE = 0x0c,               /*!< UIDs changed */
    ESP_AVRC_RN_VOLUME_CHANGE = 0x0d,             /*!< volume changed locally on TG */
    ESP_AVRC_RN_MAX_EVT
};



/**
 * @brief AVRC event notification ids
 * @ingroup a2dp
 */ 
enum esp_avrc_rn_event_ids_t{
    ESP_AVRC_RN_PLAY_STATUS_CHANGE = 0x01,        /*!< track status change, eg. from playing to paused */
    ESP_AVRC_RN_TRACK_CHANGE = 0x02,              /*!< new track is loaded */
    ESP_AVRC_RN_TRACK_REACHED_END = 0x03,         /*!< current track reached end */
    ESP_AVRC_RN_TRACK_REACHED_START = 0x04,       /*!< current track reached start position */
    ESP_AVRC_RN_PLAY_POS_CHANGED = 0x05,          /*!< track playing position changed */
    ESP_AVRC_RN_BATTERY_STATUS_CHANGE = 0x06,     /*!< battery status changed */
    ESP_AVRC_RN_SYSTEM_STATUS_CHANGE = 0x07,      /*!< system status changed */
    ESP_AVRC_RN_APP_SETTING_CHANGE = 0x08,        /*!< application settings changed */
    ESP_AVRC_RN_NOW_PLAYING_CHANGE = 0x09,        /*!< now playing content changed */
    ESP_AVRC_RN_AVAILABLE_PLAYERS_CHANGE = 0x0a,  /*!< available players changed */
    ESP_AVRC_RN_ADDRESSED_PLAYER_CHANGE = 0x0b,   /*!< the addressed player changed */
    ESP_AVRC_RN_UIDS_CHANGE = 0x0c,               /*!< UIDs changed */
    ESP_AVRC_RN_VOLUME_CHANGE = 0x0d,             /*!< volume changed locally on TG */
    ESP_AVRC_RN_MAX_EVT
};


/**
 * @brief AVRCP current status of playback
 * @ingroup a2dp
 */ 
enum  esp_avrc_playback_stat_t{
    ESP_AVRC_PLAYBACK_STOPPED = 0,                /*!< stopped */
    ESP_AVRC_PLAYBACK_PLAYING = 1,                /*!< playing */
    ESP_AVRC_PLAYBACK_PAUSED = 2,                 /*!< paused */
    ESP_AVRC_PLAYBACK_FWD_SEEK = 3,               /*!< forward seek */
    ESP_AVRC_PLAYBACK_REV_SEEK = 4,               /*!< reverse seek */
    ESP_AVRC_PLAYBACK_ERROR = 0xFF,               /*!< error */
} ;

/**
 * @brief AVRCP discovery mode
 * @ingroup a2dp
 */ 
enum esp_bt_discovery_mode_t{
    ESP_BT_NON_DISCOVERABLE,            /*!< Non-discoverable */
    ESP_BT_LIMITED_DISCOVERABLE,        /*!< Limited Discoverable */
    ESP_BT_GENERAL_DISCOVERABLE,        /*!< General Discoverable */
} ;


/**
 * @brief Bluetooth Controller mode
 * @ingroup a2dp
 */
enum esp_bt_mode_t {
    ESP_BT_MODE_IDLE       = 0x00,   /*!< Bluetooth is not operating. */
    ESP_BT_MODE_BLE        = 0x01,   /*!< Bluetooth is operating in BLE mode. */
    ESP_BT_MODE_CLASSIC_BT = 0x02,   /*!< Bluetooth is operating in Classic Bluetooth mode. */
    ESP_BT_MODE_BTDM       = 0x03,   /*!< Bluetooth is operating in Dual mode. */
} ;

/**
 * @brief Bluetooth address
 * @ingroup a2dp
 */
typedef uint8_t esp_bd_addr_t[ESP_BD_ADDR_LEN];
