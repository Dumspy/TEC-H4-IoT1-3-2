#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdint.h>

void initMQTT();
void connectWiFi();
void connectMQTT();
void publishSniffedDevice(const char* hashedMac, int rssi, float sensorX, float sensorY);
void loopMQTT();

#endif
