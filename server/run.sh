#!/usr/bin/env bash

pids=$(pgrep zappy || true)
if [ -z "$pids" ]; then
  echo "No 'zappy' process found."
  exit 1
fi

for pid in $pids; do
  echo "Sending SIGUSR1 to PID $pid"
  if kill -USR1 "$pid"; then
    echo "Signaled $pid"
  else
    echo "Failed to signal $pid" >&2
  fi
done