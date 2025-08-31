# SignalK-LwM2M Marine IoT Bridge

A production-ready bridge connecting SignalK marine data networks to OMA LwM2M IoT platforms, providing comprehensive vessel monitoring and control capabilities.

## 🚢 Features

### Complete Marine IoT Platform
- **Real-time Monitoring**: Position, navigation, electrical systems, and environmental data
- **Bidirectional Control**: Remote switch/actuator control via LwM2M write operations
- **OMA LwM2M Compliance**: Standard IoT objects for maximum platform compatibility
- **SignalK Integration**: Native WebSocket client with optimized subscriptions

### Implemented LwM2M Objects


### Bridge Registry System
- **128 Mapping Capacity**: Expandable SignalK path to LwM2M resource mappings
- **19+ Active Mappings**: Navigation, electrical, and control data streams
- **Thread-Safe Operations**: Concurrent SignalK and LwM2M access protection
- **Error Handling**: Graceful degradation with comprehensive logging

## 🏗️ Architecture

```
┌─────────────────┐    WebSocket    ┌──────────────────┐    CoAP/UDP    ┌─────────────────┐
│   SignalK       │◄──────────────►│  LwM2M Bridge    │◄──────────────►│   LwM2M Server │
│   Server        │                │                  │                │                 │
│                 │                │ ┌──────────────┐ │                │  (Leshan, 1NCE) │
│ • Navigation    │                │ │ Bridge       │ │                │                 │
│ • Electrical    │                │ │ Registry     │ │                │ • Device Mgmt   │
│ • Environmental │                │ │              │ │                │ • Remote Control│
│ • Control       │                │ │ 128 Mappings │ │                │ • Monitoring    │
└─────────────────┘                │ └──────────────┘ │                └─────────────────┘
                                   │                  │
                                   │ ┌──────────────┐ │
                                   │ │ LwM2M Objects│ │
                                   │ │              │ │
                                   │ │ 6, 3300,     │ │
                                   │ │ 3305, 3306,  │ │
                                   │ │ 3331         │ │
                                   │ └──────────────┘ │
                                   └──────────────────┘
```

## 🚀 Quick Start

### Prerequisites
- Raspberry Pi 3/4 or x86_64 Linux system
- SignalK server (v1.x) running on your vessel
- CMake and build tools

### Build & Install
```bash
git clone https://github.com/mukulele/lwm2m-signalk-wakaama.git
cd lwm2m-signalk-wakaama/examples/signalk-lwm2m-client/udp
mkdir build && cd build
cmake ..
make
```

### Configuration
Create `settings.json` with your SignalK server details:
```json
{
  "signalk": {
    "host": "localhost",
    "port": 3000,
    "subscriptions": [
      {
        "path": "navigation.position.latitude",
        "lwm2m_object": 6,
        "lwm2m_instance": 0,
        "lwm2m_resource": 0,
        "period": 2000
      }
    ]
  },
  "lwm2m": {
    "serverHost": "leshan.eclipseprojects.io",
    "serverPort": 5683,
    "endpointName": "MarineIoT-001",
    "lifetime": 300
  }
}
```

### Run
```bash
# Basic operation (standalone LwM2M objects)
./signalk-lwm2m-client -4 -h leshan.eclipseprojects.io

# With SignalK integration
./signalk-lwm2m-client -k -4

# Custom settings file
./signalk-lwm2m-client -k -4 -f /path/to/custom-settings.json
```

## 🔧 Configuration

### SignalK Data Mappings

| SignalK Path | LwM2M Object/Instance/Resource | Description |
|--------------|-------------------------------|-------------|
| `navigation.position.latitude` | 6/0/0 | GPS Latitude |
| `navigation.position.longitude` | 6/0/1 | GPS Longitude |
| `electrical.batteries.house.voltage` | 3305/0/5700 | House Battery Voltage |
| `electrical.switches.navigation.lights` | 3306/0/5850 | Navigation Lights Control |
| `electrical.solar.cumulativeEnergy` | 3331/0/5805 | Solar Energy Generated |

### Switch Control Instances

| Instance | Equipment | SignalK Path | Control Type |
|----------|-----------|--------------|--------------|
| 0 | Navigation Lights | `electrical.switches.navigation.lights` | On/Off |
| 1 | Anchor Light | `electrical.switches.anchor.light` | On/Off |
| 2 | Bilge Pump | `electrical.switches.bilgePump.main` | On/Off |
| 3 | Fresh Water Pump | `electrical.switches.freshWaterPump` | On/Off |
| 4 | Cabin Lights | `electrical.switches.cabin.lights` | On/Off + Dimmer |
| 5 | Windlass | `electrical.switches.windlass` | On/Off |

## 📡 LwM2M Server Compatibility

### Tested Platforms
- ✅ **Eclipse Leshan** - Public demo server
- ✅ **1NCE Platform** - Production cellular IoT
- ✅ **Generic LwM2M** - Any OMA-compliant server

### Remote Control
The system accepts LwM2M WRITE operations for equipment control:
```
WRITE /3306/0/5850 → true   # Turn on navigation lights
WRITE /3306/4/5851 → 75     # Set cabin lights to 75% brightness
WRITE /3306/2/5850 → false  # Turn off bilge pump
```

## 🔍 Monitoring


### Debugging
```bash
# Enable verbose logging
export LWM2M_LOG_LEVEL=DEBUG

# Monitor bridge registry
grep "Bridge" logs/client.log

# Watch LwM2M traffic
tcpdump -i any port 5683
```

## 🛠️ Development

### Adding New Objects
1. Create object files: `object_XXXX.c` and `object_XXXX.h`
2. Update `lwm2mclient.c` to include and initialize
3. Add to `CMakeLists.txt`
4. Register SignalK mappings with `bridge_register()`

### Extending Mappings
1. Update `settings.json` with new SignalK paths
2. Add bridge registrations in object initialization
3. Increase `MAX_BRIDGE_RESOURCES` if needed (currently 128)




## 📄 License

This project is licensed under the Eclipse Public License v2.0 - see the [LICENSE](LICENSE) file for details.

## 🌟 Acknowledgments

- Eclipse Wakaama project for LwM2M client implementation
- Signal K project for marine data standards
- OMA SpecWorks for LwM2M object definitions
- Marine IoT community for use case guidance

---
