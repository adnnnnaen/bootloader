#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/* CRC32 IEEE 802.3 (compatible zlib.crc32 / Python).
 * Polynôme 0xEDB88320, init 0xFFFFFFFF, XOR final 0xFFFFFFFF. */
uint32_t crc32_calc(const uint8_t *data, uint32_t len);

uint32_t crc32_of_flash(uint32_t addr, uint32_t size);

uint16_t crc16_ccitt(const uint8_t *data, uint32_t len);

#endif