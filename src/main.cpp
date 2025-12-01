#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "config.h"
#include "types.h"
#include "utils.h"
#include "wifi_sniffer.h"
#include "espnow_comm.h"
#include "triangulation.h"
#include "mqtt_client.h"

SniffedDevice devices[MAX_DEVICES];
int deviceCount = 0;

TriangulationData triangulationBuffer[MAX_DEVICES];
int triangulationCount = 0;

extern uint8_t masterAddress[];
unsigned long lastMqttPublish = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  if (IS_MASTER) {
    Serial.println("WiFi Sniffer MASTER Node");
  } else {
    Serial.println("WiFi Sniffer SLAVE Node");
  }
  Serial.println("=================================");
  
  Serial.print("Device Position: X=");
  Serial.print(DEVICE_X);
  Serial.print(", Y=");
  Serial.println(DEVICE_Y);
  Serial.print("WiFi Channel: 6");
  Serial.print("\nWiFi SSID: ");
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
  
  if (IS_MASTER) {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    Serial.print("MASTER MAC Address: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", mac[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println("\nConfigure slaves with this MAC in config.h");
  }
  
  initESPNow();
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
  
  if (IS_MASTER) {
    Serial.println("Channel set to 6 for ESP-NOW");
    delay(100);
    connectWiFi();
    initMQTT();
    connectMQTT();
  } else {
    Serial.print("Master MAC configured as: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", masterAddress[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
  }
  
  initWiFiSniffer();
  
  Serial.println("\nPromiscuous mode enabled - Sniffing started\n");
}

void loop() {
  if (IS_MASTER) {
    loopMQTT();
    
    static unsigned long lastMasterReport = 0;
    if (millis() - lastMasterReport > SLAVE_REPORT_INTERVAL) {
      processMasterSniffedDevices();
      lastMasterReport = millis();
    }
    
    if (millis() - lastMqttPublish > MQTT_PUBLISH_INTERVAL) {
      processDevices();
      lastMqttPublish = millis();
    }
  } else {
    static unsigned long lastReport = 0;
    if (millis() - lastReport > SLAVE_REPORT_INTERVAL) {
      int reportsSent = 0;
      for (int i = 0; i < deviceCount; i++) {
        if (millis() - devices[i].timestamp < RSSI_TIMEOUT) {
          SlaveReport report;
          memcpy(report.mac, devices[i].mac, 6);
          report.rssi = devices[i].rssi;
          report.x = DEVICE_X;
          report.y = DEVICE_Y;
          
          esp_now_send(masterAddress, (uint8_t*)&report, sizeof(report));
          delay(10);
          
          reportsSent++;
        }
      }
      
      #if DEBUG_LEVEL >= 1
      if (reportsSent > 0) {
        Serial.print("[SLAVE] Sent ");
        Serial.print(reportsSent);
        Serial.print(" reports (");
        Serial.print(deviceCount);
        Serial.println(" tracked)");
      }
      #endif
      
      lastReport = millis();
    }
  }
  
  delay(10);
}
