cmake_minimum_required(VERSION 3.13..3.27)

# Write a compile_commands.json file, that Clangd LSP will pick up
# It provides include paths, library paths, etc.
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Import Raspbery Pi Pico SDK
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)
include(${CMAKE_CURRENT_LIST_DIR}/pico-sdk/pico_sdk_init.cmake)

project(tinyos C CXX ASM)

pico_sdk_init()

# Include FreeRTOS
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS)
set(FREERTOS_KERNEL_RP2040_RELATIVE_PATH "portable/ThirdParty/GCC/RP2040")
add_subdirectory(${FREERTOS_KERNEL_PATH}/${FREERTOS_KERNEL_RP2040_RELATIVE_PATH} FREERTOS_KERNEL)

# Include lua
add_subdirectory(lua)

add_executable(main
    src/main.c
    src/sys/bus.c
    src/dev/sdcard.c
    src/dev/eink.c
    src/gfx/screen.c
    src/lua/screen.c
    src/os/fs.c
    src/debug.c
    src/fs/diskio.c
    src/fs/ff.c
    src/fs/ffsystem.c
    src/fs/ffunicode.c
    src/st7789/st7789.c
    src/7segment/7segment.c
)

target_include_directories(main PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_include_directories(main PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# pull in common dependencies
target_link_libraries(main
    pico_async_context_freertos
    FreeRTOS-Kernel-Heap4
    pico_stdlib
    hardware_spi
    lua_static
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_BUILD)
endif()

pico_add_extra_outputs(main)
