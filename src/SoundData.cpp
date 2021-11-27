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
//  TwoChannelSoundData
//*****************************************************************************************

/**
 *  Constructor for Data containing 2 channels
 */
TwoChannelSoundData::TwoChannelSoundData(Frame *data, int32_t len, bool loop) {
    setData(data, len);
    setLoop(loop);
}

TwoChannelSoundData::TwoChannelSoundData(bool loop) {
    setLoop(loop);
}


void TwoChannelSoundData::setData(Frame *data, int32_t len){
    this->len = len;
    this->data = data;
}

void TwoChannelSoundData::setDataRaw(uint8_t *data, int32_t len){
    this->len = len/4;
    this->data = (Frame *)data;
}


int32_t TwoChannelSoundData::getData(int32_t pos, int32_t len, Frame *data) {
    //ESP_LOGD(SOUND_DATA, "x%x - pos: %d / len: %d", __func__, pos, len);
    int result_len = std::min(len, this->len - pos);
    for (int32_t j=0;j<result_len;j++){
        data[j] = this->data[pos+j];
    }
    return result_len;
}

int32_t TwoChannelSoundData::getData(int32_t pos, Frame &frame){
    int32_t result = 0;
    if (pos<this->len){
        result = 1;
        frame.channel1 = this->data[pos].channel1;
        frame.channel2 = this->data[pos].channel2;
    }
    return result;
}

/**
 * pos and len in bytes
 */
int32_t TwoChannelSoundData::get2ChannelData(int32_t pos, int32_t len, uint8_t *data) {
    //ESP_LOGD(SOUND_DATA, "x%x - pos: %d / len: %d", __func__, pos, len);
    return getData(pos/4, len/4, (Frame*)data)*4;
}

//*****************************************************************************************
// OneChannelSoundData
//*****************************************************************************************

/** 
 * Constructor for data conisting only of one Channel 
 */
OneChannelSoundData::OneChannelSoundData(int16_t *data, int32_t len, bool loop, ChannelInfo channelInfo) {
    this->channelInfo = channelInfo;
    setData(data, len);
    setLoop(loop);
}

OneChannelSoundData::OneChannelSoundData(bool loop, ChannelInfo channelInfo) {
    this->channelInfo = channelInfo;
    setLoop(loop);
}

void OneChannelSoundData::setData(int16_t *data, int32_t len){
    this->len = len;
    this->data = data;
}

void OneChannelSoundData::setDataRaw(uint8_t *data, int32_t lenBytes){
    this->len = lenBytes / sizeof(int16_t);
    this->data = (int16_t *)data;
}

int32_t OneChannelSoundData::getData(int32_t pos, int32_t len, int16_t *data) {
    int result_len = std::min(len, this->len - pos);    
    for (int32_t j=0;j<result_len;j++){
        data[j] = this->data[pos+j];
    }
    return result_len;
}

/**
 * Data is stored in one channel with int16_t data. However we need to provide 2 channels. 
 * pos, len and result are in bytes.
 * 
 */
int32_t OneChannelSoundData::get2ChannelData(int32_t pos, int32_t len, uint8_t *data) {
    //ESP_LOGD(SOUND_DATA, "x%x - pos: %d / len: %d", __func__, pos, len);
    Frame *result_data = (Frame*) data;
    int32_t frame_count = len / 4;
    int32_t frame_start = pos / 4 ;  
    int32_t result_len = 0;
    int32_t frame_pos;

    for (int32_t j=0; j<frame_count; j++){
        frame_pos = frame_start+j;
        if (getData(frame_pos, result_data[j]) == 0)
            break;
        result_len+=4;
    }
    return result_len;
}

int32_t OneChannelSoundData::getData(int32_t pos, Frame &frame){
    int32_t result = 0;
    if (pos<this->len){
        result = 1;
        switch(channelInfo){
            case Left:
                frame.channel1 = this->data[pos];
                frame.channel2 = 0;
                break;
            case Right:
                frame.channel1 = 0;
                frame.channel2 = this->data[pos];
                break;

            case Both:
            default:
                frame.channel1 = this->data[pos];
                frame.channel2 = frame.channel1;
                break;
        }
    }
    return result;
}

//*****************************************************************************************
// OneChannel8BitSoundData
//*****************************************************************************************

/** 
 * Constructor for data conisting only of one Channel 
 */
OneChannel8BitSoundData::OneChannel8BitSoundData(int8_t *data, int32_t len, bool loop, ChannelInfo channelInfo) {
    this->channelInfo = channelInfo;
    setData(data, len);
    setLoop(loop);
}

OneChannel8BitSoundData::OneChannel8BitSoundData(bool loop, ChannelInfo channelInfo) {
    this->channelInfo = channelInfo;
    setLoop(loop);
}

void OneChannel8BitSoundData::setData(int8_t *data, int32_t len){
    this->len = len;
    this->data = data;
}

void OneChannel8BitSoundData::setDataRaw(uint8_t *data, int32_t lenBytes){
    this->len = lenBytes / sizeof(int8_t);
    this->data = (int8_t *)data;
}

int32_t OneChannel8BitSoundData::getData(int32_t pos, int32_t len, int8_t *data) {
    int result_len = std::min(len, this->len - pos);    
    for (int32_t j=0;j<result_len;j++){
        data[j] = this->data[pos+j];
    }
    return result_len;
}

/**
 * Data is stored in one channel with int16_t data. However we need to provide 2 channels. 
 * pos, len and result are in bytes.
 * 
 */
int32_t OneChannel8BitSoundData::get2ChannelData(int32_t pos, int32_t len, uint8_t *data) {
    //ESP_LOGD(SOUND_DATA, "x%x - pos: %d / len: %d", __func__, pos, len);
    Frame *result_data = (Frame*) data;
    int32_t frame_count = len / 4;
    int32_t frame_start = pos / 4 ;  
    int32_t result_len = 0;
    int32_t frame_pos;

    for (int32_t j=0; j<frame_count; j++){
        frame_pos = frame_start+j;
        if (getData(frame_pos, result_data[j]) == 0)
            break;
        result_len+=4;
    }
    return result_len;
}

int32_t OneChannel8BitSoundData::getData(int32_t pos, Frame &frame){
    int32_t result = 0;
    if (pos<this->len){
        result = 1;
        switch(channelInfo){
            case Left:
                frame.channel1 = this->data[pos] * 127;
                frame.channel2 = 0;
                break;
            case Right:
                frame.channel1 = 0;
                frame.channel2 = this->data[pos] * 127;
                break;

            case Both:
            default:
                frame.channel1 = this->data[pos] * 127;
                frame.channel2 = frame.channel1;
                break;
        }
    }
    return result;
}

