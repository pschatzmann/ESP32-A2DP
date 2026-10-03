# A Simple ESP32 Bluetooth A2DP Audio Library

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![IDF Component](https://img.shields.io/badge/IDF-Component-blue.svg)](https://github.com/pschatzmann/ESP32-A2DP)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0)


The ESP32 is a microcontroller that provides an API for Bluetooth A2DP which can be used to receive sound data e.g. from your Mobile Phone and makes it available via a callback method. The output is a PCM data stream, decoded from SBC format. The documentation can be found [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html). 

![esp32](https://pschatzmann.github.io/ESP32-A2DP/img/esp32.jpeg)

I2S is an electrical serial bus interface standard used for connecting digital audio devices together. It is used to communicate PCM audio data between integrated circuits in an electronic device.

So we can just feed the input from Bluetooth to the I2S output: An example for this from Espressif can be found on [Github](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/a2dp_sink_stream).

Unfortunately this example did not make me happy so I decided to convert it into a simple __Arduino Library__ that is very easy to use from an Arduino Software IDE.

## Supported Bluetooth Protocols

As the name of this library implies, it supports the A2DP [Bluetooth protocol](https://en.wikipedia.org/wiki/List_of_Bluetooth_profiles) which only provides audio streaming! 

It also supports Audio/Video Remote Control Profile (AVRCP) together with A2DP.

The Hands-Free Profile (HFP), Headset Profile (HSP) and standalone AVRCP without A2DP are __not__ supported!

## I2S API / Dependencies

Espressif is retiring the legacy I2S API: So with Arduino v3.0.0 (IDF v5) [my old I2S integration](https://github.com/pschatzmann/ESP32-A2DP/wiki/Legacy-I2S-API) will not be available any more. The legacy syntax is still working as long as you don't upgrade.  

In order to support a unique output API which is version independent, it is recommended to install and use the [AudioTools](https://github.com/pschatzmann/arduino-audio-tools) library. So the documentation and all the examples have been updated to use this new approach.

However you can also output to any other class which inherits from Arduino Print: e.g. the [Arduino ESP32 I2SClass](https://github.com/pschatzmann/ESP32-A2DP/wiki/A2DP-Sink#output-using-the-esp32-i2s-api) or you can use the [data callback](https://github.com/pschatzmann/ESP32-A2DP/wiki/A2DP-Sink#accessing-the-sink-data-stream-with-callbacks). 


## A2DP Sink (Music Receiver)

This can be used e.g. to build your own Bluetooth Speaker. Here is the simplest example which just uses the default settings:

```cpp
#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void setup() {
    a2dp_sink.start("MyMusic");
}

void loop() {
}
```
This creates a new Bluetooth device with the name “MyMusic” and the output is sent to the default I2S pins (BCK = 14, WS = 15, DATA = 22), which need to be connected to an external DAC.

Defining your own pins, the output with the ESP32 I2S API or to the internal DAC, data callbacks, metadata and AVRC commands are described in the [A2DP Sink documentation](https://github.com/pschatzmann/ESP32-A2DP/wiki/A2DP-Sink).

## A2DP Source (Music Sender)

This can be used to feed e.g. your Bluetooth Speaker with your audio data: PCM data with 44.1kHz sampling rate, two channels and 16 bits per sample.

```cpp
#include "BluetoothA2DPSource.h"

BluetoothA2DPSource a2dp_source;

int32_t get_sound_data(uint8_t *data, int32_t byteCount) {
    // generate your sound data
    // return the effective length in bytes
    return byteCount;
}

void setup() {
  a2dp_source.set_data_callback(get_sound_data);
  a2dp_source.start("MyMusic");
}

void loop() {}
```

Frame callbacks, Arduino Streams as data source and the selection of the target device are described in the [A2DP Source documentation](https://github.com/pschatzmann/ESP32-A2DP/wiki/A2DP-Source).

## Logging

This library uses the ESP32 logger that you can activate in Arduino in - Tools - Core Debug Log.

## Architecture / Dependencies 

The current code is purely dependent on the ESP-IDF (which is also provided by the Arduino ESP32 core). There are no other dependencies and this includes the Arduino API! 

Therefore we support:

- Arduino
- [PlatformIO](https://github.com/pschatzmann/ESP32-A2DP/wiki/PlatformIO)
- [Espressif IDF](https://github.com/pschatzmann/ESP32-A2DP/wiki/Espressif-IDF-as-a-Component)

This restriction limits however the provided examples. 

Before you clone the project, please read the following information which can be found in the [Wiki](https://github.com/pschatzmann/ESP32-A2DP/wiki/Design-Overview).

## Digital Sound Processing

This library is part of my [AudioTools](https://github.com/pschatzmann/arduino-audio-tools) project, so you can combine it with any AudioTools functionality:

- __A2DP Sink__: process the received audio with an equalizer, filters or sound effects, analyse it with FFT, or send it to an alternative output.
- __A2DP Source__: send audio from any AudioTools source, e.g. MP3 files, a microphone or a synthesizer.

You can find many [examples in the AudioTools project](https://github.com/pschatzmann/arduino-audio-tools/tree/main/examples/examples-communication/a2dp).

## Documentation

- The [class documentation](https://pschatzmann.github.io/ESP32-A2DP/html/group__a2dp.html) can be found here
- You can also find further information in the [Wiki](https://github.com/pschatzmann/ESP32-A2DP/wiki)
- The [Change History can be found in the Wiki](https://github.com/pschatzmann/ESP32-A2DP/wiki/Change-History)


## Support

I spent a lot of time to provide a comprehensive and complete documentation.
So please read the documentation first and check the issues and discussions before posting any new ones on Github.

Open __issues only for bugs__ and if it is not a bug, use a discussion: Provide enough information about 
- the selected scenario/sketch 
- what exactly you are trying to do
- your hardware
- your software versions
  - ESP32 version from the Board Manager
  - version of the ESP32-A2DP library

to enable others to understand and reproduce your issue.

Finally above all __don't__ send me any e-mails or post questions on my personal website! 

## Show and Tell

Get some inspiration [from projects that were using this library](https://github.com/pschatzmann/ESP32-A2DP/discussions/categories/show-and-tell) and share your projects with the community.

## Installation

For Arduino you can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with
```bash
cd  ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/ESP32-A2DP.git
git clone https://github.com/pschatzmann/arduino-audio-tools.git
```
For the provided examples, you will need to install the [audio-tools library](https://github.com/pschatzmann/arduino-audio-tools) as well. 

For other frameworks [see the Wiki](https://github.com/pschatzmann/ESP32-A2DP/wiki)


## Sponsor Me

This software is totally free, but you can make me happy by rewarding me with a treat

- [Buy me a coffee](https://www.buymeacoffee.com/philschatzh)
- [Paypal me](https://paypal.me/pschatzmann?country.x=CH&locale.x=en_US)

