cmake_minimum_required(VERSION 3.22)
project(ZClip VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED JUCE_DIR)
  set(JUCE_DIR "C:/JUCE")
endif()

add_subdirectory("${JUCE_DIR}" ${CMAKE_BINARY_DIR}/juce_build EXCLUDE_FROM_ALL)

juce_add_plugin(ZClip
    COMPANY_NAME "Product Zero"
    BUNDLE_ID com.productzero.zclip
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE PZrz
    PLUGIN_CODE Zclp
    FORMATS VST3
    PRODUCT_NAME "ZClip"
)

file(GLOB_RECURSE ZCLIP_SRC CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_SOURCE_DIR}/Source/*.h"
)
target_sources(ZClip PRIVATE ${ZCLIP_SRC})

target_link_libraries(ZClip PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)

target_compile_definitions(ZClip PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

set_target_properties(ZClip PROPERTIES
    JUCE_VST3_COPY_DIR "C:/Program Files/Common Files/VST3/Product Zero"
)

if(MSVC)
    target_compile_options(ZClip PRIVATE /W4 /permissive-)
endif()