#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "config.h"
#include "wifi_sniffer.h"
#include "mqtt_client.h"

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
  delay(10);
}
