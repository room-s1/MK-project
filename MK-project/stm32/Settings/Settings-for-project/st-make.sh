#!/bin/bash
set -e
cd "$(dirname "$0")"
cmake --preset=Debug
cmake --build --preset=Debug
