# Agent Guidelines for PlatformIO ESP32 Project

## Build/Test Commands
- Build: `pio run` or `platformio run`
- Upload to device: `pio run --target upload`
- Clean build: `pio run --target clean`
- Run tests: `pio test`
- Run specific test: `pio test --filter <test_name>`
- Monitor serial: `pio device monitor`

## Code Style (C++/Arduino)
- Language: C++ with Arduino framework for ESP32 (espressif32 platform)
- Board: esp32doit-devkit-v1
- Include `<Arduino.h>` as primary header
- Function declarations before `setup()` and `loop()`
- Function definitions after `loop()`
- Use camelCase for function names (e.g., `myFunction`)
- Use lower_case for variables following Arduino conventions
- Comments: Use `//` for single-line comments
- Structure: setup() runs once at boot, loop() runs continuously
- Indentation: 2 spaces (Arduino standard)
- Type safety: Use explicit types (int, float, etc.)
- Error handling: Check return values, use Serial for debugging

## Project Architecture

### Communication Flow
```
Phone WiFi → Each ESP32 sniffs (RSSI) → Hash MAC → Publish to MQTT
                                                          ↓
                                             Backend aggregates data
                                                          ↓
                                             Backend performs triangulation
```

### Device Roles
- **All devices**: Sniff WiFi packets, hash MAC addresses, publish RSSI readings to MQTT
- All devices run identical code and operate independently

### Configuration
- All settings in `include/config.h`
- Set `DEVICE_X/Y` for physical position of each sensor
- All devices connect to WiFi infrastructure
- All devices publish to same MQTT topic

### MQTT Payload Format
Each device publishes JSON with:
- `device_id`: SHA-256 hashed MAC address (GDPR compliance)
- `rssi`: Signal strength in dBm
- `sensor_x`: X position of this sensor
- `sensor_y`: Y position of this sensor
- `timestamp`: Milliseconds since boot

### Key Design Decisions
- All devices connect to WiFi infrastructure (no ESP-NOW)
- SHA-256 hashing for MAC addresses (GDPR compliance)
- Backend responsible for triangulation (not on-device)
- Simple, independent operation per device
- Easy to scale by adding more sensors
