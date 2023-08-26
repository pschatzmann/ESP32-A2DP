#pragma once

#include "BluetoothA2DPSink.h"

#if A2DP_I2S_SUPPORT

/**
 * @brief The BluetoothA2DPSinkQueued is using a separate Task with an additinal Queue to write the I2S data.
 * application. 
 * @ingroup a2dp
 * @author Phil Schatzmann
 * @copyright Apache License Version 2
 */
class BluetoothA2DPSinkQueued : public BluetoothA2DPSink {
    public:
        BluetoothA2DPSinkQueued() = default;

        /// Defines the stack size of the i2s task (in bytes)
        void set_i2s_stack_size(int size){
            i2s_stack_size = size;
        }

        /// Defines the ringbuffer size used by the i2s task (in bytes)
        void set_i2s_ringbuffer_size(int size){
            i2s_ringbuffer_size = size;
        }

        /// Defines the priority of the I2S task
        void set_i2s_task_priority(UBaseType_t prio){
            i2s_task_priority = prio;
        }

    protected:
        xTaskHandle s_bt_i2s_task_handle = nullptr;  /* handle of I2S task */
        RingbufHandle_t s_ringbuf_i2s = nullptr;     /* handle of ringbuffer for I2S */
        // I2S task
        int i2s_stack_size = 2048;
        int i2s_ringbuffer_size = 3 * 5120;
        UBaseType_t i2s_task_priority = configMAX_PRIORITIES - 0;

        void bt_i2s_task_start_up(void) override;
        void bt_i2s_task_shut_down(void) override;
        void i2s_task_handler(void *arg) override;
        size_t write_audio(const uint8_t *data, size_t size) override;

};

#endif
