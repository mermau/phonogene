cmake_minimum_required(VERSION 3.22)

project(PHONOGENE VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: usa una copia locale se JUCE_DIR è indicato, altrimenti la scarica.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.10
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Phonogene
    COMPANY_NAME "Collateral Effects"
    COMPANY_CODE Clfx
    PLUGIN_MANUFACTURER_CODE Clfx
    PLUGIN_CODE Phng
    PRODUCT_NAME "Phonogene"
    BUNDLE_ID com.collateraleffects.phonogene
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Sampler
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU Standalone
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Il Phonogene registra l'ingresso audio sull'anello di nastro.")

target_sources(Phonogene PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/TapeEngine.cpp)

target_compile_definitions(Phonogene PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Phonogene
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Strumento di verifica (non incluso nelle build di rilascio)
option(PHONOGENE_TESTS "Build offline test/snapshot tool" OFF)
if(PHONOGENE_TESTS)
    juce_add_console_app(PhonogeneTest PRODUCT_NAME "PhonogeneTest")
    target_sources(PhonogeneTest PRIVATE Tests/TestMain.cpp)
    target_compile_definitions(PhonogeneTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_include_directories(PhonogeneTest PRIVATE $<TARGET_PROPERTY:Phonogene,INCLUDE_DIRECTORIES>)
    target_compile_definitions(PhonogeneTest PRIVATE $<TARGET_PROPERTY:Phonogene,COMPILE_DEFINITIONS>)
    target_link_libraries(PhonogeneTest PRIVATE Phonogene juce::juce_recommended_config_flags)
endif()
