#!/usr/bin/env bash

# Exit immediately if a command exits with a non-zero status
set -e

export ZAPPY_EASY_ASCENSION=0

# Cleanup function to kill the server when the script exits
cleanup() {
    echo "Cleaning up..."
    pkill -f "./zappy -p 8674" || true
}
trap cleanup EXIT

echo "=== Building Server ==="
make -C server

echo "=== Starting Server ==="
pkill -f zappy || true
sleep 1
# Run the server from its own directory so it finds its assets/certs
(cd server && ./zappy -p 8674 -x 10 -y 10 -n team1 -c 10 -f 10 > ../logs/server_log_normal_probe_six_clients.txt 2>&1 &)
sleep 1 # Wait a moment for the server to fully start and bind the port

echo "=== Running server/run.sh ==="
(cd server && ./run.sh)

echo "=== Building Client ==="
make -C client

echo "=== Running Clients ==="
for i in {1..5}; do
    ./client/client localhost 8674 team1 2> logs/client_log_normal_probe_six_clients_${i}.txt &
    sleep 0.5
done
./client/client localhost 8674 team1 2> logs/client_log_normal_probe_six_clients_6.txt
