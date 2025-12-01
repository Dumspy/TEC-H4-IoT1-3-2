#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include "mbedtls/md.h"
#include "config.h"

typedef struct {
  uint8_t mac[6];
  int rssi;
  unsigned long timestamp;
} SniffedDevice;

typedef struct {
  uint8_t mac[6];
  int rssi;
  float x;
  float y;
} SlaveReport;

SniffedDevice devices[MAX_DEVICES];
int deviceCount = 0;

typedef struct {
  uint8_t mac[6];
  int rssi[3];
  float positions[3][2];
  int reportCount;
  unsigned long timestamp;
} TriangulationData;

TriangulationData triangulationBuffer[MAX_DEVICES];
int triangulationCount = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);
unsigned long lastMqttPublish = 0;

uint8_t masterAddress[] = MASTER_MAC;

void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void processDevices();
void calculatePosition(uint8_t* mac);
float rssiToDistance(int rssi);
void hashMac(uint8_t* mac, char* output);
void connectWiFi();
void connectMQTT();
void publishPosition(const char* deviceId, float x, float y);
int findDevice(uint8_t* mac);
int findTriangulationData(uint8_t* mac);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  if (IS_MASTER) {
    Serial.println("WiFi Sniffer MASTER Node");
  } else {
    Serial.println("WiFi Sniffer SLAVE Node");
  }
  Serial.println("=================================\n");

  if (IS_MASTER) {
    connectWiFi();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    connectMQTT();
  } else {
    Serial.print("Master MAC configured as: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", masterAddress[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
  }
  
  WiFi.mode(WIFI_MODE_STA);
  
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
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  
  if (!IS_MASTER) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, masterAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
  
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&snifferCallback);
  
  Serial.println("Promiscuous mode enabled - Sniffing started");
  if (!IS_MASTER) {
    Serial.print("Slave position: X=");
    Serial.print(DEVICE_X);
    Serial.print(", Y=");
    Serial.println(DEVICE_Y);
  }
}

void loop() {
  if (IS_MASTER) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();
    
    if (millis() - lastMqttPublish > MQTT_PUBLISH_INTERVAL) {
      processDevices();
      lastMqttPublish = millis();
    }
  } else {
    static unsigned long lastReport = 0;
    if (millis() - lastReport > SLAVE_REPORT_INTERVAL) {
      for (int i = 0; i < deviceCount; i++) {
        if (millis() - devices[i].timestamp < RSSI_TIMEOUT) {
          SlaveReport report;
          memcpy(report.mac, devices[i].mac, 6);
          report.rssi = devices[i].rssi;
          report.x = DEVICE_X;
          report.y = DEVICE_Y;
          
          esp_now_send(masterAddress, (uint8_t*)&report, sizeof(report));
        }
      }
      lastReport = millis();
    }
  }
  
  delay(10);
}

void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
  
  uint8_t* mac = pkt->payload + 10;
  
  if (mac[0] & 0x01) return;
  
  int rssi = ctrl.rssi;
  
  int idx = findDevice(mac);
  if (idx == -1 && deviceCount < MAX_DEVICES) {
    idx = deviceCount++;
  }
  
  if (idx != -1) {
    memcpy(devices[idx].mac, mac, 6);
    devices[idx].rssi = rssi;
    devices[idx].timestamp = millis();
  }
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (!IS_MASTER) return;
  
  SlaveReport* report = (SlaveReport*)data;
  
  int idx = findTriangulationData(report->mac);
  if (idx == -1 && triangulationCount < MAX_DEVICES) {
    idx = triangulationCount++;
    memcpy(triangulationBuffer[idx].mac, report->mac, 6);
    triangulationBuffer[idx].reportCount = 0;
  }
  
  if (idx != -1 && triangulationBuffer[idx].reportCount < 3) {
    int pos = triangulationBuffer[idx].reportCount;
    triangulationBuffer[idx].rssi[pos] = report->rssi;
    triangulationBuffer[idx].positions[pos][0] = report->x;
    triangulationBuffer[idx].positions[pos][1] = report->y;
    triangulationBuffer[idx].reportCount++;
    triangulationBuffer[idx].timestamp = millis();
  }
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
}

void processDevices() {
  for (int i = 0; i < triangulationCount; i++) {
    if (triangulationBuffer[i].reportCount >= 2 && 
        millis() - triangulationBuffer[i].timestamp < RSSI_TIMEOUT) {
      calculatePosition(triangulationBuffer[i].mac);
      triangulationBuffer[i].reportCount = 0;
    }
  }
}

void calculatePosition(uint8_t* mac) {
  int idx = findTriangulationData(mac);
  if (idx == -1) return;
  
  TriangulationData* data = &triangulationBuffer[idx];
  
  if (data->reportCount < 2) return;
  
  float distances[3];
  for (int i = 0; i < data->reportCount; i++) {
    distances[i] = rssiToDistance(data->rssi[i]);
  }
  
  float x = 0, y = 0;
  
  if (data->reportCount == 2) {
    x = (data->positions[0][0] + data->positions[1][0]) / 2.0;
    y = (data->positions[0][1] + data->positions[1][1]) / 2.0;
  } else {
    float x1 = data->positions[0][0], y1 = data->positions[0][1], r1 = distances[0];
    float x2 = data->positions[1][0], y2 = data->positions[1][1], r2 = distances[1];
    float x3 = data->positions[2][0], y3 = data->positions[2][1], r3 = distances[2];
    
    float A = 2*x2 - 2*x1;
    float B = 2*y2 - 2*y1;
    float C = r1*r1 - r2*r2 - x1*x1 + x2*x2 - y1*y1 + y2*y2;
    float D = 2*x3 - 2*x2;
    float E = 2*y3 - 2*y2;
    float F = r2*r2 - r3*r3 - x2*x2 + x3*x3 - y2*y2 + y3*y3;
    
    if (A*E - B*D != 0) {
      x = (C*E - F*B) / (E*A - B*D);
      y = (C*D - A*F) / (B*D - A*E);
    } else {
      x = (x1 + x2 + x3) / 3.0;
      y = (y1 + y2 + y3) / 3.0;
    }
  }
  
  char hashedMac[65];
  hashMac(mac, hashedMac);
  
  publishPosition(hashedMac, x, y);
  
  Serial.print("Device: ");
  Serial.print(hashedMac);
  Serial.print(" Position: (");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.println(")");
}

float rssiToDistance(int rssi) {
  int txPower = -59;
  
  if (rssi == 0) {
    return -1.0;
  }
  
  float ratio = rssi * 1.0 / txPower;
  if (ratio < 1.0) {
    return pow(ratio, 10);
  } else {
    float distance = (0.89976) * pow(ratio, 7.7095) + 0.111;
    return distance;
  }
}

void hashMac(uint8_t* mac, char* output) {
  byte shaResult[32];
  
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char*)mac, 6);
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);
  
  for (int i = 0; i < 16; i++) {
    sprintf(output + (i * 2), "%02x", shaResult[i]);
  }
  output[32] = 0;
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
  
  mqttClient.publish(MQTT_TOPIC, payload);
}

int findDevice(uint8_t* mac) {
  for (int i = 0; i < deviceCount; i++) {
    if (memcmp(devices[i].mac, mac, 6) == 0) {
      return i;
    }
  }
  return -1;
}

int findTriangulationData(uint8_t* mac) {
  for (int i = 0; i < triangulationCount; i++) {
    if (memcmp(triangulationBuffer[i].mac, mac, 6) == 0) {
      return i;
    }
  }
  return -1;
}
