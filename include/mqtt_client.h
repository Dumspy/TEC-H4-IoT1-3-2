#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

void initMQTT();
void connectWiFi();
void connectMQTT();
void publishPosition(const char* deviceId, float x, float y);
void loopMQTT();

#endif
