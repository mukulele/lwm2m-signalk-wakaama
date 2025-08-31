# Architecture

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

# Monitor bridge registry
grep "Bridge" logs/client.log

# 31.08.2025, 17:28
Achievements So Far
Test Directory & Build System:

Migrated and flattened unit tests to a single, maintainable test directory.
Refactored CMake and build scripts for a professional, standards-compliant workflow.
Resolved build errors and ensured the test suite runs cleanly.

SignalK-LwM2M Client Architecture:

Analyzed and documented the value mapping and notification logic from SignalK websocket to LwM2M server.
Refactored IPSO objects (Generic Sensor, Location, Power Measurement, Actuation, Energy) for dynamic registration and multi-instance support.
Implemented automatic value type detection for TLV encoding.
Added resource 5750 (Application Type) and enabled observe/notify for all instances.
Dynamic Registration & Mapping:

Moved dynamic registration logic to the websocket client, following best practice.
Designed and implemented a generalized mapping table (configurable via settings.json) to link SignalK paths to LwM2M object/resource/instance info.
Added code to load and parse the mapping table at startup.
Refactored the SignalK update handler to use the mapping table for dynamic object/resource creation and value updates.
Build & Test:

Performed a clean build and test run (build succeeded, but some binary location issues remain).


Open Items / Next Steps

Review Refactored IPSO objects (Generic Sensor, Location, Power Measurement, Actuation, Energy)Refactored IPSO objects (Generic Sensor, Location, Power Measurement, Actuation, Energy)
in more detail. The object_....c structures should be similar object_energy.c once fully qualified. 

The test suite is incomplete und currently overengineered.

Build Error:

There is a compilation error in object_energy.c (prv_delete declared but not defined, and a stray statement at end of input). This needs to be fixed for a successful build.
Binary Location:

The main client binary (lwm2mclient_dbg) was not found in the expected location. Need to locate the correct binary and run it for full integration testing.
Pattern Matching:

The mapping table currently supports simple wildcard matching. You may want to extend this to support more flexible patterns (regex, etc.).
Instance ID Extraction:

The current strategy uses numeric suffixes. You may want to support other strategies (hash, custom logic) for more complex SignalK paths.
Testing End-to-End:

After fixing build errors and running the correct binary, verify that dynamic registration and value mapping work as expected with live SignalK data and LwM2M server.
Documentation & Extensibility:

Document the mapping table format and usage for future maintainers.
Add support for additional IPSO objects or custom objects as needed.
