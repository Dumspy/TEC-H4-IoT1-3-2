#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
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

typedef struct {
  uint8_t mac[6];
  int rssi[3];
  float positions[3][2];
  int reportCount;
  unsigned long timestamp;
} TriangulationData;

extern SniffedDevice devices[MAX_DEVICES];
extern int deviceCount;

extern TriangulationData triangulationBuffer[MAX_DEVICES];
extern int triangulationCount;

#endif
