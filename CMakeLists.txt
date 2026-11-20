cmake_minimum_required(VERSION 3.22)
project(NFBrutal VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS")
# JUCE comes as the ./JUCE submodule (git submodule update --init). Fallback for the owner's Mac: the copy used by NF Capture.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
 add_subdirectory(JUCE)
else()
 add_subdirectory($ENV{HOME}/NF-Capture/JUCE JUCE)
endif()
set(formats VST3)
if(APPLE)
 list(APPEND formats AU)
endif()
# Licence lock: ON by default (key from the licence site); -DBRUTAL_LICENSE=OFF builds a development version without it.
option(BRUTAL_LICENSE "Require a licence key" ON)
# cross-compiling for Windows on the Mac: the VST3 manifest helper is a Windows program and cannot run here, so skip it
set(brutal_manifest TRUE)
if(CMAKE_CROSSCOMPILING AND WIN32)
 set(brutal_manifest FALSE)
endif()
juce_add_plugin(NFBrutal VST3_AUTO_MANIFEST ${brutal_manifest} COMPANY_NAME "NF Audio Tools" BUNDLE_ID "com.nfaudiotools.nfbrutal"
 PLUGIN_MANUFACTURER_CODE Nfat PLUGIN_CODE Nfbr FORMATS ${formats} PRODUCT_NAME "NF Brutal"
 IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE COPY_PLUGIN_AFTER_BUILD FALSE AU_MAIN_TYPE kAudioUnitType_Effect)
juce_add_binary_data(NFBrutalAssets HEADER_NAME NFBrutalBinaryData.h NAMESPACE NFBrutalBinaryData SOURCES Assets/Logo/NF_Audio_Tools_Logo.png)
target_sources(NFBrutal PRIVATE Source/PluginProcessor.cpp)
target_compile_definitions(NFBrutal PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 BRUTAL_LICENSE_ENFORCE=$<BOOL:${BRUTAL_LICENSE}>)
target_link_libraries(NFBrutal PRIVATE NFBrutalAssets juce::juce_audio_utils juce::juce_cryptography PUBLIC juce::juce_recommended_config_flags)
# UI preview without a host: build/NFBrutalShot_artefacts/Release/NFBrutalShot out.png [width] [grDb] [slay]
juce_add_console_app(NFBrutalShot PRODUCT_NAME "NFBrutalShot")
target_sources(NFBrutalShot PRIVATE Source/Shot.cpp Source/PluginProcessor.cpp)
target_compile_definitions(NFBrutalShot PRIVATE NFBRUTAL_NO_ENTRY=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 BRUTAL_LICENSE_ENFORCE=$<BOOL:${BRUTAL_LICENSE}>)
target_link_libraries(NFBrutalShot PRIVATE NFBrutalAssets juce::juce_audio_utils juce::juce_cryptography juce::juce_recommended_config_flags)
