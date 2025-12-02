#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <PubSubClient.h>
#include <time.h>
#include "mqtt_client.h"
#include "config.h"
#include "utils.h"

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
    
    // Sync time with NTP server
    // Denmark: UTC+2 during daylight saving (summer time)
    configTime(TZ_OFFSET_SECONDS, TZ_DST_OFFSET, "pool.ntp.org", "time.nist.gov");
    Serial.println("Syncing time with NTP...");
    time_t now = time(nullptr);
    int attempts = 0;
    while (now < 24 * 3600 && attempts < 50) {
      delay(100);
      now = time(nullptr);
      attempts++;
    }
    if (now > 24 * 3600) {
      Serial.print("Time synced: ");
      Serial.println(ctime(&now));
    } else {
      Serial.println("WARNING: NTP sync timeout");
    }
    
    uint8_t channel;
    wifi_second_chan_t second;
    esp_wifi_get_channel(&channel, &second);
    Serial.print("WiFi Channel after connect: ");
    Serial.println(channel);
    
    if (channel != 6) {
      Serial.println("WARNING: Channel changed! Re-setting to 6...");
      esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
      esp_wifi_get_channel(&channel, &second);
      Serial.print("Channel now: ");
      Serial.println(channel);
    }
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

void publishSniffedDevice(const char* hashedMac, int rssi, float sensorX, float sensorY) {
  if (!mqttClient.connected()) return;
  
  char payload[256];
  char timestamp[32];
  time_t now = time(nullptr);
  formatTimestamp(now, timestamp, sizeof(timestamp));
  
  snprintf(payload, sizeof(payload), 
    "{\"device_id\":\"%s\",\"rssi\":%d,\"sensor_x\":%.2f,\"sensor_y\":%.2f,\"timestamp\":\"%s\"}",
    hashedMac, rssi, sensorX, sensorY, timestamp);
  
  bool success = mqttClient.publish(MQTT_TOPIC, payload);
  
  #if DEBUG_LEVEL >= 2
  if (success) {
    Serial.print("[MQTT] Published: ");
    Serial.println(payload);
  } else {
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
