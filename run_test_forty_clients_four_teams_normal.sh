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
(cd server && ./zappy -p 8674 -x 30 -y 30 -n team1 team2 team3 team4 -c 60 -f 10 > ../logs/server_log_normal_probe_four_teams.txt 2>&1 &)
sleep 1 # Wait a moment for the server to fully start and bind the port

echo "=== Running server/run.sh ==="
(cd server && ./run.sh)

echo "=== Building Client ==="
make -C client

echo "=== Running Clients ==="
for i in {1..10}; do
    ./client/client localhost 8674 team1 2> logs/client_log_normal_four_teams_team1_${i}.txt &
    sleep 0.5
done

for i in {1..10}; do
    ./client/client localhost 8674 team2 2> logs/client_log_normal_four_teams_team2_${i}.txt &
    sleep 0.5
done

for i in {1..10}; do
    ./client/client localhost 8674 team3 2> logs/client_log_normal_four_teams_team3_${i}.txt &
    sleep 0.5
done

for i in {1..10}; do
    ./client/client localhost 8674 team4 2> logs/client_log_normal_four_teams_team4_${i}.txt &
    sleep 0.5
done

echo "=== Clients launched ==="
wait
