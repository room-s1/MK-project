Что куда вписывать в CMakeLists.txt:
1. .c / .cpp файлы — добавляются в секцию target_sources. Например: Core/Src/pid.c. Пути указываются относительно корня проекта (там, где лежит сам CMakeLists.txt).
2. Папки с .h файлами — добавляются в секцию target_include_directories. Например: Core/Inc/modules. При этом папку Core/Inc дублировать не нужно — она уже добавлена автоматически через cmake/stm32cubemx/CMakeLists.txt.
3. Макросы препроцессора (-DXXX) — добавляются в секцию target_compile_definitions. Например: USE_FREERTOS=1 или DEBUG_UART_BAUDRATE=115200.
4. .a библиотеки — добавляются через target_link_directories (путь к папке с библиотекой) и target_link_libraries (имя библиотеки). В обычных проектах это нужно редко.

Важные правила:
    Не редактировать cmake/stm32cubemx/CMakeLists.txt — его перезапишет CubeMX.
    .h файлы в target_sources не пишутся — только .c/.cpp.
    После правки CMakeLists.txt нужно заново запустить конфигурацию: 
    cmake --preset=Debug, затем сборку cmake --build --preset=Debug
    При добавлении новых файлов переконфигурация обязательна — иначе CMake их не увидит.


Подготовка системы Arch, редактор Code. 

1. Установить.
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils arm-none-eabi-newlib arm-none-eabi-gdb cmake ninja openocd clang stlink

2. Настройка прав доступа к ST-Link (udev)
sudo nano /etc/udev/rules.d/70-st-link.rules

------------
# ST-LINK/V2
ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", TAG+="uaccess"
# ST-LINK/V2-1
ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", TAG+="uaccess"
------------

sudo udevadm control --reload-rules
sudo udevadm trigger

3. Переподключить ST-Link к USB.


Установка расширений в VS Code
clangd / Cortex-Debug / CMake Tools / clang-format

code --install-extension llvm-vs-code-extensions.vscode-clangd
code --install-extension ms-vscode.cmake-tools
code --install-extension marus25.cortex-debug
code --install-extension xaver.clang-format


Конфигурация проекта (создаёт build/Debug, генерирует compile_commands.json)
cmake --preset=Debug
Сборка проекта (создаёт .elf, .bin, .hex, .map)
cmake --build --preset=Debug
Полный ребилд с нуля
rm -rf build/Debug && cmake --preset=Debug && cmake --build --preset=Debug
Прошить .bin через st-flash
st-flash --reset write build/Debug/$(basename $(pwd)).bin 0x08000000
Прошить .hex через st-flash
st-flash --reset write build/Debug/$(basename $(pwd)).hex
Стереть всю Flash
st-flash erase
Проверить, видит ли ST-Link чип
st-info --probe
Прошить через OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/Debug/$(basename $(pwd)).elf verify reset exit"
Сборка в VS Code
Ctrl+Shift+B
Прошивка в VS Code
Ctrl+Shift+P → Tasks: Run Task → Flash with OpenOCD
Прошивка + отладка в VS Code
F5


Cделать скрипт испольняемым, один раз в проекте. Скрипты кидать лучше в корень проекта.
chmod +x name.sh
Запуск
./name.sh 


Скрипт для сборки 

#!/bin/bash
set -e
cd "$(dirname "$0")"
cmake --preset=Debug
cmake --build --preset=Debug


Скрипт для автоматической прошивки STM32

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


Если подключение по USB идет, для удобства, для общения чипа с ПК. 
Нужен пакет sudo pacman -S python-pyserial

#!/bin/bash
BAUD=115200
while true; do
    # Ищем порт: ACM (USB CDC) или USB (CH340/CP2102/FTDI)
    PORT=$(ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -1)
    if [ -n "$PORT" ]; then
        echo "Using port: $PORT @ $BAUD"
        python -m serial.tools.miniterm "$PORT" "$BAUD"
        echo "Port closed. Reconnecting in 2 seconds..."
    else
        echo "No port found. Waiting for device..."
    fi
    sleep 2
done

