# -- CMAKE for Espressif IDF
# -- author Phil Schatzmann
# -- copyright GPLv3

# esp_ringbuf needed starting idf v6.0.0
set(A2DP_REQUIRES bt esp_common freertos hal log nvs_flash driver esp_ringbuf arduino-audio-tools)

# codec-helix (AAC decode) + codec-sbc (SBC decode, kept alongside
# AAC since registering any managed decoder via add_decoder() replaces the
# built-in SBC decode path entirely - see examples/bt_music_receiver_codec)
# only work starting idf v6.1, which is where the Bluedroid stack gained
# multi-SEP / external-codec Kconfig support (BT_A2DP_CODEC_AAC_ENABLED,
# BT_A2DP_SEP_NUM_MAX). Each one is added to REQUIRES only if it is actually
# present as a component (e.g. pulled in via idf_component.yml into
# managed_components/, or placed in components/) so the build doesn't fail
# for users who only installed one of the two, or neither.
if(IDF_VERSION_MAJOR GREATER 6 )
    idf_build_get_property(a2dp_component_dirs COMPONENT_DIRS)
    foreach(a2dp_codec codec-helix codec-sbc codec-faad codec-fdk-aac)
        foreach(a2dp_component_dir ${a2dp_component_dirs})
            if(EXISTS "${a2dp_component_dir}/${a2dp_codec}")
                list(APPEND A2DP_REQUIRES ${a2dp_codec})
                break()
            endif()
        endforeach()
    endforeach()
endif()

idf_component_register(
    SRC_DIRS src
    INCLUDE_DIRS src
    REQUIRES ${A2DP_REQUIRES}
)

target_compile_options(${COMPONENT_LIB} PUBLIC -DA2DP_LEGACY_I2S_SUPPORT=0 -DA2DP_I2S_AUDIOTOOLS=1)
