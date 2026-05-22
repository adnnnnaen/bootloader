#ifndef FLASH_IO_H
#define FLASH_IO_H

#include "stm32f4xx_hal.h"



HAL_StatusTypeDef flash_erase(uint8_t sector_number);
HAL_StatusTypeDef flash_write(uint32_t dest_addr,const uint8_t *data, uint32_t length);


#endif /* FLASH_IO_H */