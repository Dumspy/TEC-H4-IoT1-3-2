#ifndef CONFIG_H
#define CONFIG_H

#define IS_MASTER false 

#define WIFI_SSID "TEC-IOT"
#define WIFI_PASSWORD "42090793"

#define MQTT_SERVER "wilson.local"
#define MQTT_PORT 1883
#define MQTT_TOPIC "temp/devices/position"

#define DEVICE_X 5.0
#define DEVICE_Y 5.0

#define MASTER_MAC {0x70, 0xB8, 0xF6, 0x5B, 0x55, 0x00}

#define MAX_DEVICES 50
#define RSSI_TIMEOUT 10000
#define MQTT_PUBLISH_INTERVAL 5000
#define SLAVE_REPORT_INTERVAL 2000

#endif