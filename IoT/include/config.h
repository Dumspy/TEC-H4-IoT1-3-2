#ifndef CONFIG_H
#define CONFIG_H

#define WIFI_SSID "TEC-IOT"
#define WIFI_PASSWORD "42090793"

#define MQTT_SERVER "wilson.local"
#define MQTT_PORT 1883
#define MQTT_TOPIC "wifi/sniff"

#define DEVICE_X 10.0
#define DEVICE_Y 10.0

#define MAX_DEVICES 50
#define RSSI_TIMEOUT 10000
#define MQTT_PUBLISH_INTERVAL 5000

#define DEBUG_LEVEL 2

#endif