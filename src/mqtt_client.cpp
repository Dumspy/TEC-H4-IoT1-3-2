#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt_client.h"
#include "config.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void initMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed");
  }
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5s");
      delay(5000);
    }
  }
}

void publishPosition(const char* deviceId, float x, float y) {
  if (!mqttClient.connected()) return;
  
  char payload[200];
  unsigned long timestamp = millis();
  
  snprintf(payload, sizeof(payload), 
    "{\"id\":\"%s\",\"timestamp\":%lu,\"x\":%.2f,\"y\":%.2f}",
    deviceId, timestamp, x, y);
  
  bool success = mqttClient.publish(MQTT_TOPIC, payload);
  
  #if DEBUG_LEVEL >= 2 && IS_MASTER
  if (!success) {
    Serial.println("[MQTT] Publish FAILED");
  }
  #endif
}

void loopMQTT() {
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();
}
