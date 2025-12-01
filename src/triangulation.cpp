#include <Arduino.h>
#include <math.h>
#include "triangulation.h"
#include "types.h"
#include "utils.h"
#include "config.h"

extern void publishPosition(const char* deviceId, float x, float y);

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
  
  #if IS_MASTER && DEBUG_LEVEL >= 1
  Serial.print("[RESULT] ");
  Serial.print(hashedMac);
  Serial.print(" @ (");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.println(")");
  #endif
}

void processDevices() {
  for (int i = 0; i < triangulationCount; i++) {
    if (triangulationBuffer[i].reportCount == 3 && 
        millis() - triangulationBuffer[i].timestamp < RSSI_TIMEOUT) {
      
      calculatePosition(triangulationBuffer[i].mac);
      triangulationBuffer[i].reportCount = 0;
    }
  }
}
