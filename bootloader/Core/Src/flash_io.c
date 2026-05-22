#include "flash_io.h"


HAL_StatusTypeDef flash_erase(uint8_t sector_number){
    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = sector_number,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3
    };

    __disable_irq();
    HAL_FLASH_Unlock();

    uint32_t err =  0;
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&erase,&err);
    if (st != HAL_OK){
        return HAL_ERROR;
    }

    HAL_FLASH_Lock();
    __enable_irq();
    
    return HAL_OK;

}


HAL_StatusTypeDef flash_write(uint32_t dest_addr, const uint8_t *data, uint32_t length){
    /* adresse de destination doit etre alignee sur 4 octets => deux derniers bit multiple de 4 */
    HAL_StatusTypeDef st = HAL_OK;        

    if ((dest_addr & 0x3U) != 0 || (length & 0x3U) != 0) return HAL_ERROR;
    uint32_t word;

    __disable_irq();
    HAL_FLASH_Unlock();

    for (int i = 0; i < length ; i = i+4){
        word =  (uint32_t)data[i] |
                (uint32_t)data[i+1] << 8 |
                (uint32_t)data[i+2] << 16 |
                (uint32_t)data[i+3] << 24 ;

    st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, dest_addr+i, word);

        if (st != HAL_OK){
             break; 
        }
    }
    HAL_FLASH_Lock();
    __enable_irq();
    return st;

}