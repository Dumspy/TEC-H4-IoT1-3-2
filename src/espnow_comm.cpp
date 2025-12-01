#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "espnow_comm.h"
#include "types.h"
#include "utils.h"
#include "config.h"

uint8_t masterAddress[] = MASTER_MAC;

void initESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  
  if (!IS_MASTER) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterAddress, 6);
    peerInfo.channel = 6;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (!IS_MASTER) return;
  
  if (!esp_now_is_peer_exist(mac_addr)) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_addr, 6);
    peerInfo.channel = 6;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    
    #if DEBUG_LEVEL >= 2
    Serial.println("[MASTER] Registered new slave");
    #endif
  }
  
  SlaveReport* report = (SlaveReport*)data;
  
  int idx = findTriangulationData(report->mac);
  if (idx == -1 && triangulationCount < MAX_DEVICES) {
    idx = triangulationCount++;
    memcpy(triangulationBuffer[idx].mac, report->mac, 6);
    triangulationBuffer[idx].reportCount = 0;
  }
  
  if (idx != -1) {
    int posSlot = findPositionSlot(idx, report->x, report->y);
    
    if (posSlot != -1) {
      triangulationBuffer[idx].rssi[posSlot] = report->rssi;
      triangulationBuffer[idx].timestamp = millis();
    } else if (triangulationBuffer[idx].reportCount < 3) {
      int pos = triangulationBuffer[idx].reportCount;
      triangulationBuffer[idx].rssi[pos] = report->rssi;
      triangulationBuffer[idx].positions[pos][0] = report->x;
      triangulationBuffer[idx].positions[pos][1] = report->y;
      triangulationBuffer[idx].reportCount++;
      triangulationBuffer[idx].timestamp = millis();
    }
    
    #if DEBUG_LEVEL >= 2
    Serial.print("[MASTER] Rx from (");
    Serial.print(report->x);
    Serial.print(",");
    Serial.print(report->y);
    Serial.print(") | RSSI: ");
    Serial.print(report->rssi);
    Serial.print(" | Reports: ");
    Serial.print(triangulationBuffer[idx].reportCount);
    Serial.println("/3");
    #endif
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  #if DEBUG_LEVEL >= 2 && !IS_MASTER
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("[ESP-NOW] Send FAILED!");
  }
  #endif
}

void processMasterSniffedDevices() {
  if (!IS_MASTER) return;
  
  for (int i = 0; i < deviceCount; i++) {
    if (millis() - devices[i].timestamp < RSSI_TIMEOUT) {
      int idx = findTriangulationData(devices[i].mac);
      if (idx == -1 && triangulationCount < MAX_DEVICES) {
        idx = triangulationCount++;
        memcpy(triangulationBuffer[idx].mac, devices[i].mac, 6);
        triangulationBuffer[idx].reportCount = 0;
      }
      
      if (idx != -1) {
        int posSlot = findPositionSlot(idx, DEVICE_X, DEVICE_Y);
        
        if (posSlot != -1) {
          triangulationBuffer[idx].rssi[posSlot] = devices[i].rssi;
          triangulationBuffer[idx].timestamp = millis();
        } else if (triangulationBuffer[idx].reportCount < 3) {
          int pos = triangulationBuffer[idx].reportCount;
          triangulationBuffer[idx].rssi[pos] = devices[i].rssi;
          triangulationBuffer[idx].positions[pos][0] = DEVICE_X;
          triangulationBuffer[idx].positions[pos][1] = DEVICE_Y;
          triangulationBuffer[idx].reportCount++;
          triangulationBuffer[idx].timestamp = millis();
        }
      }
    }
  }
}
