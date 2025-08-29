# 🌍 Environment-Specific Configuration Inheritance

## Overview

Environment-specific configuration inheritance allows the SignalK-LwM2M bridge to adapt its behavior for different deployment environments (development, staging, production) while maintaining a single codebase and shared configuration patterns.

## 🎯 Problem Statement

### **Current Challenge:**
- Marine vessels deploy in different environments (development, testing, production)
- Each environment requires different settings (servers, update frequencies, debugging)
- Maintaining separate complete configuration files leads to:
  - **Configuration Drift** - Environments become inconsistent
  - **Maintenance Overhead** - Changes must be replicated across files
  - **Error Prone** - Easy to miss updates in specific environments
  - **Deployment Complexity** - Managing multiple complete configurations

### **Solution: Configuration Inheritance**
- **Base Configuration** - Common settings shared across all environments
- **Environment Overlays** - Specific overrides for each environment  
- **Automatic Merging** - Runtime inheritance and override resolution
- **Hot-Reload Compatible** - Works seamlessly with existing hot-reload system

## 🏗️ Architecture

### **File Structure**
```
/config/
├── settings.base.json              # 📋 Base configuration (shared)
├── settings.development.json       # 🔧 Development overrides
├── settings.staging.json           # 🧪 Staging/testing overrides
├── settings.production.json        # 🚀 Production overrides
├── settings.europe.json            # 🌍 Regional variations
└── vessels/
    ├── yacht-001.json              # 🛥️ Vessel-specific settings
    └── catamaran-002.json          # ⛵ Vessel-specific settings
```

### **Inheritance Chain**
```
Base Config → Environment Config → Vessel Config → Runtime Overrides
     ↓              ↓                    ↓              ↓
  Common       Dev/Prod/Stage      Vessel-Specific   CLI Args
  Settings      Overrides           Customization    Final Override
```

## 📋 Configuration Examples

### **Base Configuration** (`settings.base.json`)
```json
{
  "signalk_subscriptions": {
    "server": {
      "path": "/signalk/v1/stream",
      "subscribe_mode": "none",
      "reconnect_interval": 5000
    },
    "subscriptions": [
      {
        "path": "navigation.position",
        "period_ms": 2000,
        "min_period_ms": 1000,
        "description": "Vessel position tracking"
      },
      {
        "path": "electrical.batteries.house.voltage",
        "period_ms": 5000,
        "min_period_ms": 2000,
        "description": "Main house battery monitoring"
      },
      {
        "path": "electrical.solar.power",
        "period_ms": 10000,
        "min_period_ms": 5000,
        "description": "Solar panel power generation"
      }
    ]
  },
  "lwm2m": {
    "lifetime": 300,
    "endpointName": "MarineIoT",
    "binding": "U",
    "queue_mode": false
  },
  "system": {
    "log_level": "INFO",
    "metrics_enabled": true,
    "health_check_interval": 60000
  }
}
```

### **Development Environment** (`settings.development.json`)
```json
{
  "signalk_subscriptions": {
    "server": {
      "host": "localhost",
      "port": 3000,
      "reconnect_interval": 2000
    }
  },
  "lwm2m": {
    "serverHost": "localhost",
    "serverPort": 5683,
    "endpointName": "MarineIoT-DEV",
    "lifetime": 120
  },
  "system": {
    "log_level": "DEBUG",
    "hotreload_interval": 500,
    "verbose_output": true,
    "test_mode": true
  },
  "debugging": {
    "enable_simulation": true,
    "mock_gps_data": true,
    "log_all_messages": true
  }
}
```

### **Production Environment** (`settings.production.json`)
```json
{
  "signalk_subscriptions": {
    "server": {
      "host": "signalk.vessel.local",
      "port": 3000,
      "reconnect_interval": 10000
    },
    "subscriptions": [
      {
        "path": "navigation.position",
        "period_ms": 1000,
        "min_period_ms": 500
      },
      {
        "path": "electrical.batteries.house.voltage", 
        "period_ms": 3000,
        "min_period_ms": 1000
      }
    ]
  },
  "lwm2m": {
    "serverHost": "iot.1nce.com",
    "serverPort": 5684,
    "endpointName": "MarineIoT-PROD",
    "lifetime": 600,
    "queue_mode": true,
    "security": {
      "dtls_enabled": true,
      "psk_identity": "${VESSEL_PSK_ID}",
      "psk_key": "${VESSEL_PSK_KEY}"
    }
  },
  "system": {
    "log_level": "WARN",
    "hotreload_interval": 30000,
    "verbose_output": false,
    "performance_monitoring": true
  }
}
```

### **Vessel-Specific Configuration** (`vessels/yacht-001.json`)
```json
{
  "lwm2m": {
    "endpointName": "MarineIoT-YACHT-001"
  },
  "vessel_info": {
    "name": "Ocean Explorer",
    "type": "sailing_yacht",
    "length": 15.2,
    "beam": 4.8,
    "draft": 2.1
  },
  "signalk_subscriptions": {
    "subscriptions": [
      {
        "path": "environment.wind.speedOverGround",
        "period_ms": 1000,
        "description": "Wind speed for sailing yacht"
      },
      {
        "path": "navigation.headingMagnetic",
        "period_ms": 500,
        "description": "Magnetic heading for navigation"
      }
    ]
  }
}
```

## 🚀 Usage Examples

### **Command Line Interface**
```bash
# Auto-detect environment from system variables
./signalk-lwm2m-client --auto-env

# Explicit environment specification
./signalk-lwm2m-client --env development
./signalk-lwm2m-client --env production
./signalk-lwm2m-client --env staging

# Environment + vessel-specific configuration
./signalk-lwm2m-client --env production --vessel yacht-001

# Environment + regional configuration
./signalk-lwm2m-client --env production --region europe

# Manual configuration inheritance
./signalk-lwm2m-client --base settings.base.json --env settings.production.json
```

### **Environment Detection**
```bash
# Set environment via environment variables
export SIGNALK_ENV=production
export VESSEL_ID=yacht-001
export DEPLOYMENT_REGION=europe
./signalk-lwm2m-client --auto-env

# Kubernetes deployment with ConfigMaps
# Base config in ConfigMap, environment in Secret
```

### **Configuration Validation**
```bash
# Validate configuration inheritance
./signalk-lwm2m-client --validate-config --env production --vessel yacht-001

# Show final merged configuration
./signalk-lwm2m-client --show-config --env production --dry-run

# List available environments and vessels
./signalk-lwm2m-client --list-environments
```

## ⚙️ Implementation Features

### **Smart Merging Logic**
- **Deep Object Merging** - Nested objects are merged recursively
- **Array Handling** - Arrays can be replaced or merged based on configuration
- **Type Safety** - Preserve data types during inheritance
- **Conflict Resolution** - Clear precedence rules for conflicting values

### **Variable Substitution**
```json
{
  "lwm2m": {
    "endpointName": "${VESSEL_ID}-${ENVIRONMENT}",
    "serverHost": "${LWM2M_SERVER_HOST:iot.1nce.com}",
    "psk_identity": "${VESSEL_PSK_ID}"
  }
}
```

### **Hot-Reload Integration**
- Monitor multiple configuration files simultaneously
- Detect changes in base OR environment-specific files
- Automatically re-merge and apply configurations
- Preserve inheritance chain during hot-reload

### **Validation & Error Handling**
- Schema validation for each configuration layer
- Missing file fallback strategies
- Invalid JSON error isolation
- Configuration drift detection

## 📊 Benefits

### **🔧 Development Experience**
- **Rapid Environment Switching** - Change entire configuration profile with single flag
- **Consistent Base Configuration** - Shared settings reduce configuration drift
- **Easy Testing** - Separate test configurations without affecting production
- **Local Development** - Override production settings for local testing

### **🚀 Production Operations**
- **Fleet Management** - Deploy same codebase across multiple vessels
- **Regional Deployments** - Adapt to different regional servers/regulations
- **Staged Rollouts** - Test configuration changes in staging before production
- **Environment Parity** - Consistent configuration patterns across environments

### **⚡ Operational Flexibility**
- **Quick Environment Migration** - Move vessel between development/production
- **A/B Testing** - Easy configuration experimentation
- **Emergency Reconfiguration** - Rapidly switch to backup servers/settings
- **Compliance Adaptation** - Different configurations for different regulatory zones

### **🛡️ Risk Reduction**
- **Configuration Validation** - Catch configuration errors before deployment
- **Rollback Capability** - Quick revert to known-good configurations
- **Change Tracking** - Clear audit trail of configuration modifications
- **Error Isolation** - Environment-specific issues don't affect other environments

## 🎯 Marine IoT Use Cases

### **Fleet Management Scenario**
```bash
# Charter yacht fleet with different configurations
./signalk-lwm2m-client --env charter --vessel yacht-001 --region caribbean
./signalk-lwm2m-client --env charter --vessel yacht-002 --region mediterranean

# Commercial shipping with regulatory compliance
./signalk-lwm2m-client --env commercial --vessel cargo-001 --region atlantic
./signalk-lwm2m-client --env commercial --vessel tanker-002 --region pacific
```

### **Development Workflow**
```bash
# Developer testing new sensor integration
./signalk-lwm2m-client --env development --test-mode

# Staging environment for integration testing
./signalk-lwm2m-client --env staging --vessel test-platform

# Production deployment
./signalk-lwm2m-client --env production --vessel production-001
```

### **Emergency Response**
```bash
# Switch to emergency monitoring configuration
./signalk-lwm2m-client --env emergency --high-frequency-mode

# Backup server configuration during primary server maintenance
./signalk-lwm2m-client --env production --server-failover
```

## 🔮 Advanced Features

### **Configuration Templates**
- Generate environment-specific configurations from templates
- Vessel configuration wizards for easy setup
- Best practice configuration patterns
- Compliance templates for different maritime regulations

### **Dynamic Configuration**
- Runtime configuration updates based on operational conditions
- Automatic environment detection based on network/location
- Configuration recommendations based on vessel type/size
- Adaptive configurations based on connectivity quality

### **Integration Capabilities**
- **GitOps Workflows** - Configuration as code with version control
- **Kubernetes ConfigMaps** - Cloud-native configuration management
- **Ansible/Terraform** - Infrastructure as code integration
- **CI/CD Pipelines** - Automated configuration testing and deployment

This environment-specific configuration inheritance system transforms your marine IoT platform from a single-configuration solution into a **flexible, enterprise-grade deployment platform** capable of managing complex multi-vessel, multi-environment marine operations with the operational sophistication required for commercial marine IoT deployments. 🌊⚡
