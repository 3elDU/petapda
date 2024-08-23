cmake_minimum_required(VERSION 3.13..3.27)

# Write a compile_commands.json file, that Clangd LSP will pick up
# It provides include paths, library paths, etc.
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Load environment variables
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)
set(PICO_TOOLCHAIN_PATH $ENV{PICO_TOOLCHAIN_PATH})
set(CMAKE_C_COMPILER "${PICO_TOOLCHAIN_PATH}/arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "${PICO_TOOLCHAIN_PATH}/arm-none-eabi-g++")
set(CMAKE_C_COMPILER_TARGET "arm-none-eabi")
set(CMAKE_CXX_COMPILER_TARGET "arm-none-eabi")

# Import Raspbery Pi Pico SDK
set(PICO_SDK_INIT_CMAKE_FILE "${PICO_SDK_PATH}/pico_sdk_init.cmake")
message(STATUS "Pico SDK init CMake file: ${PICO_SDK_INIT_CMAKE_FILE}")
include(${PICO_SDK_INIT_CMAKE_FILE})

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
    src/sdcard.c
    src/debug.c
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

pico_add_extra_outputs(main)
