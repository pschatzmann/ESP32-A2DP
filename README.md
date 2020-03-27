# A Simple Arduino Bluetooth Music Receiver for the ESP32

The ESP32 provides a Bluetooth A2DP API that receives sound data e.g. from your Mobile Phone and makes it available via a callback method. The output is a PCM data stream decoded from SBC format. The documentation can be found [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html). 

I2S is an electrical serial bus interface standard used for connecting digital audio devices together. It is used to communicate PCM audio data between integrated circuits in an electronic device.

So we can just feed the input from Bluetooth to the I2S output: An example for this from Expressive can be found on [Github](http://https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/a2dp_sink).

Unfortunately this example did not make me happy so I decided to convert it into a simple C++ class that is very easy to use from an Arduino Software IDE.

## A Simple I2S Example
Here is the simplest example which just uses the proper default settings:

```
#include <arduino.h>
#include "esp32_bt_music_receiver.h"

BlootoothA2DSink a2d_sink;

void setup() {
    a2d_sink.start("MyMusic");
}

void loop() {
}
```
This creates a new Bluetooth device with the name “MyMusic” and the output will be sent to the following default I2S pins:
– bck_io_num = 26,
– ws_io_num = 25,
– data_out_num = 22,

which need to be conected to an external DAC. You can define your own pins easily by calling the set_pin_config method.

## Output to the Internal DAC
You can also send the output directly to the internal DAC of the ESP32 by providing the corresponding i2s_config:

```
#include <arduino.h>
#include "esp32_bt_music_receiver.h"

BlootoothA2DSink a2d_sink;

void setup() {
    static const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = 44100, // corrected by info from bluetooth
        .bits_per_sample = (i2s_bits_per_sample_t) 16, /* the DAC module will only take the 8bits from MSB */
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };

    a2d_sink.set_i2s_config(i2s_config);
    a2d_sink.start("MyMusic");

}

void loop() {
}
```

The output goes now to the DAC pins G26/G27.

## Installation
You can download the library as zip and call include Library -> zip library.
Or you can git clone this project into the Arduino libraries folder
