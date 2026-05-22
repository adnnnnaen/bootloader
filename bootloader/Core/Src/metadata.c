#include "metadata.h"
#include "crc.h"
#include "flash_io.h"
#include <string.h>


void meta_read(bootloader_meta_t *out)
{
    memcpy(out, (const void *)META_ADDR, sizeof(*out));
}


HAL_StatusTypeDef meta_write(const bootloader_meta_t *in){
    bootloader_meta_t local = *in;
    HAL_StatusTypeDef st;

    /*  CRC en RAM (avant tout effacement) */
    local.meta_crc = crc32_calc((const uint8_t*)&local, sizeof(local) - 4);

    
    st = flash_erase(FLASH_SECTOR_META);
    if (st != HAL_OK) return st;

    /*  Écrire la nouvelle metadata dans la flash*/
    st = flash_write(META_ADDR, (const uint8_t*)&local, sizeof(local));
    if (st != HAL_OK) return st;

    return HAL_OK;
}

bool meta_validate(const bootloader_meta_t *m)
{
    if (m->magic != META_MAGIC) return false;
    if (m->version_meta != META_VERSION) return false;

    /* Recalcule le CRC stocké pour vérifier l'intégrité */
    uint32_t expected = crc32_calc((const uint8_t *)m,
                                   sizeof(*m) - sizeof(uint32_t));
    return (expected == m->meta_crc);
}