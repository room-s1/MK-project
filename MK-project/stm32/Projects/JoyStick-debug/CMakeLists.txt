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

# ============================================
# АВТОМАТИЧЕСКИ ПОДХВАТЫВАЕМ ВСЕ ПАПКИ
# ============================================

# Все .c файлы из Core/Src
file(GLOB_RECURSE ALL_SRC ${CMAKE_SOURCE_DIR}/Core/Src/*.c)

# Убираем дублирующиеся системные файлы
list(REMOVE_ITEM ALL_SRC
    ${CMAKE_SOURCE_DIR}/Core/Src/system_stm32f4xx.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32f4xx_hal_msp.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32f4xx_it.c
)

# Добавляем все найденные файлы
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${ALL_SRC})

# Добавляем папку с заголовками
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/Core/Inc
)

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE)
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME}
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    m
    stm32cubemx
)

# Linker script
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
# ============================================
# AUTOMATIC SOURCE FILE DETECTION
# ============================================

file(GLOB_RECURSE USER_SOURCES ${CMAKE_SOURCE_DIR}/Core/Src/*.c)

list(REMOVE_ITEM USER_SOURCES
    ${CMAKE_SOURCE_DIR}/Core/Src/system_stm32f4xx.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32f4xx_hal_msp.c
    ${CMAKE_SOURCE_DIR}/Core/Src/stm32f4xx_it.c
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${USER_SOURCES})

