# SignalK Authentication Implementation Summary

## Overview
The SignalK-LwM2M client now supports **optional** token-based authentication following SignalK v1.7.0 specification. Authentication is configuration-driven and gracefully degrades when not required.

## Key Features

### 1. **Optional Authentication**
- **Default**: Authentication disabled (`"enabled": false`)
- **Compatible**: Works with SignalK servers with or without security
- **Graceful Degradation**: Falls back to anonymous access when auth fails

### 2. **WebSocket Authentication Flow**
```
1. Client connects to SignalK WebSocket
2. If auth enabled: Send login request with username/password
3. Server responds with JWT token + expiration
4. Token used for subsequent PUT requests
5. Automatic token renewal before expiration
```

### 3. **PUT Request Authentication**
- **HTTP Bearer Token**: Added to Authorization header when available
- **401/403 Handling**: Clear error messages when authentication required
- **Fallback**: Continues without auth for read-only operations

## Configuration

### Default Settings (No Authentication)
```json
{
  "signalk_subscriptions": {
    "server": {
      "host": "127.0.0.1",
      "port": 3000,
      "authentication": {
        "enabled": false,
        "username": "",
        "password": "",
        "token_renewal_time": 3600
      }
    }
  }
}
```

### Authenticated Settings (Production Vessels)
```json
{
  "signalk_subscriptions": {
    "server": {
      "host": "vessel.signalk.org",
      "port": 443,
      "authentication": {
        "enabled": true,
        "username": "vessel_client",
        "password": "secure_password",
        "token_renewal_time": 3600
      }
    }
  }
}
```

## Implementation Details

### Authentication Module (`signalk_auth.h/c`)
- **JWT Token Management**: Parse, store, validate tokens
- **WebSocket Login**: Protocol-compliant login/logout messages
- **Token Renewal**: Automatic renewal before expiration
- **State Management**: Track authentication state

### WebSocket Client Integration (`signalk_ws.c`)
- **Connection Flow**: Authenticate before subscriptions
- **Graceful Fallback**: Disable auth if server doesn't require it
- **Error Handling**: Retry without auth on failure

### PUT Request Integration (`signalk_control.c`)
- **Bearer Token**: Add Authorization header when authenticated
- **Error Responses**: Handle 401/403 with helpful messages
- **Compatibility**: Works with both authenticated and anonymous servers

## Usage Scenarios

### 1. **Development/Testing** (Default)
- No authentication required
- Works with demo.signalk.org
- Immediate connectivity

### 2. **Production Vessels**
- Enable authentication in settings.json
- Secure PUT operations for vessel control
- Token-based access control

### 3. **Mixed Environments**
- Same client binary works in both modes
- Configuration-driven behavior
- No code changes required

## Security Benefits

### When Authentication Enabled:
- **Secure PUT Requests**: Only authenticated clients can control vessel systems
- **Token Expiration**: Limited-time access tokens
- **Audit Trail**: Server can log authenticated operations
- **Access Control**: Per-user/device permissions

### When Authentication Disabled:
- **Read-Only Safety**: No vessel control without explicit configuration
- **Development Friendly**: Easy testing and development
- **Backward Compatible**: Works with existing SignalK setups

## Error Handling

### Authentication Failures:
```
[SignalK] Authentication failed - disabling authentication and retrying
[SignalK] This server may not require authentication for read operations
```

### PUT Request Authorization:
```
[SignalK Control] ✗ PUT failed - Authentication required (HTTP 401)
[SignalK Control] This server requires authentication for PUT requests
[SignalK Control] Enable authentication in settings.json to use vessel control
```

## Technical Implementation

### Files Modified:
- `signalk_auth.h/c` - Authentication module (NEW)
- `signalk_ws.c` - WebSocket client with auth integration
- `signalk_control.c` - PUT requests with Bearer tokens
- `signalk_subscriptions.h/c` - Configuration parsing
- `settings.json` - Default configuration (auth disabled)
- `CMakeLists.txt` - Build system updates

### Dependencies:
- **cJSON**: JSON parsing for auth responses
- **WebSocket**: Login/logout message handling
- **libcurl**: Bearer token headers for PUT requests

## Future Enhancements

1. **Certificate-based Authentication**: PKI support for enterprise
2. **Access Control Lists**: Fine-grained permissions per path
3. **Token Persistence**: Save tokens across client restarts
4. **Multi-Server Support**: Different auth per SignalK server

## Compliance

- **SignalK v1.7.0**: Full specification compliance
- **JWT Tokens**: Standard token format
- **HTTP Bearer**: RFC 6750 Authorization header
- **WebSocket Protocol**: SignalK-compliant auth messages

This implementation provides a robust, flexible authentication system that works in both development and production environments while maintaining backward compatibility with existing SignalK deployments.
