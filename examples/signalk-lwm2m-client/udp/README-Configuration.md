# SignalK-LwM2M Configuration Guide

## SignalK Path Availability

Not all SignalK servers will have every data path available. This depends on your vessel's instrumentation and the data sources connected to your SignalK server.

### Always Available Paths ✅
These paths are available on virtually all SignalK servers:
- `navigation.position.latitude` - GPS latitude
- `navigation.position.longitude` - GPS longitude

### Commonly Available Paths ⚠️
These paths are available if you have the corresponding instruments:
- `navigation.speedOverGround` - GPS speed
- `navigation.courseOverGround` - GPS course  
- `navigation.headingTrue` - Compass heading
- `navigation.datetime` - GPS time

### Optional Electrical Paths 🔋
These paths require electrical monitoring equipment:
- `electrical.batteries.house.voltage` - House battery voltage
- `electrical.batteries.engine.voltage` - Engine battery voltage
- `electrical.batteries.house.current` - House battery current
- `electrical.solar.power` - Solar panel power output
- `electrical.loads.*.power` - Individual load power consumption

## How to Enable Optional Paths

1. **Check your SignalK server** - Connect to your SignalK server web interface (usually http://your-boat-ip:3000)
2. **Browse available data** - Look at the "Data Browser" or "Server Status" to see what paths are actually available
3. **Update settings.json** - Move paths from `_optional_subscriptions` to `subscriptions` if they exist on your server

### Example: Adding Battery Monitoring

If your server has `electrical.batteries.house.voltage`, move this block:

```json
{
  "path": "electrical.batteries.house.voltage",
  "lwm2m_object": 3305,
  "lwm2m_instance": 0,
  "lwm2m_resource": 5700,
  "period": 5000,
  "minPeriod": 2000,
  "maxAge": 30000
}
```

From the `_optional_subscriptions` section to the main `subscriptions` array.

## Testing Path Availability

You can test if a path is available by running:

```bash
# Test with curl
curl -s "http://localhost:3000/signalk/v1/api/vessels/self/navigation/position" | jq

# Test with websocat (if installed)
echo '{"context":"vessels.self","subscribe":[{"path":"electrical.batteries.house.voltage","period":1000}]}' | websocat ws://localhost:3000/signalk/v1/stream
```

## Common SignalK Setups

### Basic GPS-only Setup
- Position data ✅
- Speed and course ✅
- Limited electrical data ❌

### Full Marine Electronics
- Navigation data ✅
- Wind instruments ✅
- Depth sounder ✅
- Basic electrical (battery voltage) ✅

### Advanced Electrical Monitoring
- NMEA 2000 battery monitors ✅
- Solar charge controllers ✅ 
- Individual load monitoring ✅
- Shore power monitoring ✅

## Error Handling

The client will gracefully handle missing paths:
- ⚠️ Warning logged for unavailable paths
- ✅ Continues operation with available paths
- 🔄 Automatic retry on reconnection

## Getting Help

1. Check your SignalK server logs for error messages
2. Use the SignalK server's built-in data browser
3. Test individual paths before adding them to settings.json
4. Start with minimal configuration and add paths incrementally
