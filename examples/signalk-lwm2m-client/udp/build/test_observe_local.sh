#!/bin/bash

# Simple Observation Test Script
# This demonstrates how to test observations locally before using 1NCE

echo "=== LwM2M Observation Test ==="
echo ""
echo "This will start the client and show you the available commands"
echo "including the new 'testobs' command for observation testing."
echo ""
echo "Available observation test commands:"
echo "  testobs    - Trigger manual notifications for /3/0/9 and /3/0/13"
echo "  change /3/0/9 - Manually trigger battery level change"
echo "  change /3/0/13 - Manually trigger time change"
echo ""
echo "To test with 1NCE server:"
echo "1. Start client with: ./signalk-lwm2m-client -b -h lwm2m.os.1nce.com -p 5683 -4 -k"
echo "2. Wait for STATE_READY"
echo "3. Send OBSERVE request from 1NCE to /3/0/9"
echo "4. Use 'testobs' to trigger immediate notifications"
echo ""
echo "Starting client in 3 seconds..."
sleep 3

cd /home/pi/wakaama/examples/signalk-lwm2m-client/udp/build
./signalk-lwm2m-client -h 127.0.0.1 -4
