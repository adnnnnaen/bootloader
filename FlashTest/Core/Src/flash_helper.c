#include "flash_helper.h"



HAL_StatusTypeDef flash_write_words(uint32_t addr, const uint8_t *data, uint32_t len){

    if ((addr & 0x3) != 0 || (len & 0x3) != 0 ) return HAL_ERROR;

    for (int i = 0 ; i < len -1 ; i = i+4){
            uint32_t word  = (uint32_t)data[i] |
                            (uint32_t)data[i+1] << 8 |
                            (uint32_t)data[i+2] << 16 |
                            (uint32_t)data[i+3] << 24;

        HAL_StatusTypeDef st =  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word);
        if(st != HAL_OK){
            return st ;
        }
    }
    return HAL_OK;
}
    