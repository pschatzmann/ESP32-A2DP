# -- CMAKE for Espressif IDF
# -- author Phil Schatzmann
# -- copyright GPLv3

idf_component_register(
    SRC_DIRS src
    INCLUDE_DIRS src 
    REQUIRES bt esp_common freertos hal log nvs_flash driver arduino-audio-tools
)

target_compile_options(${COMPONENT_LIB} PUBLIC -DA2DP_LEGACY_I2S_SUPPORT=0 -DA2DP_I2S_AUDIOTOOLS=1 -DESP32_CMAKE=1 -Wno-error -Wno-format -fpermissive)
