project(box2d VERSION 2.4.1)
add_compile_definitions(B2_USER_SETTINGS
                        TARGET_EXTENSION=1)

include_directories(box2d PUBLIC
                    .
                    ${CMAKE_SOURCE_DIR}/include
                    ${SDK}/C_API)

add_compile_options(-g
                    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
                    $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
                    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                    $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>)

add_subdirectory(box2d/src)
