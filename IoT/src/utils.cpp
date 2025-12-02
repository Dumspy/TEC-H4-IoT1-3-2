#include <Arduino.h>
#include "utils.h"
#include "types.h"
#include "mbedtls/md.h"
#include <stdio.h>

void hashMac(uint8_t* mac, char* output) {
  uint8_t shaResult[32];
  
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

int findDevice(uint8_t* mac) {
  for (int i = 0; i < deviceCount; i++) {
    if (memcmp(devices[i].mac, mac, 6) == 0) {
      return i;
    }
  }
  return -1;
}
