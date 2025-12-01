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
Phone WiFi → All ESP32s sniff (RSSI) → Slaves send to Master via ESP-NOW
                                      ↓
                          Master triangulates position
                                      ↓
                          Hash MAC + publish to MQTT
```

### Device Roles
- **MASTER**: Receives ESP-NOW data, triangulates, publishes to MQTT
- **SLAVE**: Sniffs WiFi, sends RSSI+position to MASTER

### Configuration
- All settings in `include/config.h`
- Set `IS_MASTER true/false` per device
- Set `DEVICE_X/Y` for physical position
- Set `MASTER_MAC` on slaves after getting master's MAC

### Key Design Decisions
- No broadcast address needed (slaves send directly to master)
- Only MASTER connects to WiFi infrastructure
- SHA-256 hashing for MAC addresses (GDPR compliance)
- ESP-NOW for low-latency peer-to-peer communication
