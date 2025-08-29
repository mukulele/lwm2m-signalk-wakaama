# SignalK Automatic Reconnection System

## Overview

The SignalK automatic reconnection system provides robust connection management for marine IoT applications where network connectivity may be intermittent due to environmental factors such as weather, distance from shore, or temporary infrastructure issues.

## Features

- **Exponential Backoff**: Intelligent retry timing that prevents overwhelming the server
- **Configurable Retry Limits**: Set maximum attempts or infinite retries for different scenarios  
- **Jitter Prevention**: Random timing variance to prevent "thundering herd" problems
- **Connection Health Monitoring**: Track connection state and error history
- **Hot-Reload Support**: Configuration changes apply without restart
- **Marine Environment Optimized**: Default settings tuned for vessel connectivity patterns

## Configuration

Add a `reconnection` section to your `settings.json`:

```json
{
  "reconnection": {
    "auto_reconnect_enabled": true,
    "max_retries": 0,
    "base_delay_ms": 1000,
    "max_delay_ms": 300000,
    "backoff_multiplier": 2.0,
    "jitter_percent": 20,
    "connection_timeout_ms": 30000,
    "reset_on_success": true
  }
}
```

### Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `auto_reconnect_enabled` | `true` | Enable/disable automatic reconnection |
| `max_retries` | `0` | Maximum retry attempts (0 = infinite) |
| `base_delay_ms` | `1000` | Initial delay between retries (1 second) |
| `max_delay_ms` | `300000` | Maximum delay cap (5 minutes) |
| `backoff_multiplier` | `2.0` | Delay multiplication factor per attempt |
| `jitter_percent` | `20` | Random timing variance (0-100%) |
| `connection_timeout_ms` | `30000` | Individual connection attempt timeout |
| `reset_on_success` | `true` | Reset retry count on successful connection |

## Reconnection Behavior

### Exponential Backoff Timing

The system calculates retry delays using exponential backoff:

```
delay = base_delay × (multiplier ^ (attempt - 1))
```

With default settings:
- Attempt 1: 1 second
- Attempt 2: 2 seconds  
- Attempt 3: 4 seconds
- Attempt 4: 8 seconds
- Attempt 5: 16 seconds
- ...continuing until max_delay_ms (5 minutes)

### Jitter Calculation

Random jitter prevents multiple clients from reconnecting simultaneously:

```
final_delay = calculated_delay ± (calculated_delay × jitter_percent / 100)
```

With 20% jitter, a 4-second delay becomes 3.2-4.8 seconds randomly.

## Usage Examples

### Marine IoT (Default Configuration)

Optimized for vessels with potentially unstable connectivity:

```json
{
  "reconnection": {
    "auto_reconnect_enabled": true,
    "max_retries": 0,
    "base_delay_ms": 1000,
    "max_delay_ms": 300000,
    "backoff_multiplier": 2.0,
    "jitter_percent": 20,
    "connection_timeout_ms": 30000,
    "reset_on_success": true
  }
}
```

**Best for**: Ocean-going vessels, remote anchorages, cellular connectivity

### Harbor/Marina (Fast Reconnection)

Quick recovery for stable network environments:

```json
{
  "reconnection": {
    "auto_reconnect_enabled": true,
    "max_retries": 10,
    "base_delay_ms": 500,
    "max_delay_ms": 30000,
    "backoff_multiplier": 1.5,
    "jitter_percent": 10,
    "connection_timeout_ms": 15000,
    "reset_on_success": true
  }
}
```

**Best for**: Marina WiFi, shore-based installations, testing environments

### Satellite/Cellular (Conservative)

Longer delays for expensive or limited connectivity:

```json
{
  "reconnection": {
    "auto_reconnect_enabled": true,
    "max_retries": 0,
    "base_delay_ms": 5000,
    "max_delay_ms": 600000,
    "backoff_multiplier": 2.5,
    "jitter_percent": 30,
    "connection_timeout_ms": 60000,
    "reset_on_success": true
  }
}
```

**Best for**: Satellite internet, metered cellular data, remote locations

## API Reference

### Core Functions

```c
// Initialize reconnection system
bool signalk_reconnect_init(const signalk_reconnect_config_t *config);

// Load configuration from settings.json
bool signalk_reconnect_load_config(const char *config_file);

// Attempt connection with retry logic
signalk_connect_result_t signalk_reconnect_attempt(const char *server, int port);

// Handle connection events
void signalk_reconnect_on_disconnect(void);
void signalk_reconnect_on_connect(void);

// Check connection state
bool signalk_reconnect_should_retry(void);
const signalk_connection_state_t* signalk_reconnect_get_state(void);
```

### Utility Functions

```c
// Calculate delay for specific attempt number
int signalk_reconnect_calculate_delay(int attempt_number);

// Enable/disable auto-reconnect
bool signalk_reconnect_is_enabled(void);
void signalk_reconnect_set_enabled(bool enabled);

// Reset state and cleanup
void signalk_reconnect_reset(void);
void signalk_reconnect_cleanup(void);
```

## Integration Example

The reconnection system integrates seamlessly with the existing WebSocket client:

```c
#include "signalk_reconnect.h"

// In WebSocket callback function
static int callback_signalk(struct lws *wsi,
                            enum lws_callback_reasons reason,
                            void *user, void *in, size_t len)
{
    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        signalk_reconnect_on_connect();  // Notify success
        break;
        
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
    case LWS_CALLBACK_CLOSED:
        signalk_reconnect_on_disconnect();  // Trigger reconnection
        break;
    }
    return 0;
}

// In main loop
while (running) {
    lws_service(context, 100);
    
    // Check for reconnection attempts
    if (!wsi && signalk_reconnect_should_retry()) {
        // Attempt reconnection
        wsi = lws_client_connect_via_info(&ccinfo);
    }
}
```

## Testing

### Interactive Test Program

Run the reconnection test program:

```bash
# Build the test
make test-reconnect

# Interactive mode
./test-reconnect

# Automated scenarios
./test-reconnect burst      # 3 quick disconnects
./test-reconnect extended   # 5 disconnects with delays
./test-reconnect stress     # 10 rapid disconnects
```

### Test Output Example

```
SignalK Reconnection System Test
================================

=== Testing Exponential Backoff Calculation ===
Attempt  1:   1000ms ( 1.0s)
Attempt  2:   2000ms ( 2.0s)
Attempt  3:   4000ms ( 4.0s)
Attempt  4:   8000ms ( 8.0s)
Attempt  5:  16000ms (16.0s)

Connection State:
  Connected: NO
  Retry Count: 0
  Next Delay: 1000ms
  Last Error: Reset
  Auto-reconnect: ENABLED
```

## Monitoring and Diagnostics

### Connection State Structure

```c
typedef struct {
    bool is_connected;              // Current connection status
    int retry_count;                // Current number of retries
    time_t last_attempt;            // Timestamp of last connection attempt
    time_t last_success;            // Timestamp of last successful connection
    int next_delay_ms;              // Calculated delay for next retry
    char last_error[256];           // Description of last connection error
} signalk_connection_state_t;
```

### Error Codes

```c
typedef enum {
    SIGNALK_CONNECT_SUCCESS = 0,    // Connection successful
    SIGNALK_CONNECT_FAILED,         // Connection failed (will retry)
    SIGNALK_CONNECT_TIMEOUT,        // Connection timeout
    SIGNALK_CONNECT_MAX_RETRIES,    // Maximum retries exceeded
    SIGNALK_CONNECT_DISABLED        // Auto-reconnect disabled
} signalk_connect_result_t;
```

## Best Practices

### Marine Environment Considerations

1. **Infinite Retries**: Set `max_retries: 0` for ocean crossings
2. **Longer Timeouts**: Use 30-60 second connection timeouts for satellite links
3. **Higher Jitter**: Use 20-30% jitter to handle fleet reconnections
4. **Conservative Delays**: Start with 5+ second base delays for expensive connectivity

### Performance Optimization

1. **Monitor Retry Patterns**: Track connection state to identify network issues
2. **Adjust for Network Type**: Use different configs for WiFi vs cellular vs satellite
3. **Reset on Success**: Always enable `reset_on_success` to quickly recover from outages
4. **Connection Pooling**: Consider connection health before attempting expensive operations

### Troubleshooting

Common issues and solutions:

| Issue | Symptom | Solution |
|-------|---------|----------|
| Too aggressive | Rapid connection attempts | Increase `base_delay_ms` |
| Too slow | Long recovery times | Decrease `base_delay_ms` or `backoff_multiplier` |
| Server overload | Connection rejections | Increase `jitter_percent` |
| Permanent failures | Endless retries | Set appropriate `max_retries` |

## Implementation Notes

- Thread-safe design with global state protection
- Minimal memory footprint (~1KB)
- No external dependencies beyond cJSON for configuration
- Compatible with existing libwebsockets integration
- Supports hot-reload configuration changes

The reconnection system provides production-ready reliability for marine IoT deployments while remaining lightweight and configurable for different operational scenarios.
