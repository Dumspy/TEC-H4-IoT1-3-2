# Configuration Instructions

## Setup

This project implements WiFi sniffing with triangulation using multiple ESP32 devices.

### Hardware Requirements
- 3+ ESP32 boards (esp32doit-devkit-v1 or similar)
- One board configured as MASTER
- Two or more boards configured as SLAVE

### Software Configuration

Edit `include/config.h` before uploading:

#### For MASTER device:
```cpp
#define IS_MASTER true
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
#define MQTT_SERVER "192.168.1.100"
#define MQTT_PORT 1883
#define DEVICE_X 0.0
#define DEVICE_Y 0.0
```

#### For SLAVE devices:
```cpp
#define IS_MASTER false
#define DEVICE_X 5.0    // Actual X position in meters
#define DEVICE_Y 0.0    // Actual Y position in meters
#define MASTER_MAC {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}  // Master's MAC address
```

### Physical Setup
1. Place ESP32s at known positions (e.g., corners of a room)
2. Measure distances between devices
3. Set DEVICE_X and DEVICE_Y coordinates for each device
4. Power on MASTER first to get its MAC address from serial output
5. Configure SLAVES with MASTER's MAC address
6. Upload code to all devices

### How It Works

1. **All devices** run WiFi promiscuous mode to sniff packets and measure RSSI
2. **SLAVE devices** send RSSI measurements + their position to MASTER via ESP-NOW
3. **MASTER device** collects data from slaves and its own measurements
4. **MASTER** calculates (x,y) position using trilateration
5. **MASTER** hashes MAC addresses (SHA-256) for privacy
6. **MASTER** publishes `{id, timestamp, x, y}` to MQTT

### Clean Architecture

✅ **Direct peer-to-peer**: Slaves send directly to MASTER (no broadcasting)
✅ **Single config source**: All settings in `include/config.h`
✅ **Minimal WiFi**: Only MASTER connects to infrastructure WiFi
✅ **Privacy-first**: SHA-256 MAC hashing before transmission

### MQTT Output Format
```json
{
  "id": "a1b2c3d4e5f6...",
  "timestamp": 1234567890,
  "x": 2.5,
  "y": 3.2
}
```

### Privacy & GDPR
- MAC addresses are hashed using SHA-256 before transmission
- Only hashed IDs are sent to MQTT
- Raw MAC addresses never leave the device
- Data retention: 10 seconds (RSSI_TIMEOUT)

### Troubleshooting
- Monitor serial output at 115200 baud
- Check WiFi connection on MASTER
- Verify MQTT broker is reachable
- Ensure SLAVE devices have correct MASTER_MAC
- Use `pio device monitor` to view output
