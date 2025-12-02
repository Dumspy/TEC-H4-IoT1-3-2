#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "wifi_sniffer.h"
#include "utils.h"
#include "config.h"
#include "mqtt_client.h"

void initWiFiSniffer() {
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
}

void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
  
  uint8_t* mac = pkt->payload + 10;
  
  if (mac[0] & 0x01) return;
  
  int rssi = ctrl.rssi;
  
  char hashedMac[65];
  hashMac(mac, hashedMac);
  
  publishSniffedDevice(hashedMac, rssi, DEVICE_X, DEVICE_Y);
  
  #if DEBUG_LEVEL >= 2
  Serial.print("[SNIFFER] Device | RSSI: ");
  Serial.print(rssi);
  Serial.println(" dBm");
  #endif
}
