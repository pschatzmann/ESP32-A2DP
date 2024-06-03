# -- CMAKE for Espressif IDF
# -- author Phil Schatzmann
# -- copyright GPLv3

idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "src"
    REQUIRES bt esp_common freertos hal log nvs_flash driver
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -Wno-format -fpermissive)
