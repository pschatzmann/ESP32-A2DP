# -- CMAKE for Expressive IDF
# -- author Phil Schatzmann
# -- copyright GPLv3

cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)

project(esp32_a2dp)

# Compile To Library
add_library(esp32_a2dp ${CMAKE_CURRENT_SOURCE_DIR}/src/BluetoothA2DPSink.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/BluetoothA2DPSource.cpp)
target_include_directories(esp32_a2dp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

