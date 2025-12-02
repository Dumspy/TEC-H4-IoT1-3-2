#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <time.h>

void hashMac(uint8_t* mac, char* output);
void formatTimestamp(time_t now, char* output, size_t size);

#endif
