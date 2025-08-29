# SignalK JSON Configuration System

## Overview

The SignalK-LwM2M client now supports dynamic configuration through JSON files, providing a flexible and maintainable way to manage SignalK subscriptions and server settings.

## Configuration Files

### `settings.json`
The main configuration file containing:
- **Server Configuration**: SignalK server connection details
- **Subscription Configuration**: Array of SignalK paths with timing and precision settings

## File Structure

```
/websocket_client/
├── settings.json                 # 📋 Main configuration file
├── signalk_subscriptions.c       # 🔧 Configuration management logic
├── signalk_subscriptions.h       # 📝 Configuration interface
└── signalk_ws.c                  # 🌐 WebSocket client (uses config)
```

## Configuration Schema

### Server Configuration
```json
{
  "signalk_subscriptions": {
    "server": {
      "host": "127.0.0.1",           // SignalK server hostname/IP
      "port": 3000,                  // SignalK server port
      "path": "/signalk/v1/stream",  // WebSocket connection path
      "subscribe_mode": "none"       // Initial subscription mode
    }
  }
}
```

### Subscription Configuration
```json
{
  "subscriptions": [
    {
      "path": "navigation.position",     // SignalK data path
      "period_ms": 1000,               // Update interval in milliseconds
      "min_period_ms": 500,            // Minimum update interval
      "high_precision": true,          // Enable high-precision data handling
      "description": "Vessel position" // Human-readable description
    }
  ]
}
```

## Update Frequency Categories

### 🚀 High-Frequency (≤1s)
- **navigation.position** - Critical for real-time position tracking
- **navigation.speedOverGround** - Essential for navigation
- **navigation.courseOverGroundTrue** - Course information
- **navigation.datetime** - Timestamp synchronization

### ⚡ Medium-Frequency (2-5s)
- **navigation.gnss.methodQuality** - GNSS fix quality (2s)
- **navigation.gnss.satellites** - Satellite count (3s)
- **navigation.gnss.antennaAltitude** - Altitude information (3s)
- **navigation.gnss.horizontalDilution** - HDOP values (5s)
- **navigation.gnss.differentialAge** - Differential corrections (5s)
- **navigation.gnss.differentialReference** - Reference station (5s)

### 🔄 Low-Frequency (>5s)
- **environment.rpi.cpu.temperature** - System monitoring (30s)
- **environment.rpi.memory.utilisation** - Memory usage (30s)
- **environment.rpi.sd.utilisation** - Storage usage (30s)
- **environment.rpi.cpu.utilisation** - CPU usage (30s)
- **environment.rpi.cpu.core** - Per-core CPU usage (30s)
- **network.interfaces** - Network interface info (60s)

## API Functions

### Configuration Management
```c
// Load configuration from JSON file
bool signalk_load_config_from_file(const char* filename);

// Save current configuration to JSON file
bool signalk_save_config_to_file(const char* filename);

// Free allocated configuration memory
void signalk_free_config(void);
```

### Subscription Management
```c
// Create SignalK subscription message from loaded config
bool signalk_create_subscription_message(char** json_string);

// Process subscription response from SignalK server
bool signalk_process_subscription_response(const char* message);

// Log current subscription status and statistics
void signalk_log_subscription_status(void);
```

## Configuration Access

### Global Variables
```c
extern signalk_server_config_t* signalk_server_config;     // Server settings
extern signalk_subscription_config_t* signalk_subscriptions; // Subscription array
extern int signalk_subscription_count;                     // Number of subscriptions
```

## Usage Examples

### Loading Configuration
```c
// Load from default settings.json
if (signalk_load_config_from_file(NULL)) {
    printf("Configuration loaded successfully\n");
    signalk_log_subscription_status();
}

// Load from custom file
if (signalk_load_config_from_file("custom_config.json")) {
    printf("Custom configuration loaded\n");
}
```

### Creating Subscription Message
```c
char* subscription_json = NULL;
if (signalk_create_subscription_message(&subscription_json)) {
    // Send subscription_json to SignalK server via WebSocket
    lws_write(wsi, subscription_json, strlen(subscription_json), LWS_WRITE_TEXT);
    free(subscription_json);
}
```

### Modifying Configuration
1. Edit `settings.json` directly
2. Restart the client to load new configuration
3. Or programmatically modify and save:

```c
// Modify a subscription
signalk_subscriptions[0].period_ms = 500;  // Increase frequency

// Save changes
signalk_save_config_to_file("modified_settings.json");
```

## Benefits

### ✅ **Flexibility**
- No code recompilation needed for configuration changes
- Easy to create different configurations for different environments
- Runtime configuration validation and error reporting

### ✅ **Maintainability**
- Human-readable JSON format with descriptions
- Centralized configuration management
- Clear separation of configuration and logic

### ✅ **Scalability**
- Easy to add new subscription paths
- Configurable update frequencies for bandwidth optimization
- Support for high-precision data marking

### ✅ **Production Ready**
- Robust error handling and fallback mechanisms
- Memory management with proper cleanup
- Configuration validation and status reporting

## File Locations

- **settings.json**: `/websocket_client/settings.json` (relative to build directory: `../websocket_client/settings.json`)
- **Configuration loaded at**: SignalK WebSocket client startup
- **Fallback behavior**: Uses command-line parameters if JSON loading fails

## Integration

The JSON configuration system is fully integrated with the existing SignalK-LwM2M bridge:
1. **Startup**: Configuration loaded from `settings.json`
2. **Connection**: Uses server settings from JSON
3. **Subscription**: Dynamically creates subscription message from JSON array
4. **Runtime**: Processes data according to configured precision settings
5. **Cleanup**: Properly frees configuration memory on shutdown
