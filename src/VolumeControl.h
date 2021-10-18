#pragma once

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

#include "BluetoothA2DPCommon.h"

/**
 * @brief Abstract class for handling of the volume of the audio data
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */

class VolumeControl {
    public:
        virtual void update_audio_data(Frame* data, uint16_t frameCount, uint8_t volume, bool mono_downmix, bool is_volume_used) {
            if (mono_downmix || is_volume_used) {
                ESP_LOGD(BT_AV_TAG, "update_audio_data");
                int32_t volumeFactor = get_volume_factor(volume);
                int32_t max = get_volume_factor_max();
                for (int i=0;i<frameCount;i++){
                    int32_t pcmLeft = data[i].channel1;
                    int32_t pcmRight = data[i].channel2;
                    // if mono -> we provide the same output on both channels
                    if (mono_downmix) {
                        pcmRight = pcmLeft = (pcmLeft + pcmRight) / 2;
                    }
                    // adjust the volume
                    if (is_volume_used) {
                        pcmLeft = pcmLeft * volumeFactor / max; 
                        pcmRight = pcmRight * volumeFactor / max; 
                    }
                    data[i].channel1 = pcmLeft;
                    data[i].channel2 = pcmRight;
                }
            }
        }
        // provides a factor in the range of 0 to 4096
        virtual int32_t get_volume_factor(uint8_t volume) = 0;

        // provides the max factor value 4096
        virtual int32_t get_volume_factor_max() {
            return 0x1000;
        }


};

/**
 * @brief Default implementation for handling of the volume of the audio data
 * @author elehobica
 * @copyright Apache License Version 2
 */
class DefaultVolumeControl : public VolumeControl {
        // provides a factor in the range of 0 to 4096
        virtual int32_t get_volume_factor(uint8_t volume) {
            constexpr double base = 1.4;
            constexpr double bits = 12;
            constexpr double zero_ofs = pow(base, -bits);
            constexpr double scale = pow(2.0, bits);
            double volumeFactorFloat = (pow(base, volume * bits / 127.0 - bits) - zero_ofs) * scale / (1.0 - zero_ofs);
            int32_t volumeFactor = volumeFactorFloat;
            if (volumeFactor > 0x1000) {
                volumeFactor = 0x1000;
            }
            return volumeFactor;
        }

};

/**
 * Simple exponentional volume control
 * @author rbruelma
 */
class SimpleExponentialVolumeControl : public VolumeControl {
        // provides a factor in the range of 0 to 4096
        virtual int32_t get_volume_factor(uint8_t volume) {
            double volumeFactorFloat = volume;
            volumeFactorFloat = pow(2.0, volumeFactorFloat * 12.0 / 127.0);
            int32_t volumeFactor = volumeFactorFloat - 1.0;
            if (volumeFactor > 0xfff) {
                volumeFactor = 0xfff;
            }
            return volumeFactor;
        }

};

/**
 * @brief The simplest possible implementation of a VolumeControl
 * @author pschatzmann
 * @copyright Apache License Version 2
 */
class LinearVolumeControl : public VolumeControl {
        // provides a factor in the range of 0 to 4096
        virtual int32_t get_volume_factor(uint8_t volume) {
            return volume;
        }
        virtual int32_t get_volume_factor_max() {
            return 128;
        }
};

/**
 * @brief Keeps the audio data as is -> no volume control!
 * @author pschatzmann
 * @copyright Apache License Version 2
 */
class NoVolumeControl : public VolumeControl {
    public:
        virtual void update_audio_data(Frame* data, uint16_t frameCount, uint8_t volume, bool mono_downmix, bool ivolume_used) {
        }
};