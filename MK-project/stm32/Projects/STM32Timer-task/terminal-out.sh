#!/bin/bash

while true; do
    # Ищем порт каждый раз заново
    PORT=$(ls /dev/ttyACM* 2>/dev/null | head -1)
    
    if [ -n "$PORT" ]; then
        echo "Using port: $PORT"
        python -m serial.tools.miniterm $PORT 115200
        echo "Port closed. Reconnecting in 2 seconds..."
    else
        echo "No port found. Waiting for device..."
    fi
    
    sleep 2
done