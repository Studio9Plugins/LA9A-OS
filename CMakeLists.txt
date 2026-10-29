# =============================================================================
# LA9A - Levelling Compressor - CMake build script
# Studio9 Plugins
#
# Builds a VST3 (and, on macOS, also AU) plugin using a locally cloned JUCE
# checkout. Works on Windows (Visual Studio / MSVC), Linux (Makefiles /
# Ninja), and macOS (Xcode or Makefiles/Ninja; builds a universal x86_64 +
# arm64 binary - see the CMAKE_OSX_ARCHITECTURES setting below).
#
# --- USAGE ---
#
# You must tell CMake where your local JUCE checkout lives via the JUCE_PATH
# cache variable. Example:
#
#   Linux:
#     cmake -B build -DJUCE_PATH=/home/yourname/JUCE
#     cmake --build build --config Release -j4
#
#   Windows (from a "Developer Command Prompt" or with a generator that
#   matches your installed Visual Studio):
#     cmake -B build -DJUCE_PATH=C:/Dev/JUCE
#     cmake --build build --config Release -j4
#
#   macOS:
#     cmake -B build -DJUCE_PATH=/Users/yourname/JUCE -G Xcode
#     cmake --build build --config Release
#
# On Linux, JUCE's GUI/audio modules need some system dev packages installed
# first.
#
# =============================================================================

cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

    # Universal binary: build both Intel and Apple Silicon into one binary
    # so the plugin loads natively on either Mac without needing separate
    # downloads or Rosetta.
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS" FORCE)
endif()

# Version is the single source of truth for the UI's version label (see
# LA9A_VERSION_STRING compile definition below) — bump it here, nowhere else.
project(LA9A VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Locate JUCE
# -----------------------------------------------------------------------------
# JUCE_PATH must point at the root of a local JUCE checkout (the folder that
# itself contains JUCE's own top-level CMakeLists.txt). Not set automatically
# on purpose - override it on the command line as shown above, or set it here
# permanently if you'd rather hardcode a path for this machine.
if(WIN32)
    set(_juce_path_default "C:/JUCE")
else()
    set(_juce_path_default "$ENV{HOME}/JUCE")
endif()
set(JUCE_PATH "${_juce_path_default}" CACHE PATH "Path to JUCE root (clone or SDK)")

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at:\n  ${JUCE_PATH}\n"
            "Set JUCE_PATH when configuring, e.g.\n"
            "  cmake -B build -DJUCE_PATH=/path/to/JUCE")
endif()

add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")

# -----------------------------------------------------------------------------
# Embedded assets (background skin)
# -----------------------------------------------------------------------------
juce_add_binary_data(LA9AData SOURCES
    Assets/LA9A_Skin.png
    Assets/Knob_128x128_128f.png
    Assets/Switch_100x60_2f.png
    Assets/Switch_AB_100x60_2f.png
)
set_target_properties(LA9AData PROPERTIES POSITION_INDEPENDENT_CODE ON)


option(COPY_PLUGIN_AFTER_BUILD "Copy built plugin(s) to the system plugin folder after build" ON)

set(_la9a_formats VST3)
if(APPLE)
    list(APPEND _la9a_formats AU)
endif()

juce_add_plugin(LA9A
    COMPANY_NAME                "MyCompanyName"  # << Replace with your company name
    PLUGIN_MANUFACTURER_CODE    XXXX             # << Replace with own Manufacturer code
    PLUGIN_CODE                 XXXX             # << Replace with own Plugin code
    FORMATS                     ${_la9a_formats}
    PRODUCT_NAME                "LA9A Compressor"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD     ${COPY_PLUGIN_AFTER_BUILD}
)


target_sources(LA9A PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/LevelingCompressor.cpp
    Source/VerticalMeter.cpp
)

# Feeds the CMake project version into the UI's version label (see
# PluginEditor.cpp) so there is exactly one place to bump it.
target_compile_definitions(LA9A PUBLIC
    LA9A_VERSION_STRING="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LA9A
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        LA9AData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)


