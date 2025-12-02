#ifndef TRIANGULATION_H
#define TRIANGULATION_H

#include <stdint.h>

float rssiToDistance(int rssi);
void calculatePosition(uint8_t* mac);
void processDevices();

#endif
