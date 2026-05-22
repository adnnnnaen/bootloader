#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include"stm32f4xx_hal.h"


/* Marqueurs et valeurs magiques */
#define META_MAGIC          0xB00710ADUL    /* signature "BOOTLOAD" */
#define META_VERSION        1

/* Slots */
#define SLOT_A              0
#define SLOT_B              1

/* Adresses mémoire */
#define META_ADDR           0x08004000UL    /* secteur 1 */
#define SLOT_A_ADDR         0x08008000UL    /* secteurs 2+3+4 */
#define SLOT_A_SIZE         (96UL * 1024UL)
#define SLOT_B_ADDR         0x08020000UL    /* secteur 5 */
#define SLOT_B_SIZE         (128UL * 1024UL)

/* Numéros de secteur */
#define FLASH_SECTOR_META          1
#define FLASH_SECTOR_SLOT_A_FIRST  2
#define FLASH_SECTOR_SLOT_A_LAST   4
#define FLASH_SECTOR_SLOT_B        5

/* Structure des métadonnées (128 octets) */
typedef struct __attribute__((packed)) { /*Pour bloquer la padding */
    uint32_t magic;
    uint16_t version_meta;
    uint8_t  active_slot;
    uint8_t  boot_count;

    uint32_t slot_a_version;
    uint32_t slot_a_size;
    uint32_t slot_a_crc;
    uint8_t  slot_a_validated;
    uint8_t  _pad_a[3];

    uint32_t slot_b_version;
    uint32_t slot_b_size;
    uint32_t slot_b_crc;
    uint8_t  slot_b_validated;
    uint8_t  _pad_b[3];

    uint8_t  _reserved[84];
    uint32_t meta_crc;
} bootloader_meta_t;

static_assert(sizeof(bootloader_meta_t) == 128, "bootloader_meta_t must be 128 bytes");


void               meta_read (bootloader_meta_t *out);
HAL_StatusTypeDef  meta_write(const bootloader_meta_t *in);
bool               meta_validate(const bootloader_meta_t *m);

#endif /* METADATA_H */