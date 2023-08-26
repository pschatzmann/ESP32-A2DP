# -- CMAKE for Expressive IDF
# -- author Phil Schatzmann
# -- copyright GPLv3

idf_component_register(
    SRCS "src/BluetoothA2DPCommon.cpp" "src/BluetoothA2DPSink.cpp" "src/BluetoothA2DPSource.cpp" "src/BluetoothA2DPSinkQueued.cpp" "src/SoundData.cpp"
    INCLUDE_DIRS "src"
    REQUIRES bt esp_common freertos hal log nvs_flash driver
)

target_compile_options(${COMPONENT_LIB} PUBLIC "-Wno-unused-variable" "-Wno-missing-field-initializers" "-fpermissive")
