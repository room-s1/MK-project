#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

echo "=========================================="
echo "   Fix CMakeLists.txt for STM32 project"
echo "=========================================="

if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: CMakeLists.txt not found in current directory"
    exit 1
fi

BACKUP="CMakeLists.txt.backup.$(date +%Y%m%d_%H%M%S)"
cp CMakeLists.txt "$BACKUP"
echo "Backup created: $BACKUP"

if ! grep -q "arm-none-eabi-gcc" CMakeLists.txt; then
    echo "ERROR: ARM toolchain not found in CMakeLists.txt"
    echo "Please generate project with CMake toolchain in CubeMX first"
    exit 1
fi

echo "ARM toolchain detected"

if grep -q "Core/Inc" CMakeLists.txt; then
    echo "Core/Inc already in include paths"
else
    echo "Adding Core/Inc to include paths"
    sed -i '/target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE)/a\
    ${CMAKE_SOURCE_DIR}/Core/Inc' CMakeLists.txt
fi

if grep -q "file(GLOB_RECURSE USER_SOURCES" CMakeLists.txt; then
    echo "Automatic source detection already present"
else
    echo "Adding automatic source file detection"
    
    cat >> CMakeLists.txt << 'EOF'

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

EOF
fi

if grep -q "USE_HAL_DRIVER" CMakeLists.txt; then
    echo "HAL definitions already present"
else
    echo "Adding HAL definitions"
    sed -i '/target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE)/a\
    USE_HAL_DRIVER\
    STM32F411xE' CMakeLists.txt
fi

if grep -q "target_link_libraries.*m" CMakeLists.txt; then
    echo "Math library already linked"
else
    echo "Adding math library"
    sed -i '/target_link_libraries(${CMAKE_PROJECT_NAME}/a\
    m' CMakeLists.txt
fi

echo ""
echo "Building project..."
echo ""

rm -rf build
mkdir build
cd build

cmake ..

if [ $? -ne 0 ]; then
    echo ""
    echo "CMake configuration failed"
    exit 1
fi

cmake --build . -j$(nproc)

if [ $? -ne 0 ]; then
    echo ""
    echo "Build failed"
    exit 1
fi

echo ""
echo "=========================================="
echo "   SUCCESS"
echo "=========================================="

ELF_FILE=$(find . -maxdepth 1 -name "*.elf" | head -1)
if [ -n "$ELF_FILE" ]; then
    echo "ELF file: $(pwd)/$ELF_FILE"
    ls -lh "$ELF_FILE"
fi

echo ""
echo "Backup saved to: $BACKUP"