#!/bin/bash
# ============================================================
#  Build script for STM32 CubeMX project (CMake toolchain)
#  Просто собирает проект. CMakeLists.txt не трогает.
# ============================================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

echo "=========================================="
echo "   Build STM32 project"
echo "=========================================="

# --- Проверки ---

if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: CMakeLists.txt not found in $PROJECT_DIR"
    exit 1
fi

if [ ! -f "cmake/gcc-arm-none-eabi.cmake" ]; then
    echo "ERROR: cmake/gcc-arm-none-eabi.cmake not found."
    echo "Regenerate project in CubeMX with Toolchain = CMake."
    exit 1
fi

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo "ERROR: arm-none-eabi-gcc not found in PATH."
    echo "Install ARM GNU Toolchain and add it to PATH."
    exit 1
fi

# --- Определяем пресет ---

# Если есть CMakePresets.json — используем пресеты (рекомендуется)
if [ -f "CMakePresets.json" ]; then
    echo "Using CMakePresets.json"
    PRESET="${1:-Debug}"

    # Проверяем, что такой пресет существует
    if ! cmake --list-presets 2>/dev/null | grep -q "\"$PRESET\""; then
        echo "WARNING: preset '$PRESET' not found, falling back to manual toolchain"
        USE_PRESET=0
    else
        USE_PRESET=1
    fi
else
    echo "CMakePresets.json not found, using manual toolchain"
    USE_PRESET=0
fi

# --- Конфигурация ---

BUILD_DIR="build"

if [ "$USE_PRESET" -eq 1 ]; then
    echo ""
    echo ">>> Configuring with preset: $PRESET"
    cmake --preset "$PRESET"
    BUILD_DIR="build/$PRESET"
else
    echo ""
    echo ">>> Configuring with toolchain file"
    rm -rf "$BUILD_DIR"
    cmake -B "$BUILD_DIR" \
        -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake \
        -DCMAKE_BUILD_TYPE=Debug
fi

# --- Сборка ---

echo ""
echo ">>> Building"
cmake --build "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 4)"

# --- Результат ---

echo ""
echo "=========================================="
echo "   SUCCESS"
echo "=========================================="

ELF_FILE=$(find "$BUILD_DIR" -maxdepth 2 -name "*.elf" | head -1)
if [ -n "$ELF_FILE" ]; then
    echo "ELF:  $(pwd)/$ELF_FILE"
    ls -lh "$ELF_FILE"

    BIN_FILE="${ELF_FILE%.elf}.bin"
    if [ -f "$BIN_FILE" ]; then
        echo "BIN:  $(pwd)/$BIN_FILE"
        ls -lh "$BIN_FILE"
    fi
else
    echo "WARNING: ELF file not found in $BUILD_DIR"
fi