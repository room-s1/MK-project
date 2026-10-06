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

