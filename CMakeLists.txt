# -- CMAKE for Espressif IDF
# -- author Phil Schatzmann
# -- copyright GPLv3

# esp_ringbuf needed starting idf v6.0.0
set(A2DP_REQUIRES bt esp_common freertos hal log nvs_flash driver esp_ringbuf arduino-audio-tools)

# arduino-libhelix (AAC decode) + arduino-libsbc (SBC decode, kept alongside
# AAC since registering any managed decoder via add_decoder() replaces the
# built-in SBC decode path entirely - see examples/bt_music_receiver_codec)
# are only wired up starting idf v6.2, which is where the Bluedroid stack
# gained multi-SEP / external-codec Kconfig support (BT_A2DP_CODEC_AAC_ENABLED,
# BT_A2DP_SEP_NUM_MAX).
if(IDF_VERSION_MAJOR GREATER 6 OR (IDF_VERSION_MAJOR EQUAL 6 AND IDF_VERSION_MINOR GREATER_EQUAL 2))
    list(APPEND A2DP_REQUIRES arduino-libhelix arduino-libsbc)
endif()

idf_component_register(
    SRC_DIRS src
    INCLUDE_DIRS src
    REQUIRES ${A2DP_REQUIRES}
)

target_compile_options(${COMPONENT_LIB} PUBLIC -DA2DP_LEGACY_I2S_SUPPORT=0 -DA2DP_I2S_AUDIOTOOLS=1)
