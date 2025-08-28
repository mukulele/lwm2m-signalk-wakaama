#!/bin/bash

# Test Server-Initiated Observations for SignalK-LwM2M Client
# This script sets up various observation scenarios

echo "=== SignalK-LwM2M Observation Testing ==="
echo

# Function to send observe request using coap-client
send_observe_request() {
    local host=$1
    local port=$2
    local path=$3
    local observe_flag=$4
    
    echo "📡 Sending observe request to: coap://$host:$port/$path"
    
    if [ "$observe_flag" == "observe" ]; then
        echo "   Starting observation (with observe option)..."
        coap-client -m get -o 0 -B 300 "coap://$host:$port/$path" &
        echo "   Observation started in background (PID: $!)"
    elif [ "$observe_flag" == "cancel" ]; then
        echo "   Canceling observation..."
        coap-client -m get "coap://$host:$port/$path"
    else
        echo "   Simple GET request..."
        coap-client -m get "coap://$host:$port/$path"
    fi
    echo
}

# Function to update SignalK data to trigger notifications
trigger_signalk_updates() {
    echo "🌊 Triggering SignalK data updates to test notifications..."
    
    # Send varying navigation data
    for i in {1..5}; do
        echo "Update $i/5..."
        
        # Update GPS satellites (should trigger notification if observed)
        curl -X PUT "http://localhost:3000/signalk/v1/api/vessels/self/navigation/gnss/satellites" \
             -H "Content-Type: application/json" \
             -d "{\"value\": $((7 + i))}" 2>/dev/null || echo "   (SignalK server not responding)"
        
        # Update CPU temperature
        temp=$(echo "scale=1; 320 + $i * 2.5" | bc -l)
        curl -X PUT "http://localhost:3000/signalk/v1/api/vessels/self/environment/rpi/cpu/temperature" \
             -H "Content-Type: application/json" \
             -d "{\"value\": $temp}" 2>/dev/null || echo "   (SignalK server not responding)"
        
        # Update speed
        speed=$(echo "scale=3; 0.1 + $i * 0.05" | bc -l)
        curl -X PUT "http://localhost:3000/signalk/v1/api/vessels/self/navigation/speedOverGround" \
             -H "Content-Type: application/json" \
             -d "{\"value\": $speed}" 2>/dev/null || echo "   (SignalK server not responding)"
        
        sleep 2
    done
    echo "✅ SignalK updates completed"
    echo
}

# Function to test various LwM2M resources
test_lwm2m_resources() {
    local client_ip=$1
    local client_port=$2
    
    echo "🔍 Testing various LwM2M resources for observation..."
    echo
    
    # Test Generic Sensor Object (3300) - our SignalK bridge
    echo "--- Generic Sensor Object (3300) ---"
    send_observe_request $client_ip $client_port "3300/0/5700" "observe"  # Sensor Value
    send_observe_request $client_ip $client_port "3300/0/5701" "get"      # Sensor Units
    send_observe_request $client_ip $client_port "3300/1/5700" "observe"  # Second sensor instance
    
    # Test Device Object (3) 
    echo "--- Device Object (3) ---"
    send_observe_request $client_ip $client_port "3/0/9" "observe"        # Battery Level
    send_observe_request $client_ip $client_port "3/0/13" "get"           # Current Time
    
    # Test Location Object (6)
    echo "--- Location Object (6) ---" 
    send_observe_request $client_ip $client_port "6/0/0" "observe"        # Latitude
    send_observe_request $client_ip $client_port "6/0/1" "observe"        # Longitude
    
    # Test Server Object (1)
    echo "--- Server Object (1) ---"
    send_observe_request $client_ip $client_port "1/0/1" "get"            # Lifetime
    
    echo "✅ Observation requests sent"
    echo
}

# Function to monitor the client output for observation activities
monitor_client_logs() {
    echo "📋 Monitoring client for observation activity..."
    echo "    Look for patterns like:"
    echo "    - 'COAP_GET with Observe option'"
    echo "    - 'Sending notification'"
    echo "    - 'lwm2m_resource_value_changed'"
    echo "    - 'Observe token:'"
    echo
}

# Main testing flow
main() {
    echo "Starting LwM2M Observation Testing"
    echo "=================================="
    echo
    
    # Default client settings (can be overridden)
    CLIENT_IP=${1:-"127.0.0.1"}
    CLIENT_PORT=${2:-"56830"}
    
    echo "🎯 Target Client: $CLIENT_IP:$CLIENT_PORT"
    echo
    
    # Check if coap-client is available
    if ! command -v coap-client &> /dev/null; then
        echo "❌ coap-client not found! Installing libcoap2-bin..."
        sudo apt-get update && sudo apt-get install -y libcoap2-bin
        echo
    fi
    
    # Check if bc is available for calculations
    if ! command -v bc &> /dev/null; then
        echo "📊 Installing bc for calculations..."
        sudo apt-get install -y bc
        echo
    fi
    
    echo "🚀 Starting observation tests..."
    echo
    
    # Step 1: Test basic resource access
    echo "=== Step 1: Basic Resource Discovery ==="
    send_observe_request $CLIENT_IP $CLIENT_PORT "" "get"
    
    # Step 2: Set up observations on key resources
    echo "=== Step 2: Setting up Observations ==="
    test_lwm2m_resources $CLIENT_IP $CLIENT_PORT
    
    # Step 3: Trigger data changes
    echo "=== Step 3: Triggering Data Changes ==="
    trigger_signalk_updates
    
    # Step 4: Cancel some observations
    echo "=== Step 4: Testing Observation Cancellation ==="
    send_observe_request $CLIENT_IP $CLIENT_PORT "3300/0/5700" "cancel"
    
    echo "=== Testing Complete ==="
    echo
    echo "💡 Tips for analyzing results:"
    echo "   1. Watch the client terminal for observation messages"
    echo "   2. Check for 'lwm2m_resource_value_changed' calls"
    echo "   3. Monitor CoAP notification packets"
    echo "   4. Verify SignalK data changes trigger LwM2M notifications"
    echo
    echo "🔧 Manual testing commands:"
    echo "   # Observe Generic Sensor Value:"
    echo "   coap-client -m get -o 0 'coap://$CLIENT_IP:$CLIENT_PORT/3300/0/5700'"
    echo
    echo "   # Cancel observation:"
    echo "   coap-client -m get 'coap://$CLIENT_IP:$CLIENT_PORT/3300/0/5700'"
    echo
    echo "   # Get battery level with observation:"
    echo "   coap-client -m get -o 0 'coap://$CLIENT_IP:$CLIENT_PORT/3/0/9'"
    echo
}

# Run the test
main "$@"
