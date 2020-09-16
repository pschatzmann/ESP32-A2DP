# A Simple Arduino Bluetooth Music Receiver and Sender for the ESP32

The ESP32 provides a Bluetooth A2DP API that receives sound data e.g. from your Mobile Phone and makes it available via a callback method. The output is a PCM data stream decoded from SBC format. The documentation can be found [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html). 

I2S is an electrical serial bus interface standard used for connecting digital audio devices together. It is used to communicate PCM audio data between integrated circuits in an electronic device.

So we can just feed the input from Bluetooth to the I2S output: An example for this from Expressive can be found on [Github](http://https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/a2dp_sink).

Unfortunately this example did not make me happy so I decided to convert it into a simple __Arduino Library__ that is very easy to use from an Arduino Software IDE.

## A Simple I2S Example (A2DS Sink)
Here is the simplest example which just uses the proper default settings:

```
#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

void setup() {
    a2dp_sink.start("MyMusic");
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
#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

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

    a2dp_sink.set_i2s_config(i2s_config);
    a2dp_sink.start("MyMusic");

}

void loop() {
}
```

The output goes now to the DAC pins G26/G27.


## Sending Data from a A2DS Data Source

We can also generate sound and send it e.g. to a Bluetooth Speaker.  

The supported audio codec in ESP32 A2DP is SBC: A SBC audio stream is encoded from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data. 

When you start the BluetoothA2DPSource, you need to pass the Bluetooth name that you want to connect to and a 'call back
function' that generates the sound data:

```
#include "BluetoothA2DPSource.h"

BluetoothA2DPSource a2dp_source;

// callback 
int32_t get_sound_data(uint8_t *data, int32_t len) {
    // generate your sound data 
    // return the length of the generated sound - which usually is identical with len
    return len;
}

void setup() {
  a2dp_source.start("MyMusic", get_sound_data);  
}

void loop() {
}

```
In the examples you can find an impelentation that generates sound with the help of the sine function.

## Installation

You can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with
```
cd  ~/Documents/Arduino/libraries
git clone pschatzmann/ESP32-A2DP.git
```

## Change History

V.1.1.0 
- New functionality: BluetoothA2DPSource
- Renamed project from 'esp32_bt_music_receiver' to 'ESP32-A2DP'
- Corrected Spelling Mistake from Blootooth to Bluetooth in Class names: The correct class name is BluetoothA2DPSink!
- The include h files are now called like the class names (e.g. BluetoothA2DPSink.h and BluetoothA2DPSource.h)

V.1.0.0 
- Initial Release




