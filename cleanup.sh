#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "=== cleaning Server ==="
make fclean -C server

echo "=== cleaning Client ==="
make fclean -C client

echo "=== cleaning Logs ==="
cd logs && rm -rf *

echo "=== CLEAN ==="
