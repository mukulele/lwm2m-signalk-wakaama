#!/bin/bash

# Simple SignalK data generator for testing
# This simulates a basic marine instrument sending data

echo "Generating test SignalK data..."

# Send some test navigation data to SignalK server
curl -X PUT \
  -H "Content-Type: application/json" \
  -d '{
    "updates": [
      {
        "timestamp": "'$(date -u +%Y-%m-%dT%H:%M:%S.%3NZ)'",
        "values": [
          {
            "path": "navigation.speedOverGround",
            "value": 5.2
          },
          {
            "path": "navigation.courseOverGround", 
            "value": 45.5
          },
          {
            "path": "environment.wind.speedApparent",
            "value": 8.3
          },
          {
            "path": "environment.wind.angleApparent",
            "value": 0.785
          }
        ]
      }
    ]
  }' \
  http://localhost:3000/signalk/v1/stream

echo "Test data sent to SignalK server"
