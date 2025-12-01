# ESP32 WiFi Sniffer with Triangulation - Quick Start

## Current Configuration

Your config.h is set up with:
- WiFi: `TEC-IOT` / `42090793`
- MQTT Server: `wilson.local:1883`
- MQTT Topic: `temp/devices/position`

## Setup Steps

### 1. Configure and Upload Master Device
In `include/config.h`:
```cpp
#define IS_MASTER true
#define DEVICE_X 0.0
#define DEVICE_Y 0.0
```

Upload to ESP32, open Serial Monitor (115200 baud), and **copy the MAC address** displayed:
```
MASTER MAC Address: AA:BB:CC:DD:EE:FF
Configure slaves with this MAC in config.h
```

### 2. Configure Slave Devices
For each slave, edit `include/config.h`:
```cpp
#define IS_MASTER false
#define DEVICE_X 5.0    // Set actual X position in meters
#define DEVICE_Y 3.0    // Set actual Y position in meters
#define MASTER_MAC {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}  // Use Master's MAC from step 1
```

Upload to each slave ESP32.

### 3. Physical Setup
- Place Master at (0, 0)
- Place Slaves at known coordinates (measure accurately!)
- Example layout:
  ```
  Slave2 (0,5)----Slave3 (5,5)
      |              |
      |    Devices   |
      |   detected   |
      |     here     |
  Master (0,0)----Slave1 (5,0)
  ```

### 4. Monitor Output

**Master:**
```
WiFi Sniffer MASTER Node
MASTER MAC Address: AA:BB:CC:DD:EE:FF
Configure slaves with this MAC in config.h
Connecting to WiFi...
WiFi connected
IP: 192.168.1.100
Connecting to MQTT...connected
Promiscuous mode enabled - Sniffing started
Device: a1b2c3d4... Position: (2.5, 3.2)
```

**Slave:**
```
WiFi Sniffer SLAVE Node
Master MAC configured as: AA:BB:CC:DD:EE:FF
Slave position: X=5.0, Y=3.0
Promiscuous mode enabled - Sniffing started
```

### 5. MQTT Output

Subscribe to `temp/devices/position`:
```bash
mosquitto_sub -h wilson.local -t temp/devices/position
```

Output:
```json
{"id":"a1b2c3d4e5f6...","timestamp":1234567890,"x":2.50,"y":3.20}
```

## How It Works

1. **All devices** sniff WiFi packets in promiscuous mode (captures RSSI)
2. **Slaves** send `{MAC, RSSI, X, Y}` to Master via ESP-NOW every 2s
3. **Master** collects RSSI data from slaves + its own measurements
4. **Master** calculates position using trilateration (needs 3+ measurements)
5. **Master** hashes MAC (SHA-256) for privacy compliance
6. **Master** publishes to MQTT every 5s

## Key Configuration Changes

✅ **No more duplication**: `MASTER_MAC` is only in `config.h`
✅ **Automatic MAC display**: Master shows its MAC on boot
✅ **Clear slave setup**: Just copy the MAC from master's serial output

## Troubleshooting

- Use `pio device monitor` or Arduino Serial Monitor at 115200 baud
- Verify MQTT broker: `ping wilson.local`
- Check Master MAC is correctly copied to slave configs (including `0x` prefix!)
- Ensure devices are within ESP-NOW range (~100-200m line of sight)
- Need 3+ devices for accurate triangulation (2 devices = midpoint only)
- WiFi channel: all devices should be on same channel

## Privacy (GDPR)

✅ MAC addresses are SHA-256 hashed before transmission
✅ Raw MACs never leave the device
✅ 10-second data retention (RSSI_TIMEOUT)
✅ No persistent storage of personal data
