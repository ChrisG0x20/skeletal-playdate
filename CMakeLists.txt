cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(ENVSDK $ENV{PLAYDATE_SDK_PATH})

if (NOT ${ENVSDK} STREQUAL "")
    # Convert path from Windows
    file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
    execute_process(
        COMMAND bash -c "egrep '^\\s*SDKRoot' $HOME/.Playdate/config"
        COMMAND head -n 1
        COMMAND cut -c9-
        OUTPUT_VARIABLE SDK
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

if (NOT EXISTS ${SDK})
    message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
    return()
endif()

# See if the build configuration is targeting the device
if (DEFINED CACHE{CMAKE_TOOLCHAIN_FILE})
    string(FIND ${CMAKE_TOOLCHAIN_FILE} "arm.cmake" IS_DEVICE_BUILD)
    if (-1 EQUAL ${IS_DEVICE_BUILD})
        set(IS_DEVICE_BUILD FALSE)
    else()
        set(IS_DEVICE_BUILD TRUE)
    endif()
else()
    set(IS_DEVICE_BUILD FALSE)
endif()

# Verify that the ARM compiler path is set
if ((${IS_DEVICE_BUILD}) AND (NOT EXISTS ${ARM_TOOLCHAIN_PATH}))
    message(FATAL_ERROR "ARM GCC Toolchain path not found; set the path with switch -DARM_TOOLCHAIN_PATH=")
    return()
endif()

# Make the Playdate SDK version available to the app
file(READ ${SDK}/VERSION.txt SDK_VERSION)
string(REPLACE "." ";" SDK_VERSION_LIST ${SDK_VERSION})
list(GET SDK_VERSION_LIST 0 SDK_VERSION_MAJOR)
list(GET SDK_VERSION_LIST 1 SDK_VERSION_MINOR)
list(GET SDK_VERSION_LIST 2 SDK_VERSION_PATCH)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Game Name Customization
set(PLAYDATE_GAME_NAME Skeleton)
set(PLAYDATE_GAME_DEVICE Skeleton_DEVICE)

project(${PLAYDATE_GAME_NAME}
        VERSION 1.0.0
        LANGUAGES CXX C ASM)

configure_file(config.h.in config.h)

file(GLOB IMAGES
    "Source/images/*"
)

include_directories("include" "extern")

set(SOURCE_FILES
    src/main.cpp)

if (TOOLCHAIN STREQUAL "armgcc")
    add_executable(${PLAYDATE_GAME_DEVICE}
                   ${SDK}/C_API/buildsupport/setup.c
                   ${SOURCE_FILES})

    # NOTE: -g to support profiling on-device and appears to be used with API callbacks somehow.
    target_compile_options(${PLAYDATE_GAME_DEVICE} PUBLIC
                           -g -Wa,-amhls=${PLAYDATE_GAME_DEVICE}.s
                           $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti> $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
                           $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>
                           $<$<COMPILE_LANGUAGE:ASM>:-amhls>)

    # TODO: maybe this means stubs wouldn't have to be coded?
    # target_link_options(${PLAYDATE_GAME_DEVICE} PUBLIC --specs=nosys.specs)

    target_include_directories(${PLAYDATE_GAME_DEVICE} PUBLIC
                               ${PROJECT_BINARY_DIR})
else()
    add_library(${PLAYDATE_GAME_NAME}
                SHARED ${SOURCE_FILES}
                ${IMAGES})

    target_compile_options(${PLAYDATE_GAME_NAME} PUBLIC
                           -g
                           $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti> $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
                           $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>)

    target_include_directories(${PLAYDATE_GAME_NAME} PUBLIC
                               ${PROJECT_BINARY_DIR})
endif()

include(${SDK}/C_API/buildsupport/playdate_game.cmake)
