# Security Policy

## Overview
This document outlines security procedures and general policies for the Temperature Monitoring System. The system includes hardware components (ESP32, sensors, relays) and software components (web interface, MQTT communication).

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.2.5   | :white_check_mark: |
| 1.2.6   | :white_check_mark: |
| 1.3.2   | :white_check_mark: |

## Security Features

### Overview
The authentication system uses a session-based approach with secure token management.

### Implementation Details
#### Session Management (`AuthManager.cpp`)
```cpp
- Session lifetime: 3600 seconds (1 hour)
- Token length: 32 characters
- Token character set: [0-9a-zA-Z]
- Active session limit: Determined by available memory

### Authentication
- Built-in user authentication system
- Configurable username/password
- Session-based web interface security
- Hardware reset capability for credential recovery

### Network Security
- TLS/SSL encryption for MQTT communication
- Secure WebSocket support
- Certificate validation
- Configurable MQTT credentials

### Hardware Security
- Physical reset button requires 10-second hold (prevents accidental resets)
- Sensor data validation
- Configuration stored in protected flash memory
- Watchdog timer implementation

## Default Settings

**Important**: Change these defaults immediately after initial setup:
- Default username: `admin`
- Default password: `admin`
- Default MQTT security: TLS enabled
- Default web interface: HTTP (configure HTTPS if needed)
```
## Reporting a Vulnerability

### When to Report
Report a vulnerability if you discover:
- Authentication bypass possibilities
- Network security weaknesses
- Hardware exploitation methods
- Data exposure risks
- Configuration vulnerabilities

### How to Report
1. **Do not** create a public GitHub issue for security vulnerabilities
2. Send details to mybltd@mailfence.com
3. Include:
   - Type of vulnerability
   - Steps to reproduce
   - Affected versions
   - Potential impact
   - (Optional) Suggested fix

### Response Timeline
- Since I am the sole contributor an pretty busy with other thing,
no guarantee is given.

## Best Practices for Deployment

### Network Configuration
1. Place device behind firewall
2. Use VPN for remote access
3. Configure secure MQTT broker
4. Enable TLS for all communications
5. Use strong, unique passwords

### Physical Security
1. Limit physical access to device
2. Secure sensor connections
3. Monitor for tampering
4. Implement secure boot if available
5. Use secure flash storage

### Software Security
1. Keep firmware updated
2. Monitor system logs
3. Regular security audits
4. Validate all sensor data
5. Implement rate limiting

## Known Security Limitations

1. Web interface session timeout fixed at 1 hour
2. HTTP basic auth as fallback
3. No hardware encryption for sensor data
4. Limited brute force protection

## Security-Related Configuration

### MQTT Security
```ini
mqtt.broker=your-broker
mqtt.port=8883  # TLS port
mqtt.username=your-username
mqtt.password=your-password
mqtt.tls=true

