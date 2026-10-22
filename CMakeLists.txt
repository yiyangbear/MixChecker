cmake_minimum_required(VERSION 3.22)

project(MixChecker VERSION 0.1.0 LANGUAGES C CXX OBJC OBJCXX)

include(FetchContent)

find_package(JUCE 9.0.3 EXACT CONFIG QUIET)

if(NOT JUCE_FOUND)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MixChecker
    COMPANY_NAME "Mix Checker"
    BUNDLE_ID "com.mixchecker.plugin"
    PLUGIN_MANUFACTURER_CODE MxCk
    PLUGIN_CODE MxCh
    FORMATS AU VST3
    PRODUCT_NAME "Mix Checker"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_add_binary_data(MixCheckerUIAssets
    SOURCES
        Resources/HardwareUI/faceplate_3x.png
        Resources/HardwareUI/mode_up_3x.png
        Resources/HardwareUI/mode_down_3x.png
        Resources/HardwareUI/bypass_up_3x.png
        Resources/HardwareUI/bypass_down_3x.png
        Resources/HardwareUI/lamp_off_3x.png
        Resources/HardwareUI/lamp_on_3x.png
        Resources/HardwareUI/screw_3x.png
)

target_sources(MixChecker
    PRIVATE
        Source/DspEngine.cpp
        Source/DspEngine.h
        Source/DspPresets.cpp
        Source/DspPresets.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
)

target_compile_features(MixChecker PRIVATE cxx_std_17)

target_compile_definitions(MixChecker
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MixChecker
    PRIVATE
        MixCheckerUIAssets
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(APPLE)
    add_custom_command(TARGET MixChecker_AU POST_BUILD
        COMMAND /usr/bin/codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:MixChecker_AU>"
        COMMENT "Applying an ad-hoc signature to the Audio Unit bundle"
    )

    add_custom_command(TARGET MixChecker_VST3 POST_BUILD
        COMMAND /usr/bin/codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:MixChecker_VST3>"
        COMMENT "Applying an ad-hoc signature to the VST3 bundle"
    )
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(MixCheckerPassThroughTest Tests/PassThroughTest.cpp)

    target_include_directories(MixCheckerPassThroughTest PRIVATE Source)
    target_compile_features(MixCheckerPassThroughTest PRIVATE cxx_std_17)
    target_link_libraries(MixCheckerPassThroughTest
        PRIVATE
            MixChecker
            juce::juce_audio_processors
            juce::juce_dsp
    )

    add_test(NAME MixChecker.PassThrough COMMAND MixCheckerPassThroughTest)
endif()
