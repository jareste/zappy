#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "=== cleaning Logs ==="
cd logs && rm -rf *

echo "=== CLEAN ==="
