#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H

#include <esp_wifi.h>

void initWiFiSniffer();
void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

#endif
