#!/bin/bash

echo "Starting LwM2M client in background..."
./signalk-lwm2m-client -h 127.0.0.1 -4 &
CLIENT_PID=$!

echo "Client PID: $CLIENT_PID"
echo "Waiting 3 seconds..."
sleep 3

echo "Sending SIGINT to client..."
kill -INT $CLIENT_PID

echo "Waiting for client to finish..."
wait $CLIENT_PID
EXIT_CODE=$?

echo "Client exited with code: $EXIT_CODE"

if [ $EXIT_CODE -eq 0 ]; then
    echo "✓ Clean exit"
else
    echo "✗ Unclean exit (segfault or other error)"
fi
