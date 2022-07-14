
#include "BluetoothA2DPSinkQueued.h"

void BluetoothA2DPSinkQueued::bt_i2s_task_start_up(void)
{
    if ((s_ringbuf_i2s = xRingbufferCreate(i2s_ringbuffer_size, RINGBUF_TYPE_BYTEBUF)) == NULL) {
        ESP_LOGE(BT_AV_TAG, "xRingbufferCreate");    
        return;
    }
    BaseType_t result = xTaskCreatePinnedToCore(ccall_i2s_task_handler, "BtI2STask", i2s_stack_size, NULL, i2s_task_priority, &s_bt_i2s_task_handle, task_core);
    if (result!=pdPASS){
        ESP_LOGE(BT_AV_TAG, "xTaskCreatePinnedToCore");
    } else {
        ESP_LOGI(BT_AV_TAG, "BtI2STask Started");
    }

}

void BluetoothA2DPSinkQueued::bt_i2s_task_shut_down(void)
{
    if (s_bt_i2s_task_handle) {
        vTaskDelete(s_bt_i2s_task_handle);
        s_bt_i2s_task_handle = nullptr;
    }
    if (s_ringbuf_i2s) {
        vRingbufferDelete(s_ringbuf_i2s);
        s_ringbuf_i2s = nullptr;
    }

    ESP_LOGI(BT_AV_TAG, "BtI2STask shutdown");
}

/* NEW I2S Task & ring buffer */

void BluetoothA2DPSinkQueued::i2s_task_handler(void *arg)
{
    uint8_t *data = NULL;
    size_t item_size = 0;

    while (true) {
        /* receive data from ringbuffer and write it to I2S DMA transmit buffer */
        data = (uint8_t *)xRingbufferReceive(s_ringbuf_i2s, &item_size, (portTickType)portMAX_DELAY);

        if (item_size != 0){
            i2s_write_data(data, item_size);
            ESP_LOGD(BT_AV_TAG, "i2s_task_handler->%d",item_size);    
            vRingbufferReturnItem(s_ringbuf_i2s, (void *)data);
        } else {
            ESP_LOGD(BT_AV_TAG, "i2s_task_handler-> no data");    
        }
    }
}

size_t BluetoothA2DPSinkQueued::write_audio(const uint8_t *data, size_t size)
{
    size_t result = size;
    if (s_ringbuf_i2s==nullptr){
        ESP_LOGE(BT_AV_TAG, "s_ringbuf_i2s is null");    
        result = 0;
    }

    BaseType_t rc = xRingbufferSend(s_ringbuf_i2s, (void *)data, size, (portTickType)portMAX_DELAY);
    if (rc==pdFALSE){
        ESP_LOGE(BT_AV_TAG, "xRingbufferSend: %d", size);    
        result = 0;
    } 
    return result;
}