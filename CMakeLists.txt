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

# Build box2d
add_compile_definitions(B2_USER_SETTINGS)
# add_subdirectory(extern ${PROJECT_BINARY_DIR})

project(${PLAYDATE_GAME_NAME}
        VERSION 1.0.0
        LANGUAGES CXX C ASM)

configure_file(config.h.in config.h)

file(GLOB IMAGES
    "Source/images/*"
)

include_directories(include
                    extern
                    extern/box2d/include)

add_compile_options(-g
                    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
                    $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
                    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                    $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>)

set(GAME_SOURCE_FILES
    src/pd.cpp
    src/main.cpp)

set(BOX2D_SRC extern/box2d/src)

set(BOX2D_SOURCE_FILES
	${BOX2D_SRC}/collision/b2_broad_phase.cpp
	${BOX2D_SRC}/collision/b2_chain_shape.cpp
	${BOX2D_SRC}/collision/b2_circle_shape.cpp
	${BOX2D_SRC}/collision/b2_collide_circle.cpp
	${BOX2D_SRC}/collision/b2_collide_edge.cpp
	${BOX2D_SRC}/collision/b2_collide_polygon.cpp
	${BOX2D_SRC}/collision/b2_collision.cpp
	${BOX2D_SRC}/collision/b2_distance.cpp
	${BOX2D_SRC}/collision/b2_dynamic_tree.cpp
	${BOX2D_SRC}/collision/b2_edge_shape.cpp
	${BOX2D_SRC}/collision/b2_polygon_shape.cpp
	${BOX2D_SRC}/collision/b2_time_of_impact.cpp
	${BOX2D_SRC}/common/b2_block_allocator.cpp
	${BOX2D_SRC}/common/b2_draw.cpp
	${BOX2D_SRC}/common/b2_math.cpp
	${BOX2D_SRC}/common/b2_settings.cpp
	${BOX2D_SRC}/common/b2_stack_allocator.cpp
	${BOX2D_SRC}/common/b2_timer.cpp
	${BOX2D_SRC}/dynamics/b2_body.cpp
	${BOX2D_SRC}/dynamics/b2_chain_circle_contact.cpp
	${BOX2D_SRC}/dynamics/b2_chain_circle_contact.h
	${BOX2D_SRC}/dynamics/b2_chain_polygon_contact.cpp
	${BOX2D_SRC}/dynamics/b2_chain_polygon_contact.h
	${BOX2D_SRC}/dynamics/b2_circle_contact.cpp
	${BOX2D_SRC}/dynamics/b2_circle_contact.h
	${BOX2D_SRC}/dynamics/b2_contact.cpp
	${BOX2D_SRC}/dynamics/b2_contact_manager.cpp
	${BOX2D_SRC}/dynamics/b2_contact_solver.cpp
	${BOX2D_SRC}/dynamics/b2_contact_solver.h
	${BOX2D_SRC}/dynamics/b2_distance_joint.cpp
	${BOX2D_SRC}/dynamics/b2_edge_circle_contact.cpp
	${BOX2D_SRC}/dynamics/b2_edge_circle_contact.h
	${BOX2D_SRC}/dynamics/b2_edge_polygon_contact.cpp
	${BOX2D_SRC}/dynamics/b2_edge_polygon_contact.h
	${BOX2D_SRC}/dynamics/b2_fixture.cpp
	${BOX2D_SRC}/dynamics/b2_friction_joint.cpp
	${BOX2D_SRC}/dynamics/b2_gear_joint.cpp
	${BOX2D_SRC}/dynamics/b2_island.cpp
	${BOX2D_SRC}/dynamics/b2_island.h
	${BOX2D_SRC}/dynamics/b2_joint.cpp
	${BOX2D_SRC}/dynamics/b2_motor_joint.cpp
	${BOX2D_SRC}/dynamics/b2_mouse_joint.cpp
	${BOX2D_SRC}/dynamics/b2_polygon_circle_contact.cpp
	${BOX2D_SRC}/dynamics/b2_polygon_circle_contact.h
	${BOX2D_SRC}/dynamics/b2_polygon_contact.cpp
	${BOX2D_SRC}/dynamics/b2_polygon_contact.h
	${BOX2D_SRC}/dynamics/b2_prismatic_joint.cpp
	${BOX2D_SRC}/dynamics/b2_pulley_joint.cpp
	${BOX2D_SRC}/dynamics/b2_revolute_joint.cpp
	${BOX2D_SRC}/dynamics/b2_weld_joint.cpp
	${BOX2D_SRC}/dynamics/b2_wheel_joint.cpp
	${BOX2D_SRC}/dynamics/b2_world.cpp
	${BOX2D_SRC}/dynamics/b2_world_callbacks.cpp
	${BOX2D_SRC}/rope/b2_rope.cpp)

if (TOOLCHAIN STREQUAL "armgcc")
    add_executable(${PLAYDATE_GAME_DEVICE}
                   ${SDK}/C_API/buildsupport/setup.c
                   ${BOX2D_SOURCE_FILES}
                   ${GAME_SOURCE_FILES})

    # NOTE: -g to support profiling on-device and appears to be used with API callbacks somehow.
    target_compile_options(${PLAYDATE_GAME_DEVICE} PUBLIC
                           -Wa,-amhls=${PLAYDATE_GAME_DEVICE}.s
                           $<$<COMPILE_LANGUAGE:ASM>:-amhls>)

    target_include_directories(${PLAYDATE_GAME_DEVICE} PUBLIC ${PROJECT_BINARY_DIR})

    # TODO: maybe this means stubs wouldn't have to be coded?
    target_link_options(${PLAYDATE_GAME_DEVICE} PUBLIC -nodefaultlibs -nostdlib)# --specs=nosys.specs)
    # target_link_libraries(${PLAYDATE_GAME_DEVICE} box2d)
    # add_dependencies(${PLAYDATE_GAME_DEVICE} box2d)
else()
    add_library(${PLAYDATE_GAME_NAME} SHARED
                ${BOX2D_SOURCE_FILES}
                ${GAME_SOURCE_FILES}
                ${IMAGES})

    target_include_directories(${PLAYDATE_GAME_NAME} PUBLIC ${PROJECT_BINARY_DIR})

    # target_link_libraries(${PLAYDATE_GAME_NAME} box2d)
    # add_dependencies(${PLAYDATE_GAME_NAME} box2d)
endif()

include(${SDK}/C_API/buildsupport/playdate_game.cmake)
