#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "config.h"
#include "types.h"
#include "utils.h"
#include "wifi_sniffer.h"
#include "mqtt_client.h"

SniffedDevice devices[MAX_DEVICES];
int deviceCount = 0;

unsigned long lastMqttPublish = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println("WiFi Sniffer Node");
  Serial.println("=================================");
  
  Serial.print("Device Position: X=");
  Serial.print(DEVICE_X);
  Serial.print(", Y=");
  Serial.println(DEVICE_Y);
  Serial.print("WiFi SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("MQTT Server: ");
  Serial.print(MQTT_SERVER);
  Serial.print(":");
  Serial.println(MQTT_PORT);
  Serial.print("MQTT Topic: ");
  Serial.println(MQTT_TOPIC);
  Serial.println("=================================\n");

  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  
  connectWiFi();
  initMQTT();
  connectMQTT();
  
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
  initWiFiSniffer();
  
  Serial.println("\nPromiscuous mode enabled - Sniffing started\n");
}

void loop() {
  loopMQTT();
  
  if (millis() - lastMqttPublish > MQTT_PUBLISH_INTERVAL) {
    int publishCount = 0;
    
    for (int i = 0; i < deviceCount; i++) {
      if (millis() - devices[i].timestamp < RSSI_TIMEOUT) {
        char hashedMac[65];
        hashMac(devices[i].mac, hashedMac);
        
        publishSniffedDevice(hashedMac, devices[i].rssi, DEVICE_X, DEVICE_Y);
        publishCount++;
        
        delay(10);
      }
    }
    
    #if DEBUG_LEVEL >= 1
    Serial.print("[PUBLISH] Sent ");
    Serial.print(publishCount);
    Serial.print(" devices (");
    Serial.print(deviceCount);
    Serial.println(" tracked)");
    #endif
    
    lastMqttPublish = millis();
  }
  
  delay(10);
}
