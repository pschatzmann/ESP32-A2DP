# -- CMAKE for Espressif IDF
# -- author Phil Schatzmann
# -- copyright GPLv3

idf_component_register(
    SRC_DIRS src
    INCLUDE_DIRS src 
    REQUIRES bt esp_common freertos hal log nvs_flash driver
)

target_compile_options(${COMPONENT_LIB} PUBLIC -DA2DP_LEGACY_I2S_SUPPORT=1 -DA2DP_I2S_AUDIOTOOLS=0 -Wno-error -Wno-format -fpermissive)
