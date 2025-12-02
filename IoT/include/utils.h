#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "types.h"

void hashMac(uint8_t* mac, char* output);
int findDevice(uint8_t* mac);
int findTriangulationData(uint8_t* mac);
int findPositionSlot(int triIndex, float x, float y);

#endif
