cmake_minimum_required(VERSION 3.22)
project(ZClip VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- JUCE location ----
# Point this to your JUCE source (you said it's C:/JUCE)
if(NOT DEFINED JUCE_DIR)
  set(JUCE_DIR "C:/JUCE")
endif()

# ---- Bring in JUCE ----
add_subdirectory("${JUCE_DIR}" ${CMAKE_BINARY_DIR}/juce_build EXCLUDE_FROM_ALL)

# ---- Plugin target ----
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

# ---- Sources ----
file(GLOB_RECURSE ZCLIP_SRC CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_SOURCE_DIR}/Source/*.h"
)
target_sources(ZClip PRIVATE ${ZCLIP_SRC})

# ---- Link JUCE modules ----
target_link_libraries(ZClip PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp            # needed for dsp::Oversampling
)

# ---- Defines / options ----
target_compile_definitions(ZClip PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# ---- Copy destination for VST3 after build ----
set_target_properties(ZClip PROPERTIES
    JUCE_VST3_COPY_DIR "C:/Program Files/Common Files/VST3/Product Zero"
)

# ---- (Optional) warnings/tooling ----
if(MSVC)
    target_compile_options(ZClip PRIVATE /W4 /permissive-)
endif()