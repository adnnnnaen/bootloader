#include "crc.h"

uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1U) crc = (crc >> 1) ^ 0xEDB88320U;
            else          crc >>= 1;
        }
    }
    return ~crc;
}


uint32_t crc32_of_flash(uint32_t addr, uint32_t size)
{
    return crc32_calc((const uint8_t *)addr, size);
}