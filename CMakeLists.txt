# -- CMAKE for Espressif IDF
# -- author Phil Schatzmann
# -- copyright GPLv3

idf_component_register(
    SRC_DIRS src
    INCLUDE_DIRS src 
#   esp_ringbuf needed starting idf v6.0.0, see
    REQUIRES bt esp_common freertos hal log nvs_flash driver esp_ringbuf arduino-audio-tools  
)

target_compile_options(${COMPONENT_LIB} PUBLIC -DA2DP_LEGACY_I2S_SUPPORT=0 -DA2DP_I2S_AUDIOTOOLS=1)
