cmake_minimum_required(VERSION 3.22)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(EgyptianSampler VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(EgyptianSampler
    COMPANY_NAME "Egyptian"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Egyp
    PLUGIN_CODE Esm1
    FORMATS VST3 Standalone
    VST3_CATEGORIES Instrument Synth
    PRODUCT_NAME "Egyptian Sampler")

target_sources(EgyptianSampler PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp)


# ---- ASIO support (Standalone): downloads the Steinberg ASIO SDK if reachable ----
set(ASIO_ZIP "${CMAKE_BINARY_DIR}/asiosdk.zip")
if(NOT EXISTS "${CMAKE_BINARY_DIR}/asiosdk")
    file(DOWNLOAD "https://download.steinberg.net/sdk_downloads/asiosdk_2.3.3_2019-06-14.zip" "${ASIO_ZIP}" STATUS ASIO_STATUS)
    list(GET ASIO_STATUS 0 ASIO_CODE)
    if(ASIO_CODE EQUAL 0)
        file(ARCHIVE_EXTRACT INPUT "${ASIO_ZIP}" DESTINATION "${CMAKE_BINARY_DIR}/asiosdk")
    endif()
endif()
file(GLOB ASIO_ROOT "${CMAKE_BINARY_DIR}/asiosdk/*")
list(GET ASIO_ROOT 0 ASIO_ROOT)
if(EXISTS "${ASIO_ROOT}/common/iasiodrv.h")
    message(STATUS "ASIO SDK found: ${ASIO_ROOT}")
    set(EGY_ASIO 1)
    target_include_directories(EgyptianSampler PRIVATE "${ASIO_ROOT}/common" "${ASIO_ROOT}/host" "${ASIO_ROOT}/host/pc")
else()
    message(WARNING "ASIO SDK not found - building without ASIO")
    set(EGY_ASIO 0)
endif()
target_compile_definitions(EgyptianSampler PUBLIC JUCE_ASIO=${EGY_ASIO}
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(EgyptianSampler
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
