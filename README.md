# A Simple Arduino Bluetooth Music Receiver and Sender for the ESP32

The ESP32 provides a Bluetooth A2DP API that receives sound data e.g. from your Mobile Phone and makes it available via a callback method. The output is a PCM data stream decoded from SBC format. The documentation can be found [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html). 

I2S is an electrical serial bus interface standard used for connecting digital audio devices together. It is used to communicate PCM audio data between integrated circuits in an electronic device.

So we can just feed the input from Bluetooth to the I2S output: An example for this from Expressive can be found on [Github](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/a2dp_sink).

Unfortunately this example did not make me happy so I decided to convert it into a simple __Arduino Library__ that is very easy to use from an Arduino Software IDE.

## A2DP Sink

### A Simple I2S Example (A2DS Sink) using default Pins
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
This creates a new Bluetooth device with the name “MyMusic” and the output will be sent to the following default I2S pins which need to be conected to an external DAC:
- bck_io_num = 26
- ws_io_num = 25
- data_out_num = 22


### Defining Pins

You can define your own pins easily by calling the ```set_pin_config``` method in the setup before the ```start```.

```
#include "BluetoothA2DPSink.h"

BluetoothA2DPSink a2dp_sink;

void setup() {
    i2s_pin_config_t my_pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 22,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(my_pin_config);
    a2dp_sink.start("MyMusic");
}

void loop() {
}
```


### Output to the Internal DAC
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

The output goes now to the DAC pins GPIO25 (Channel 1) and GPIO26 (Channel 2).

### Accessing the Sink Data Stream with Callbacks

You can be notified when a packet is received:

```
// In the setup function:
a2dp_sink.set_on_data_received(data_received_callback);


// Then somewhere in your sketch:
void data_received_callback() {
  Serial.println("Data packet received");
}
```

Or you can access the packet:

```
// In the setup function:
a2dp_sink.set_stream_reader(read_data_stream);

// Then somewhere in your sketch:
void read_data_stream(const uint8_t *data, uint32_t length)
{
  // Do something with the data packet
}
```
In the ```a2dp_sink.set_stream_reader()``` method you can provide an optional parameter that defines if you want the output to I2S to be active or deactive - So you can use this method to e.g. to switch off I2S just by calling ```a2dp_sink.set_stream_reader(read_data_stream, false)```

### Support for Metadata

You can register a method which will be called when the system receives any AVRC metadata. Here is an example
```
void avrc_metadata_callback(uint8_t data1, const uint8_t *data2) {
  Serial.printf("AVRC metadata rsp: attribute id 0x%x, %s\n", data1, data2);
}
a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
a2dp_sink.start("BT");
```

### Support for AVRC Commands

I have added the following AVRC commmands, that you can use to 'control' your A2DP Source:

- play();
- pause();
- stop();
- next();
- previous();


## A2DP Source

### Sending Data from a A2DS Data Source with a Callback

We can also generate sound and send it e.g. to a Bluetooth Speaker.  

The supported audio codec in ESP32 A2DP is SBC: A SBC audio stream is encoded from PCM data normally formatted as 44.1kHz sampling rate, two-channel 16-bit sample data. 

When you start the BluetoothA2DPSource, you need to pass the Bluetooth name that you want to connect to and a 'call back
function' that generates the sound data:

```
#include "BluetoothA2DPSource.h"

BluetoothA2DPSource a2dp_source;

// callback 
int32_t get_sound_data(Channels *data, int32_t len) {
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

In the examples you can find an implentation that generates sound with the help of the sin() function.
You can also inticate multiple alternative Bluetooth names. The system just connects to the first one
which is available:

```
void setup() {
  static std::vector<char*> bt_names = {"MyMusic","RadioPlayer","MusicPlayer"};
  a2dp_source.start(bt_names, get_sound_data); 
} 

```


### Sending Data from a A2DS Data Source with Recorded Data

You can also provide the data directly as simple array of uint8_t:

```
#include "BluetoothA2DPSource.h"

extern const uint8_t StarWars10_raw[];
extern const unsigned int StarWars10_raw_len;

BluetoothA2DPSource a2dp_source;
SoundData *music = new OneChannelSoundData((int16_t*)StarWars30_raw, StarWars30_raw_len/2);

void setup() {
  a2dp_source.start("RadioPlayer");  
  a2dp_source.writeData(music);
}

void loop() {
}

```

The array can be prepared e.g. in the following way:

  - Open any sound file in Audacity. 
    - Select Tracks -> Resample and select 44100
    - Export -> Export Audio -> Header Raw ; Signed 16 bit PCM
  - Convert to c file e.g. with "xxd -i file_example_WAV_1MG.raw file_example_WAV_1MG.c"
    - add the const qualifier to the generated array definition. E.g const unsigned char file_example_WAV_1MG_raw[] = {

You might want to compile with the Partition Scheme: Huge App!

In the example above we provide the data with one channel. This has the advantage that it uses much less space then 
a 2 channel recording, which you could use in the following way: 

```
SoundData *data = new TwoChannelSoundData((Channels*)StarWars10_raw,StarWars10_raw_len/4);

```

In the constructor you can pass additional parameters:

```
TwoChannelSoundData(Channels *data, int32_t len, bool loop=false);
OneChannelSoundData(int16_t *data, int32_t len, bool loop=false, ChannelInfo channelInfo=Both);

```

## Architecture / Dependencies  

The current code is purely dependent on the ESP-IDF (which is also provided by the Arduino ESP32 core). There are no other dependencies and this includes the Arduino API! 

This restriction limits the provided examples. Please have a look at the [audio-tools](https://github.com/pschatzmann/arduino-audio-tools) project: There I demonstrate e.g. how to connect different microphones, use SD cards and different Audio File Formats.


## Documentation

The class documentation can be found [here](https://pschatzmann.github.io/ESP32-A2DP/doc/html/annotated.html)


## Installation

You can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with
```
cd  ~/Documents/Arduino/libraries
git clone pschatzmann/ESP32-A2DP.git
```

If you are using a current version of ESP IDF, you will receive compile errors like `'ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE' was not declared in this scope`. To use the new API you can uncomment `#define CURRENT_ESP_IDF` in `src/BluetoothA2DPCommon.h`



## Change History

Master
- provide get_connection_status() method
- provide end() method to shut down bluetooth
- add compile time switch for new ESP IDF
- provide set_discoverability() to change BT discovery mode (only for current ESP IDF)
- add additional flag in set_stream_reader to deactivate i2s 
- Remove includes of Arduino.h to highlight independence of Arduino API
- New examples with LED and auto shut down on idle
- Implement mono downmix
- Clean up compile warnings so that build with warnings ALL will succeed
- Example with set_pin_config
- bluetooth devices with address starting 00 did not auto reconnect
- expand to 24 or 32 bits using i2s_config.bits_per_sample
- Correction which allows restart after calling end() thanks to [alexus2033](https://github.com/alexus2033)

V.1.2.0
- Metadata support with the help of a callback function - Thanks to [JohnyMielony](https://github.com/JohnyMielony)
- AVRC command support thanks to [PeterPark](https://github.com/KIdon-Park) 
- Improved init_bluetooth checks, in case bluedroid was already initialized elsewhere - Thanks to [Antonis Katzourakis](https://github.com/ant0nisk)
- No auto reconnect after clean disconnect - Thanks to [Bajczi Levente](https://github.com/leventeBajczi)
- The data is rescaled to when written to the internal DAC - Thanks to [Martin Hron](https://github.com/thinkcz)
- Corrected wrong case of include to Arduino.h - Thanks to [RyanDavis](https://github.com/RyanDavis)
- Added callback to received packets - Thanks to [Mishaux](https://github.com/Mishaux)
- Automatically reconnect to last source - Thanks to [JohnyMielony](https://github.com/JohnyMielony)
- Support for data callback - Thanks to [Mike Mishaux](https://github.com/Mishaux)
- Improved init_bluetooth checks, in case bluedroid was already initialized elsewhere [Antonis Katzourakis](https://github.com/ant0nisk)
- No auto reconnect after clean disconnect thanks to [Bajczi Levente](https://github.com/leventeBajczi)
- Made all methods virtual to enable flexible subclassing
- Error Corrections in BluetoothA2DPSource
- Support for writeData in BluetoothA2DPSource
- Support for multiple alternative BT names in BluetoothA2DPSource
- Redesign to protect internal callbacks in BluetoothA2DPSink
- Generate Documentation with the help of doxygen

V.1.1.0 
- New functionality: BluetoothA2DPSource
- Renamed project from 'esp32_bt_music_receiver' to 'ESP32-A2DP'
- Corrected Spelling Mistake from Blootooth to Bluetooth in Class names: The correct class name is BluetoothA2DPSink!
- The include h files are now called like the class names (e.g. BluetoothA2DPSink.h and BluetoothA2DPSource.h)

V.1.0.0 
- Initial Release




