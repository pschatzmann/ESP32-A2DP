#include "SoundData.h"

#define SOUND_DATA "SOUND_DATA"

//*****************************************************************************************

bool SoundData::doLoop() {
    return automatic_loop;
}
void SoundData::setLoop(bool loop) {
    automatic_loop = loop;
}

//*****************************************************************************************

/**
 *  Constructor for Data containing 2 channels
 */
TwoChannelSoundData::TwoChannelSoundData(Channels *data, int32_t len, bool loop) {
    this->len = len;
    this->data = data;
    setLoop(loop);
}

int32_t TwoChannelSoundData::getData(int32_t pos, int32_t len, Channels *data) {
    //ESP_LOGD(SOUND_DATA, "x%x - pos: %d / len: %d", __func__, pos, len);
    int result_len = min(len, this->len - pos);
    for (int32_t j=0;j<result_len;j++){
        data[j] = this->data[pos+j];
    }
    return result_len;
}

int32_t TwoChannelSoundData::getData(int32_t pos, Channels &channels){
    int32_t result = 0;
    if (pos<this->len){
        result = 1;
        channels.channel1 = this->data[pos].channel1;
        channels.channel2 = this->data[pos].channel2;
    }
    return result;
}

/**
 * pos and len in bytes
 */
int32_t TwoChannelSoundData::get2ChannelData(int32_t pos, int32_t len, uint8_t *data) {
    //ESP_LOGD(SOUND_DATA, "x%x - pos: %d / len: %d", __func__, pos, len);
    return getData(pos/4, len/4, (Channels*)data)*4;
}

//*****************************************************************************************

/** 
 * Constructor for data conisting only of one Channel 
 */
OneChannelSoundData::OneChannelSoundData(int16_t *data, int32_t len, bool loop, ChannelInfo channelInfo) {
    this->len = len;
    this->data = data;
    this->channelInfo = channelInfo;
    setLoop(loop);
}

int32_t OneChannelSoundData::getData(int32_t pos, int32_t len, int16_t *data) {
    int result_len = min(len, this->len - pos);    
    for (int32_t j=0;j<result_len;j++){
        data[j] = this->data[pos+j];
    }
    return result_len;
}

/**
 * Data is stored in one channel with int16_t data. However we need to provide 2 Channels 
 * pos and len are in bytes.
 * 
 */
int32_t OneChannelSoundData::get2ChannelData(int32_t pos, int32_t len, uint8_t *data) {
    //ESP_LOGD(SOUND_DATA, "x%x - pos: %d / len: %d", __func__, pos, len);
    Channels *result_data = (Channels*) data;
    int32_t req_len_in_channels = len / 4;
    int32_t start = pos / 4 ;  // position in uint16_t array
    int32_t result_len = 0;
    int32_t source_data_pos;

    for (int32_t j=0;j<req_len_in_channels;j++){
        source_data_pos = start+j;
        if (getData(source_data_pos, result_data[j]) == 0)
            break;
        result_len+=4;
    }
    return result_len;
}

int32_t OneChannelSoundData::getData(int32_t pos, Channels &channels){
    int32_t result = 0;
    if (pos<this->len){
        result = 1;
        switch(channelInfo){
            case Left:
                channels.channel1 = this->data[pos];
                channels.channel2 = 0;
                break;
            case Right:
                channels.channel1 = 0;
                channels.channel2 = this->data[pos];
                break;

            case Both:
            default:
                channels.channel1 = this->data[pos];
                channels.channel2 = this->data[pos];
                break;
        }
    }
    return result;
}

