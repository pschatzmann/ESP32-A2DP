#pragma once
#include "BluetoothA2DPSink.h"

/**
 * @brief Custom 32 bit stereo output. 
 * We demonstrate how we can adapt the functionality by subclassing and overwriting a method. 
 * it seems that i2s_write_expand doesn't utilize the resolution below 16bits. 
 * It apparently makes difference in sound quality when small volume value is applied if 
 * lower bits are fully used.
 *
 * @author elehobica
 **/
class BluetoothA2DPSink32 : public BluetoothA2DPSink {
    protected:
        void audio_data_callback(const uint8_t *data, uint32_t len) {
            ESP_LOGD(BT_AV_TAG, "%s", __PRETTY_FUNCTION__);
            Frame* frame = (Frame*) data;  // convert to array of frames
            static constexpr int blk_size = 128;
            static uint32_t data32[blk_size/2];
            uint32_t rest_len = len;
            int32_t volumeFactor = 0x1000;
            
            // adjust the volume
            volume_control()->update_audio_data((Frame*)data, len/4);

            while (rest_len>0) {
                uint32_t blk_len = (rest_len>=blk_size) ? blk_size : rest_len;
                for (int i=0; i<blk_len/4; i++) {
                    int32_t pcmLeft = frame->channel1;
                    int32_t pcmRight = frame->channel2;
                    pcmLeft = pcmLeft  * volumeFactor;
                    pcmRight = pcmRight  * volumeFactor;
                    data32[i*2+0] = pcmLeft;
                    data32[i*2+1] = pcmRight;
                    frame++;
                }
                
                size_t i2s_bytes_written;
                if (i2s_write(i2s_port,(void*) data32, blk_len*2, &i2s_bytes_written, portMAX_DELAY)!=ESP_OK){
                    ESP_LOGE(BT_AV_TAG, "i2s_write has failed");
                }

                if (i2s_bytes_written<blk_len*2){
                    ESP_LOGE(BT_AV_TAG, "Timeout: not all bytes were written to I2S");
                }
                rest_len -= blk_len;
            }
        }

};