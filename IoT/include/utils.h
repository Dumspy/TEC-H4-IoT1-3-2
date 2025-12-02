#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void hashMac(uint8_t* mac, char* output);
int findDevice(uint8_t* mac);

#endif
