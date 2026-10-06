#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

echo "Исправление CMakeLists.txt для ARM компиляции..."

# Проверяем, нужно ли добавлять ARM настройки
if grep -q "set(CMAKE_C_COMPILER /usr/bin/arm-none-eabi-gcc)" CMakeLists.txt; then
    echo " ARM компилятор уже настроен, пропускаем..."
else
    echo " Добавляем ARM компилятор..."
    
# Создаём полностью новый CMakeLists.txt на основе шаблона
    cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.22)

# ============================================
# ARM компилятор для STM32
# ============================================
set(CMAKE_C_COMPILER /usr/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER /usr/bin/arm-none-eabi-gcc)

set(COMMON_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${COMMON_FLAGS} --specs=nosys.specs --specs=nano.specs" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} --specs=nosys.specs --specs=nano.specs" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "${COMMON_FLAGS}" CACHE STRING "" FORCE)

set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)
add_compile_options(-Wno-int-to-pointer-cast -Wno-pointer-to-int-cast)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
get_filename_component(CMAKE_PROJECT_NAME ${CMAKE_SOURCE_DIR} NAME)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)
add_executable(${CMAKE_PROJECT_NAME})
add_subdirectory(cmake/stm32cubemx)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE)
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)

#Linker script (с подавлением предупреждения RWX)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE 
    -T ${CMAKE_SOURCE_DIR}/STM32F411XX_FLASH.ld
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--no-warn-rwx-segments
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    OUTPUT_NAME ${CMAKE_PROJECT_NAME}.elf
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMENT "Generating BIN and HEX files"
)
EOF
fi

echo ""
echo "Сборка проекта..."
rm -rf build
cmake -B build
cmake --build build

echo ""
echo "Готово!"
echo ""
