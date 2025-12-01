#ifndef ESPNOW_COMM_H
#define ESPNOW_COMM_H

#include <esp_now.h>
#include <stdint.h>

void initESPNow();
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void processMasterSniffedDevices();

#endif
