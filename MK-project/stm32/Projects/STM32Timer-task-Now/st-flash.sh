#!/bin/bash
set -e
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$PROJECT_DIR"
PROJECT_NAME=$(basename "$PROJECT_DIR")
if [ -f "build/Debug/${PROJECT_NAME}.bin" ]; then
    FIRMWARE="build/Debug/${PROJECT_NAME}.bin"
elif ls build/Debug/*.bin 2>/dev/null | grep -q .; then
    FIRMWARE=$(ls build/Debug/*.bin | head -1)
else
    echo "❌ Файл прошивки (.bin) не найден в build/Debug/"
    echo "   Сначала соберите проект:"
    echo "   cmake --preset=Debug && cmake --build --preset=Debug"
    exit 1
fi
echo "Прошивка через ST-Link..."
echo "   Файл: $FIRMWARE"
echo "   Адрес: 0x08000000"
if ! st-info --probe 2>/dev/null | grep -q "STM32"; then
    echo "ST-Link не найден или чип не отвечает"
    echo "Проверьте подключение программатора и питание платы"
    exit 1
fi
st-flash --reset write "$FIRMWARE" 0x08000000
echo "✅ Прошивка завершена"
