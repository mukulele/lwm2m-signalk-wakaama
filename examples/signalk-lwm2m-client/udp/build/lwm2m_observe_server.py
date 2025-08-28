#!/usr/bin/env python3
"""
Simple LwM2M Observation Test Server
Tests server-initiated observations with the SignalK-LwM2M client
"""

import socket
import time
import threading
import struct
from datetime import datetime

# CoAP Message Types
COAP_TYPE_CON = 0  # Confirmable
COAP_TYPE_NON = 1  # Non-confirmable  
COAP_TYPE_ACK = 2  # Acknowledgement
COAP_TYPE_RST = 3  # Reset

# CoAP Codes
COAP_GET = 1
COAP_POST = 2
COAP_PUT = 3
COAP_DELETE = 4
COAP_CREATED = 65      # 2.01
COAP_DELETED = 66      # 2.02
COAP_VALID = 67        # 2.03
COAP_CHANGED = 68      # 2.04
COAP_CONTENT = 69      # 2.05

# CoAP Options
COAP_OPTION_OBSERVE = 6
COAP_OPTION_URI_PATH = 11

class CoAPMessage:
    def __init__(self):
        self.version = 1
        self.type = COAP_TYPE_CON
        self.token_length = 0
        self.code = COAP_GET
        self.message_id = 0
        self.token = b''
        self.options = []
        self.payload = b''
    
    def pack(self):
        """Pack CoAP message into bytes"""
        # Header: Version(2) + Type(2) + Token Length(4) + Code(8) + Message ID(16)
        header = struct.pack('!BBH', 
                           (self.version << 6) | (self.type << 4) | self.token_length,
                           self.code,
                           self.message_id)
        
        # Token
        message = header + self.token
        
        # Options (simplified - just URI path and observe)
        for option in self.options:
            message += self._pack_option(option)
        
        # Payload marker and payload
        if self.payload:
            message += b'\xFF' + self.payload
            
        return message
    
    def _pack_option(self, option):
        """Pack a single option"""
        option_num, option_value = option
        
        if isinstance(option_value, str):
            option_value = option_value.encode('utf-8')
        elif isinstance(option_value, int):
            if option_value == 0:
                option_value = b''
            else:
                option_value = struct.pack('!I', option_value)
                # Remove leading zeros
                while option_value and option_value[0] == 0:
                    option_value = option_value[1:]
        
        value_len = len(option_value)
        
        # Option delta and length encoding (simplified)
        if option_num < 13 and value_len < 13:
            header = struct.pack('!B', (option_num << 4) | value_len)
        else:
            # Extended format handling would go here
            header = struct.pack('!B', (option_num << 4) | value_len)
        
        return header + option_value
    
    @classmethod
    def unpack(cls, data):
        """Unpack CoAP message from bytes"""
        if len(data) < 4:
            return None
            
        msg = cls()
        
        # Parse header
        first_byte, msg.code, msg.message_id = struct.unpack('!BBH', data[:4])
        msg.version = (first_byte >> 6) & 0x3
        msg.type = (first_byte >> 4) & 0x3
        msg.token_length = first_byte & 0xF
        
        offset = 4
        
        # Parse token
        if msg.token_length > 0:
            msg.token = data[offset:offset + msg.token_length]
            offset += msg.token_length
        
        # Parse options and payload (simplified)
        if offset < len(data):
            if data[offset:offset+1] == b'\xFF':
                msg.payload = data[offset+1:]
            else:
                # Parse options (simplified parsing)
                pass
        
        return msg

class LwM2MObserveServer:
    def __init__(self, host='0.0.0.0', port=5683):
        self.host = host
        self.port = port
        self.socket = None
        self.running = False
        self.observations = {}  # {client_addr: {resource_path: observe_token}}
        self.message_id = 1
        
    def start(self):
        """Start the observation server"""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind((self.host, self.port))
        self.running = True
        
        print(f"🏃 LwM2M Observation Server started on {self.host}:{self.port}")
        print("📡 Ready to send observation requests to clients...")
        print()
        
        # Start background notification sender
        threading.Thread(target=self._notification_loop, daemon=True).start()
        
        try:
            self._listen()
        except KeyboardInterrupt:
            print("\n🛑 Server stopping...")
        finally:
            self.stop()
    
    def stop(self):
        """Stop the server"""
        self.running = False
        if self.socket:
            self.socket.close()
    
    def _listen(self):
        """Listen for incoming CoAP messages"""
        while self.running:
            try:
                data, addr = self.socket.recvfrom(1024)
                threading.Thread(target=self._handle_message, args=(data, addr), daemon=True).start()
            except socket.error:
                if self.running:
                    print("Socket error occurred")
                break
    
    def _handle_message(self, data, addr):
        """Handle incoming CoAP message"""
        try:
            msg = CoAPMessage.unpack(data)
            if not msg:
                return
            
            print(f"📨 Received from {addr}: Code={msg.code}, MID={msg.message_id}, Token={msg.token.hex()}")
            
            # This is a notification from client - log it
            if msg.code == COAP_CONTENT:
                print(f"   🔔 NOTIFICATION received! Payload: {msg.payload}")
                print(f"   📊 Resource value updated: {msg.payload.decode('utf-8', errors='ignore')}")
                
                # Send ACK
                ack = CoAPMessage()
                ack.type = COAP_TYPE_ACK
                ack.code = 0  # Empty ACK
                ack.message_id = msg.message_id
                ack.token = msg.token
                
                self.socket.sendto(ack.pack(), addr)
                print(f"   ✅ ACK sent for notification")
            
        except Exception as e:
            print(f"Error handling message: {e}")
        
        print()
    
    def send_observe_request(self, client_addr, resource_path, observe=True):
        """Send observation request to a client"""
        try:
            msg = CoAPMessage()
            msg.type = COAP_TYPE_CON
            msg.code = COAP_GET
            msg.message_id = self.message_id
            self.message_id += 1
            
            # Generate token
            token = struct.pack('!H', msg.message_id)
            msg.token = token
            msg.token_length = len(token)
            
            # Add options
            if observe:
                msg.options.append((COAP_OPTION_OBSERVE, 0))  # Start observation
            
            # Add URI path option(s)
            path_parts = resource_path.strip('/').split('/')
            for part in path_parts:
                if part:
                    msg.options.append((COAP_OPTION_URI_PATH, part))
            
            data = msg.pack()
            self.socket.sendto(data, client_addr)
            
            if observe:
                # Store observation
                if client_addr not in self.observations:
                    self.observations[client_addr] = {}
                self.observations[client_addr][resource_path] = token
                
                print(f"📡 OBSERVE request sent to {client_addr} for /{resource_path}")
                print(f"   Token: {token.hex()}, MID: {msg.message_id}")
            else:
                print(f"📡 GET request sent to {client_addr} for /{resource_path}")
                
            return True
            
        except Exception as e:
            print(f"❌ Error sending observe request: {e}")
            return False
    
    def _notification_loop(self):
        """Background loop to demonstrate observation requests"""
        time.sleep(2)  # Wait for server to be ready
        
        # Default client address (will be updated when we detect client)
        client_addr = ('127.0.0.1', 56830)
        
        print("🎯 Starting observation test sequence...")
        print()
        
        # Test sequence
        test_resources = [
            ('3300/0/5700', 'Generic Sensor Value'),  # SignalK bridge sensor
            ('3/0/9', 'Battery Level'),               # Device battery
            ('6/0/0', 'Latitude'),                    # Location latitude  
            ('6/0/1', 'Longitude'),                   # Location longitude
            ('3300/1/5700', 'Second Sensor Value'),   # Another sensor instance
        ]
        
        for i, (resource, description) in enumerate(test_resources, 1):
            print(f"📍 Test {i}/{len(test_resources)}: Observing {description} ({resource})")
            success = self.send_observe_request(client_addr, resource, observe=True)
            
            if success:
                print(f"   ✅ Observation request sent for /{resource}")
            else:
                print(f"   ❌ Failed to send observation request")
            
            time.sleep(3)  # Wait between requests
            print()
        
        print("🏁 Initial observation requests completed!")
        print("   🔔 Waiting for notifications from client...")
        print("   📱 Try updating SignalK data or triggering resource changes")
        print()
        
        # Keep sending periodic requests
        while self.running:
            time.sleep(30)
            
            # Re-observe a changing resource
            print(f"🔄 [{datetime.now().strftime('%H:%M:%S')}] Re-observing sensor value...")
            self.send_observe_request(client_addr, '3300/0/5700', observe=True)
            print()

def main():
    print("=== LwM2M Observation Test Server ===")
    print("🎯 This server will send observation requests to your SignalK-LwM2M client")
    print("🔔 It will display any notifications received from the client")
    print()
    
    server = LwM2MObserveServer()
    
    print("💡 Usage:")
    print("   1. Start your SignalK-LwM2M client")
    print("   2. This server will automatically send observation requests")
    print("   3. Update SignalK data to trigger notifications")
    print("   4. Watch for notification messages here")
    print()
    print("🚀 Starting server...")
    print()
    
    try:
        server.start()
    except KeyboardInterrupt:
        print("👋 Goodbye!")

if __name__ == '__main__':
    main()
