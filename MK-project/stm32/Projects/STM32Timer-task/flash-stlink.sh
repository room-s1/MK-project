#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"

# Определяем имя проекта из папки
PROJECT_NAME=$(basename "$PROJECT_DIR")

# Ищем файл прошивки
if [ -f "build/${PROJECT_NAME}.bin" ]; then
    FIRMWARE="build/${PROJECT_NAME}.bin"
elif ls build/*.bin 2>/dev/null | grep -q .; then
    FIRMWARE=$(ls build/*.bin | head -1)
else
    echo "❌ Файл прошивки (.bin) не найден!"
    echo "   Сначала запустите ./fixCmake.sh"
    exit 1
fi

echo "Прошивка через ST-Link..."
echo "   Файл: $FIRMWARE"
echo "   Адрес: 0x08000000"

# Проверяем, видит ли программатор
if ! st-info --probe 2>/dev/null | grep -q "STM32"; then
    echo "ST-Link не найден!"
    echo "   Проверьте подключение программатора"
    exit 1
fi

# Прошивка
st-flash --reset write "$FIRMWARE" 0x08000000

echo " Прошивка завершена"