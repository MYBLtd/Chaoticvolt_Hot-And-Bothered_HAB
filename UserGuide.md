# Temperature Monitoring System - User Guide

## Table of Contents
1. [Initial Setup](#initial-setup)
2. [Web Interface](#web-interface)
3. [MQTT Configuration](#mqtt-configuration)
4. [Sensor Management](#sensor-management)
5. [Display Configuration](#display-configuration)
6. [Troubleshooting](#troubleshooting)

## Initial Setup

### Hardware Requirements
- ESP32 Development Board
- DS18B20 Temperature Sensors (OneWire)
- TM1637 4-Digit Display
- 2x Relays (Optional)
- Power Supply (5V)

### Hardware Installation
1. **Wiring Connections**

ESP32 Pin Mapping:

OneWire Data: GPIO_4
Display CLK: GPIO_16
Display DIO: GPIO_17
Relay 1: GPIO_32
Relay 2: GPIO_33

2. **Network Setup**
- Connect Ethernet cable
- Device will obtain IP via DHCP
- Default hostname: `tempmon`

3. **First Boot**
- Power on the device
- Wait for display test sequence
- Device will show "----" when ready

### Initial Access
1. **Find Device IP**
- Check your router's DHCP client list
- Device hostname: `tempmon`
- Default mDNS address: `tempmon.local`

2. **Login**
- Open web browser to `http://<device-ip>`
- Default credentials:
  - Username: `admin`
  - Password: `admin`
- **Important**: Change default password immediately

## Web Interface

### Dashboard Overview
1. **Main Display**
- Real-time temperature readings
- Sensor status indicators
- Temperature history graph
- Relay status and controls

2. **Navigation**
- Dashboard: Main monitoring view
- Preferences: System configuration
- Logout: End session

### Temperature Monitoring
1. **Graph Features**
- Real-time updates
- Historical data (last 24 hours)
- Multiple sensor overlay
- Temperature trend analysis

2. **Relay Control**
- Manual relay toggling
- Status indicators
- Last state display

## MQTT Configuration

### Broker Setup
1. **Access MQTT Settings**
- Navigate to Preferences
- Locate MQTT Configuration section

2. **Required Settings**
Broker Address: your-mqtt-broker
Port: 8883 (TLS) or 1883 (non-TLS)
Username: your-mqtt-username
Password: your-mqtt-password

### Topic Structure
system_name/device_id/sensors/<sensor_id>/temperature
system_name/device_id/sensors/<sensor_id>/status
system_name/device_id/relay/<relay_id>/state

### Data Format
- Temperature: Float value (°C)
- Status: "online" or "error"
- Relay State: "ON" or "OFF"

## Sensor Management

### Adding Sensors
1. **Automatic Detection**
   - Connect sensor to OneWire bus
   - System automatically detects new sensors
   - Allow up to 30 seconds for detection (or longer depending on settings on preferences page)

2. **Sensor Naming**
   - Navigate to Preferences
   - Locate detected sensor by ID
   - Assign meaningful name
   - Save changes
   Change only one or a few at the time. Page refresh will wipe you entries!!
   This is a bug, but my time has ran out.

### Display Sensor Selection
1. **Choose Display Sensor**
   - Select sensor from dropdown
   - System saves selection
   - Display updates immediately

### Sensor Maintenance
1. **Regular Checks**
   - Verify physical connections
   - Check sensor readings
   - Monitor error rates

2. **Calibration**
   - System uses factory calibration
   - No manual calibration required
   - Replace sensor if accuracy degrades

## Display Configuration

### Display Settings
1. **Brightness Control**
   - Adjustable brightness (1-15)
   - Default: 7
   - Configure in Preferences

2. **Update Interval**
   - Default: 1 second
   - Configurable in firmware
   - Balance between responsiveness and stability

### Display Modes
1. **Normal Operation**
   - Shows current temperature
   - Decimal point indicates valid reading
   - Updates continuously

2. **Status Messages**
   - "----": Initializing
   - "Err": Sensor error
   - "Lost": Sensor disconnected

## Troubleshooting

### Common Issues

1. **No Display Output**
   - Check power supply
   - Verify display connections
   - Confirm display selection

2. **Invalid Readings**
Symptoms:

"Err" on display
Incorrect temperatures
Fluctuating values

Solutions:

Check sensor connections
Verify wiring polarity
Replace sensor if necessary

3. **Network Issues**

Symptoms:

No web interface
MQTT disconnections
Delayed updates

Solutions:

Check Ethernet connection
Verify network settings
Confirm MQTT broker status

4. **Authentication Problems**

Symptoms:

Login failures
Session timeouts
Access denied errors

Solutions:

Clear browser cache
Verify credentials
Reset authentication if necessary

### Hardware Reset
1. **Credential Reset**
- Locate reset button
- Hold for 10 seconds
- Release when display shows "rst"
- Default credentials restored

2. **System Reset**
- Power cycle device
- Wait 30 seconds
- Check display for status
- Verify network connection

### Error Messages
| Display | Web Interface | Meaning | Action |
|---------|---------------|---------|---------|
| Err | Error | Sensor fault | Check connections |
| ---- | Initializing | System startup | Wait 30 seconds |
| Lost | Disconnected | No sensor | Check wiring |
| rst | Reset | System resetting | Wait for completion |

### Support Resources
1. **Documentation**
- User guide (this document)
- API documentation
- Hardware specifications

2. **Community Support**
- GitHub Issues
- Project Wiki
- Discussion Forums

3. **System Logs**
- Serial output (115200 baud)
- Web interface logs
- MQTT debug messages

