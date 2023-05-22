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

#include "SoundData.h"
#include "esp_log.h"

/**
 * @brief Abstract class for handling of the volume of the audio data
 * @ingroup a2dp
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */

class A2DPVolumeControl {

    protected:
        bool is_volume_used = false;
        bool mono_downmix = false;
        int32_t volumeFactor;
        int32_t volumeFactorMax;

    public:
        A2DPVolumeControl() {
            volumeFactorMax = 0x1000;
        }

        virtual void update_audio_data(Frame* data, uint16_t frameCount) {
            if (data!=nullptr && frameCount>0 && ( mono_downmix || is_volume_used)) {
                ESP_LOGD("VolumeControl", "update_audio_data");
                for (int i=0;i<frameCount;i++){
                    int32_t pcmLeft = data[i].channel1;
                    int32_t pcmRight = data[i].channel2;
                    // if mono -> we provide the same output on both channels
                    if (mono_downmix) {
                        pcmRight = pcmLeft = (pcmLeft + pcmRight) / 2;
                    }
                    // adjust the volume
                    if (is_volume_used) {
                        pcmLeft = pcmLeft * volumeFactor / volumeFactorMax;
                        pcmRight = pcmRight * volumeFactor / volumeFactorMax;
                    }
                    data[i].channel1 = pcmLeft;
                    data[i].channel2 = pcmRight;
                }
            }
        }

        // provides a factor in the range of 0 to 4096
        int32_t get_volume_factor() {
            return volumeFactor;
        }

        // provides the max factor value 4096
        int32_t get_volume_factor_max() {
            return volumeFactorMax;
        }

        void set_enabled(bool enabled) {
            is_volume_used = enabled;
        }

        void set_mono_downmix(bool enabled) {
            mono_downmix = enabled;
        }

        virtual void set_volume(uint8_t volume) = 0;
};

/**
 * @brief Default implementation for handling of the volume of the audio data
 * @author elehobica
 * @copyright Apache License Version 2
 */
class A2DPDefaultVolumeControl : public A2DPVolumeControl {

        virtual void set_volume(uint8_t volume) {
            constexpr double base = 1.4;
            constexpr double bits = 12;
            constexpr double zero_ofs = pow(base, -bits);
            constexpr double scale = pow(2.0, bits);
            double volumeFactorFloat = (pow(base, volume * bits / 127.0 - bits) - zero_ofs) * scale / (1.0 - zero_ofs);
            volumeFactor = volumeFactorFloat;
            if (volumeFactor > 0x1000) {
                volumeFactor = 0x1000;
            }
        }
};

/**
 * @brief  Exponentional volume control
 * @author rbruelma
 */
class A2DPSimpleExponentialVolumeControl : public A2DPVolumeControl {
        virtual void set_volume(uint8_t volume) {
            double volumeFactorFloat = volume;
            volumeFactorFloat = pow(2.0, volumeFactorFloat * 12.0 / 127.0);
            int32_t volumeFactor = volumeFactorFloat - 1.0;
            if (volumeFactor > 0xfff) {
                volumeFactor = 0xfff;
            }
        }
};

/**
 * @brief The simplest possible implementation of a VolumeControl
 * @author pschatzmann
 * @copyright Apache License Version 2
 */
class A2DPLinearVolumeControl : public A2DPVolumeControl {

        A2DPLinearVolumeControl() {
            volumeFactorMax = 128;
        }

        virtual void set_volume(uint8_t volume) {
            volumeFactor = volume;
        }
};

/**
 * @brief Keeps the audio data as is -> no volume control!
 * @author pschatzmann
 * @copyright Apache License Version 2
 */
class A2DPNoVolumeControl : public A2DPVolumeControl {
    public:
        virtual void update_audio_data(Frame* data, uint16_t frameCount) {
        }
};