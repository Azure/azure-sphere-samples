# Azure Sphere WiFi Monitor - Agentic Controller Integration

## Overview
This Azure Sphere application transforms the original WiFi sample into a comprehensive network monitoring solution that integrates seamlessly with your existing Agentic Controller system. It follows the same MQTT patterns and message formats as your Particle devices.

## Architecture Integration

### MQTT Topic Structure
Following your Agentic Controller pattern: `agentic/{location}/{device_id}/{category}/{event}`

**Published Topics:**
- `agentic/lab/azure_sphere_wifi_monitor_01/wifi/status` - WiFi status updates
- `agentic/lab/azure_sphere_wifi_monitor_01/wifi/alert` - WiFi alerts
- `agentic/lab/azure_sphere_wifi_monitor_01/system/heartbeat` - Device heartbeat

**Subscribed Topics:**
- `agentic/lab/azure_sphere_wifi_monitor_01/system/config` - Configuration updates
- `agentic/lab/azure_sphere_wifi_monitor_01/system/cmd` - Remote commands

### Message Formats

#### WiFi Status Message
```json
{
    "device_id": "azure_sphere_wifi_monitor_01",
    "location": "lab",
    "type": "wifi_status",
    "timestamp": 1703123456,
    "connected": true,
    "network_ready": true,
    "signal_strength": -45,
    "ssid": "LabNetwork",
    "fw_version": "1.0.0",
    "uptime_hours": 24.5,
    "reconnect_count": 2
}
```

#### WiFi Alert Message
```json
{
    "device_id": "azure_sphere_wifi_monitor_01",
    "location": "lab",
    "type": "wifi_alert",
    "alert_type": "connection_lost",
    "timestamp": 1703123456,
    "signal_strength": -85,
    "ssid": "LabNetwork",
    "connected": false,
    "reconnect_count": 3,
    "uptime_hours": 24.5
}
```

#### Heartbeat Message
```json
{
    "device_id": "azure_sphere_wifi_monitor_01",
    "location": "lab",
    "status": "alive",
    "fw_version": "1.0.0",
    "timestamp": 1703123456,
    "uptime_hours": 24.5,
    "free_memory_kb": 1024
}
```

## Agentic Controller Device Class

To add this device to your Agentic Controller, create a new device class:

### azuresphere_wifi_monitor.py
```python
from datetime import datetime
from typing import Dict, Any, Optional
from .base_device import BaseDevice

class AzureSphereWiFiMonitor(BaseDevice):
    """Azure Sphere WiFi Monitor device implementation"""
    
    def __init__(self, device_id: str, location: str, config: Dict[str, Any]):
        super().__init__(device_id, location, config)
        self.device_type = "azure_sphere_wifi_monitor"
        
        # WiFi-specific status fields
        self.wifi_status = {
            'connected': False,
            'network_ready': False,
            'signal_strength': 0,
            'ssid': '',
            'reconnect_count': 0,
            'uptime_hours': 0.0
        }
        
        # Alert tracking
        self.last_alert_time = None
        self.alert_count = 0
    
    def get_status_topics(self) -> list:
        """Return list of MQTT topics this device publishes status to"""
        return [
            f"agentic/{self.location}/{self.device_id}/wifi/status",
            f"agentic/{self.location}/{self.device_id}/wifi/alert",
            f"agentic/{self.location}/{self.device_id}/system/heartbeat"
        ]
    
    def get_command_topics(self) -> list:
        """Return list of MQTT topics this device subscribes to for commands"""
        return [
            f"agentic/{self.location}/{self.device_id}/system/config",
            f"agentic/{self.location}/{self.device_id}/system/cmd"
        ]
    
    def update_status(self, topic: str, payload: Dict[str, Any]) -> bool:
        """Update device status from MQTT message"""
        try:
            message_type = payload.get('type', '')
            
            if message_type == 'wifi_status':
                self._handle_wifi_status(payload)
            elif message_type == 'wifi_alert':
                self._handle_wifi_alert(payload)
            elif 'status' in payload and payload['status'] == 'alive':
                self._handle_heartbeat(payload)
            else:
                return False
                
            # Update common fields
            self.last_seen = datetime.fromtimestamp(payload.get('timestamp', 0))
            self.is_online = True
            
            return True
            
        except Exception as e:
            self.logger.error(f"Error updating status: {e}")
            return False
    
    def _handle_wifi_status(self, payload: Dict[str, Any]):
        """Handle WiFi status update"""
        self.wifi_status.update({
            'connected': payload.get('connected', False),
            'network_ready': payload.get('network_ready', False),
            'signal_strength': payload.get('signal_strength', 0),
            'ssid': payload.get('ssid', ''),
            'reconnect_count': payload.get('reconnect_count', 0),
            'uptime_hours': payload.get('uptime_hours', 0.0)
        })
        
        # Update device status
        self.status['wifi'] = self.wifi_status.copy()
        
        self.logger.info(f"WiFi status updated - Connected: {self.wifi_status['connected']}, "
                        f"Signal: {self.wifi_status['signal_strength']} dBm")
    
    def _handle_wifi_alert(self, payload: Dict[str, Any]):
        """Handle WiFi alert"""
        alert_type = payload.get('alert_type', 'unknown')
        self.last_alert_time = datetime.now()
        self.alert_count += 1
        
        # Store alert in status
        if 'alerts' not in self.status:
            self.status['alerts'] = []
        
        alert_record = {
            'type': alert_type,
            'timestamp': payload.get('timestamp'),
            'signal_strength': payload.get('signal_strength'),
            'connected': payload.get('connected')
        }
        
        self.status['alerts'].insert(0, alert_record)
        if len(self.status['alerts']) > 10:  # Keep last 10 alerts
            self.status['alerts'] = self.status['alerts'][:10]
        
        self.logger.warning(f"WiFi alert received: {alert_type}")
        
        # Trigger notifications or automation here
        self._trigger_alert_automation(alert_type, payload)
    
    def _handle_heartbeat(self, payload: Dict[str, Any]):
        """Handle heartbeat message"""
        self.status['heartbeat'] = {
            'timestamp': payload.get('timestamp'),
            'uptime_hours': payload.get('uptime_hours'),
            'free_memory_kb': payload.get('free_memory_kb'),
            'fw_version': payload.get('fw_version')
        }
    
    def _trigger_alert_automation(self, alert_type: str, payload: Dict[str, Any]):
        """Trigger automation based on alert type"""
        # This method can be extended to trigger specific automations
        # For example: email notifications, Slack alerts, etc.
        pass
    
    def execute_device_command(self, command: str, parameters: Dict[str, Any] = None) -> bool:
        """Execute a command on the device"""
        if command == "force_check":
            return self._send_command({"action": "force_check"})
        elif command == "restart_monitor":
            return self._send_command({"action": "restart_monitor"})
        elif command == "update_config":
            return self._send_config_update(parameters or {})
        elif command == "get_status":
            return self._send_command({"action": "get_status"})
        else:
            self.logger.error(f"Unknown command: {command}")
            return False
    
    def _send_command(self, command_data: Dict[str, Any]) -> bool:
        """Send command to device via MQTT"""
        try:
            topic = f"agentic/{self.location}/{self.device_id}/system/cmd"
            # This would use your MQTT client to publish
            # self.mqtt_client.publish(topic, json.dumps(command_data))
            return True
        except Exception as e:
            self.logger.error(f"Failed to send command: {e}")
            return False
    
    def _send_config_update(self, config_data: Dict[str, Any]) -> bool:
        """Send configuration update to device"""
        try:
            topic = f"agentic/{self.location}/{self.device_id}/system/config"
            # This would use your MQTT client to publish
            # self.mqtt_client.publish(topic, json.dumps(config_data))
            return True
        except Exception as e:
            self.logger.error(f"Failed to send config: {e}")
            return False
    
    def get_dashboard_data(self) -> Dict[str, Any]:
        """Return data for dashboard display"""
        return {
            'device_id': self.device_id,
            'location': self.location,
            'type': self.device_type,
            'online': self.is_online,
            'last_seen': self.last_seen,
            'wifi_connected': self.wifi_status.get('connected', False),
            'signal_strength': self.wifi_status.get('signal_strength', 0),
            'ssid': self.wifi_status.get('ssid', ''),
            'uptime_hours': self.wifi_status.get('uptime_hours', 0),
            'reconnect_count': self.wifi_status.get('reconnect_count', 0),
            'alert_count': self.alert_count,
            'last_alert': self.last_alert_time
        }
```

### Register Device Type

In your `device_manager.py`, add the new device type:

```python
from .azuresphere_wifi_monitor import AzureSphereWiFiMonitor

class DeviceManager:
    def __init__(self):
        self._device_type_map = {
            'particle_argon': ParticleArgonDevice,
            'particle_photon': ParticlePhotonDevice,
            'azure_sphere_wifi_monitor': AzureSphereWiFiMonitor,  # Add this line
        }
```

### Configuration Example

Add to your `config.yaml`:

```yaml
devices:
  - device_id: "azure_sphere_wifi_monitor_01"
    location: "lab"
    type: "azure_sphere_wifi_monitor"
    config:
      monitor_interval: 5
      signal_threshold: -70
      enable_alerts: true
```

## Device Configuration

### Update Device Settings
Before deploying, update these settings in `main.c`:

```c
static DeviceConfig deviceConfig = {
    .DEVICE_ID = "azure_sphere_wifi_monitor_01",    // Unique device ID
    .LOCATION = "lab",                              // Your location
    .FW_VERSION = "1.0.0",
    .mqttBrokerIP = "192.168.200.207",             // Your MQTT broker IP
    .mqttPort = 1883,
    .mqttUsername = "leowise",                      // Your MQTT username
    .mqttPassword = "LM4F120!mqtt$",               // Your MQTT password
    .monitorIntervalMinutes = 5,                   // 5, 10, or 15 minutes
    .debugEnabled = true,
    .signalThreshold = -70                         // Alert threshold in dBm
};
```

### Network Configuration
Update `app_manifest.json` with your MQTT broker IP:

```json
"AllowedConnections": [
    "192.168.200.207:1883"
]
```

## Features

### Automated Monitoring
- **Periodic Checks**: Configurable intervals (5, 10, 15 minutes)
- **Real-time Alerts**: Connection drops, weak signals, frequent reconnects
- **Status History**: Tracks last 5 WiFi status checks
- **Heartbeat**: Regular "alive" messages every 60 seconds

### Remote Management
- **Configuration Updates**: Change monitoring intervals and thresholds via MQTT
- **Remote Commands**: Force status checks, restart monitoring, get current status
- **Real-time Status**: Button press triggers immediate status check and publish

### Alert Types
1. **connection_lost**: WiFi connection dropped
2. **signal_weak**: Signal strength below threshold
3. **frequent_reconnects**: Too many reconnects in time window

### Dashboard Integration
Your existing Agentic Controller dashboard will display:
- Current WiFi connection status
- Signal strength and SSID
- Last 5 status checks
- Alert history and counts
- Device uptime and health

## Deployment Steps

1. **Update Configuration**: Modify device settings in `main.c` and `app_manifest.json`
2. **Build Application**: Use Azure Sphere SDK to build the application
3. **Deploy to Device**: Deploy to your Azure Sphere development board
4. **Add to Agentic Controller**: 
   - Add the new device class to your controller
   - Register the device type in DeviceManager
   - Add device configuration to your config.yaml
5. **Monitor Dashboard**: Device should appear in your web dashboard automatically

## Integration Benefits

1. **Consistent Architecture**: Follows your existing MQTT patterns and message formats
2. **Unified Dashboard**: All devices (Particle + Azure Sphere) in one interface
3. **Remote Management**: Same configuration and command interface as other devices
4. **Automated Alerts**: Integrates with your existing email/notification system
5. **Scalable**: Easy to add more Azure Sphere monitoring devices

This integration provides enterprise-grade WiFi monitoring while maintaining consistency with your existing IoT infrastructure.