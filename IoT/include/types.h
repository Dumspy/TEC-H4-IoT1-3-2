#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include "config.h"

typedef struct {
  uint8_t mac[6];
  int rssi;
  unsigned long timestamp;
} SniffedDevice;

extern SniffedDevice devices[MAX_DEVICES];
extern int deviceCount;

#endif
