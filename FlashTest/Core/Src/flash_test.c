#include "flash_test.h"
#include "uart_log.h"
#include "flash_helper.h"
#include "stm32f4xx_hal.h"


void flash_test(void){
    /* On va écrire 16 octets à l'adresse 0x08060000 (dans le secteur 7) */
    const uint8_t test_data[16] = "ADNANEISTHEBESTA"; // Adnane is the best of all time xD
    uint32_t target = 0x08060000;

    __disable_irq();   // Désactivation des interruptions pendant écriture/effacement pour éviter HARD_FAULT
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = FLASH_SECTOR_7,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3
    };
    uint32_t err = 0;
    HAL_StatusTypeDef st;

    st = HAL_FLASHEx_Erase(&erase, &err);
    if (st != HAL_OK) {
        LOG_BOOT("Erase failed: st=%d, err=0x%lX", st, (unsigned long)err);
    }

    st = flash_write_words(target, test_data, 16);
    if (st != HAL_OK) {
        LOG_BOOT("Write failed: st=%d", st);
    }

    HAL_FLASH_Lock();
    __enable_irq();

    const uint8_t *p = (const uint8_t *)target;
    for (int i = 0; i < 16; i++) {
        LOG_BOOT("Data at address %p is '%c' (0x%02X)", (void *)(p + i), p[i], p[i]);
    }
}