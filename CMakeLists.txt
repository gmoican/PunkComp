# 3.24.1 is bundled in Visual Studio 2022 v17.4
# 3.24.1 is also bundled in CLion as of 2023
cmake_minimum_required(VERSION 3.24.1)

# # #

### Reads in our VERSION file and sticks in it CURRENT_VERSION variable
# Be sure the file has no newlines!
# This exposes CURRENT_VERSION to the build system
# And it's later fed to JUCE so it shows up as VERSION in your IDE
file(STRINGS VERSION CURRENT_VERSION)

# Figure out the major version to append to our PROJECT_NAME
string(REGEX MATCH "([0-9]+)" MAJOR_VERSION ${CURRENT_VERSION})
message(STATUS "Major version: ${MAJOR_VERSION}")

# # #

### Configures universal binaries and decides which version of macOS to support
# This must be set before the project() call
# see: https://cmake.org/cmake/help/latest/variable/CMAKE_OSX_DEPLOYMENT_TARGET.html
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Support macOS down to High Sierra")

# Building universal binaries on macOS increases build time
# This is set on CI but not during local dev
if ((DEFINED ENV{CI}) AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
    message("Building for Apple Silicon and x86_64")
    set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)
endif ()

# By default we don't want Xcode schemes to be made for modules, etc
set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# # #

# Change me!
# This is the internal name of the project and the name of JUCE's shared code target
set(PROJECT_NAME "PunkKomp")

# Change me!
# JUCE's PRODUCT_NAME is what DAWs will display
set(PRODUCT_NAME "Punk Komp")

# Change me!
# Used for the MacOS bundle name and Installers
set(COMPANY_NAME "punkarra4")

# Change me!
# Used for the MacOS bundle identifier (and signing)
set(BUNDLE_ID "com.punkarra4.PunkKomp")

# Set the plugin formats you want built
# Valid choices: AAX Unity VST VST3 AU AUv3 Standalone
set(FORMATS AU VST3)

# For simplicity, the name of the CMake project is also the name of the target
project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

# # #

### Couple tweaks that IMO should be JUCE defaults
# Adds all the module sources so they appear correctly in the IDE
# Must be set before JUCE is added as a sub-dir (or any targets are made)
# https://github.com/juce-framework/JUCE/commit/6b1b4cf7f6b1008db44411f2c8887d71a3348889
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# Creates a /Modules directory in the IDE with the JUCE Module code
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Show all module sources in IDE projects" ON)

# Color our warnings and errors
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif ()

# # #

# JUCE is setup as a submodule in the /JUCE folder
# Locally, you must run `git submodule update --init --recursive` once
# and later `git submodule update --remote --merge` to keep it up to date
# On Github Actions, this is done as a part of actions/checkout
add_subdirectory(JUCE)

# Add any other modules you want modules here, before the juce_add_plugin call
# juce_add_module(modules/my_module)

# See `docs/CMake API.md` in the JUCE repo for all config options
juce_add_plugin("${PROJECT_NAME}"
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"

    # On MacOS, plugin is copied to ~/Users/yourname/Library/Audio/Plug-Ins/
    COPY_PLUGIN_AFTER_BUILD TRUE

    # Change me!
    # A four-character plugin id
    # First character MUST be uppercase for AU formats
    PLUGIN_MANUFACTURER_CODE Koji

    # Change me!
    # A unique four-character plugin id
    # Note: this must have at least one upper-case character
    PLUGIN_CODE K182
    FORMATS "${FORMATS}"

    # The name of your final executable
    # This is how it's listed in the DAW
    # This can be different from PROJECT_NAME and can have spaces!
    # You might want to use v${MAJOR_VERSION} here once you go to v2...
    PRODUCT_NAME "${PRODUCT_NAME}")

# This lets us use our code in both the JUCE targets and our Test target
# Without running into ODR violations
add_library(SharedCode INTERFACE)

# C++20, please
# Use cxx_std_23 for C++23 (as of CMake v 3.20)
target_compile_features(SharedCode INTERFACE cxx_std_20)

# # #

# Manually list all .h and .cpp files for the plugin
# If you are like me, you'll use globs for your sanity.
# Just ensure you employ CONFIGURE_DEPENDS so the build system picks up changes
# If you want to appease the CMake gods and avoid globs, manually add files like so:
# set(SourceFiles Source/PluginEditor.h Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginProcessor.cpp)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" 
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

# # #

### Adds a BinaryData target for embedding assets into the binary
# HEADS UP: Pamplejuce assumes anything you stick in the assets folder you want to included in your binary!
# This makes life easy, but will bloat your binary needlessly if you include unused files
file(GLOB_RECURSE AssetFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

# Setup our binary data as a target called Assets
juce_add_binary_data(Assets SOURCES ${AssetFiles})

# Required for Linux happiness:
# See https://forum.juce.com/t/loading-pytorch-model-using-binarydata/39997/2
set_target_properties(Assets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

### MacOS only: Cleans up folder and target organization on Xcode.
# No, we don't want our source buried in extra nested folders
set_target_properties(SharedCode PROPERTIES FOLDER "")

# The Xcode source tree should uhhh, still look like the source tree, yo
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/source PREFIX "" FILES ${SourceFiles})

# It tucks the Plugin varieties into a "Targets" folder and generate an Xcode Scheme manually
# Xcode scheme generation is turned off globally to limit noise from other targets
# The non-hacky way of doing this is via the global PREDEFINED_TARGETS_FOLDER property
# However that doesn't seem to be working in Xcode
# Not all plugin types (au, vst) available on each build type (win, macos, linux)
foreach (target ${FORMATS} "All")
    if (TARGET ${PROJECT_NAME}_${target})
        set_target_properties(${PROJECT_NAME}_${target} PROPERTIES
            # Tuck the actual plugin targets into a folder where they won't bother us
            FOLDER "Targets"
            # Let us build the target in Xcode
            XCODE_GENERATE_SCHEME ON)

        # Set the default executable that Xcode will open on build
        # Note: you must manually build the AudioPluginHost.xcodeproj in the JUCE subdir
        if ((NOT target STREQUAL "All") AND (NOT target STREQUAL "Standalone"))
            set_target_properties(${PROJECT_NAME}_${target} PROPERTIES
                XCODE_SCHEME_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app")
        endif ()
    endif ()
endforeach ()

if (TARGET Assets)
    set_target_properties(Assets PROPERTIES FOLDER "Targets")
endif ()

# # #

# This is where you can set preprocessor definitions for JUCE and your plugin
target_compile_definitions(SharedCode
    INTERFACE

    # JUCE_WEB_BROWSER and JUCE_USE_CURL off by default
    JUCE_WEB_BROWSER=0  # If you set this to 1, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
    JUCE_USE_CURL=0     # If you set this to 1, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
    JUCE_VST3_CAN_REPLACE_VST2=0

    # Uncomment if you are paying for a an Indie/Pro license or releasing under GPLv3
    # JUCE_DISPLAY_SPLASH_SCREEN=0

    # lets the app known if we're Debug or Release
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"

    # JucePlugin_Name is for some reason doesn't use the nicer PRODUCT_NAME
    PRODUCT_NAME_WITHOUT_VERSION="Distortion Designer"
)

# # #

# Link to any other modules you added (with juce_add_module) here!
# Usually JUCE modules must have PRIVATE visibility
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module
# However, with Pamplejuce, you'll link modules to SharedCode with INTERFACE visibility
# This allows the JUCE plugin targets and the Tests target to link against it
target_link_libraries(SharedCode
    INTERFACE
    Assets
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Link the JUCE plugin targets our SharedCode target
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

# # #

### IPP support, comment out to disable
# # When present, use Intel IPP for performance on Windows
# if (WIN32) # Can't use MSVC here, as it won't catch Clang on Windows
#     find_package(IPP)
#     if (IPP_FOUND)
#         target_link_libraries(SharedCode INTERFACE IPP::ipps IPP::ippcore IPP::ippi IPP::ippcv)
#         message("IPP LIBRARIES FOUND")
#         target_compile_definitions(SharedCode INTERFACE PAMPLEJUCE_IPP=1)
#     else ()
#         message("IPP LIBRARIES *NOT* FOUND")
#     endif ()
# endif ()

# # #

### Everything related to the tests target
# Required for ctest (which is just an easier way to run in cross-platform CI)
# include(CTest) could be used too, but adds additional targets we don't care about
# See: https://github.com/catchorg/Catch2/issues/2026
# You can also forgo ctest entirely and call ./Tests directly from the build dir
enable_testing()

# Go into detail when there's a CTest failure
set(CTEST_OUTPUT_ON_FAILURE ON)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

# "GLOBS ARE BAD" is brittle and silly dev UX, sorry CMake!
file(GLOB_RECURSE TestFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h")

# Organize the test source in the Tests/ folder in Xcode
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests PREFIX "" FILES ${TestFiles})

# Use Catch2 v3 on the devel branch
Include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
    GIT_TAG v3.4.0)
FetchContent_MakeAvailable(Catch2) # find_package equivalent

# Setup the test executable, again C++20 please
add_executable(Tests ${TestFiles})
target_compile_features(Tests PRIVATE cxx_std_20)

# Our test executable also wants to know about our plugin code...
target_include_directories(Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

# Copy over compile definitions from our plugin target so it has all the JUCEy goodness
target_compile_definitions(Tests PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)

# And give tests access to our shared code
target_link_libraries(Tests PRIVATE SharedCode Catch2::Catch2WithMain)

# Make an Xcode Scheme for the test executable so we can run tests in the IDE
set_target_properties(Tests PROPERTIES XCODE_GENERATE_SCHEME ON)

# When running Tests we have specific needs
target_compile_definitions(Tests PUBLIC
    JUCE_MODAL_LOOPS_PERMITTED=1 # let us run Message Manager in tests
    RUN_PAMPLEJUCE_TESTS=1 # also run tests in other module .cpp files guarded by RUN_PAMPLEJUCE_TESTS
)

# Load and use the .cmake file provided by Catch2
# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md
# We have to manually provide the source directory here for now
include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(Tests)

# # #

### A separate target keeps the Tests target fast!
file(GLOB_RECURSE BenchmarkFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks/*.h")

# Organize the test source in the Tests/ folder in the IDE
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/benchmarks PREFIX "" FILES ${BenchmarkFiles})

add_executable(Benchmarks ${BenchmarkFiles})
target_compile_features(Benchmarks PRIVATE cxx_std_20)
catch_discover_tests(Benchmarks)

# Our benchmark executable also wants to know about our plugin code...
target_include_directories(Benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

# Copy over compile definitions from our plugin target so it has all the JUCEy goodness
target_compile_definitions(Benchmarks PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_DEFINITIONS>)

# And give tests access to our shared code
target_link_libraries(Benchmarks PRIVATE SharedCode Catch2::Catch2WithMain)

# Make an Xcode Scheme for the test executable so we can run tests in the IDE
set_target_properties(Benchmarks PROPERTIES XCODE_GENERATE_SCHEME ON)

# When running Tests we have specific needs
target_compile_definitions(Benchmarks PUBLIC
    JUCE_MODAL_LOOPS_PERMITTED=1 # let us run Message Manager in tests
    RUN_PAMPLEJUCE_TESTS=1 # also run tests in module .cpp files guarded by RUN_PAMPLEJUCE_TESTS
)

# # #

### Pass some config to GA (like our PRODUCT_NAME)
# Write some temp files to make GitHub Actions / packaging easy

if ((DEFINED ENV{CI}))
    set (env_file "${PROJECT_SOURCE_DIR}/.env")
    message ("Writing ENV file for CI: ${env_file}")

    # the first call truncates, the rest append
    file(WRITE  "${env_file}" "PROJECT_NAME=${PROJECT_NAME}\n")
    file(APPEND "${env_file}" "PRODUCT_NAME=${PRODUCT_NAME}\n")
    file(APPEND "${env_file}" "VERSION=${CURRENT_VERSION}\n")
    file(APPEND "${env_file}" "BUNDLE_ID=${BUNDLE_ID}\n")
    file(APPEND "${env_file}" "COMPANY_NAME=${COMPANY_NAME}\n")
endif ()