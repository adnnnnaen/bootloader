#ifndef FLASH_HELPER_H
#define FLASH_HELPER_H

#include "stm32f4xx_hal.h"

/**
 * @brief  Helper qui écrit un buffer d'octets en flash, par mots de 4 octets.
 *
 * Préconditions :
 *   - secteur déjà effacé
 *   - flash unlock
 *   - addr alignée sur 4 octets
 *   - len multiple de 4
 *
 * @param  addr   Adresse de destination en flash (doit être alignée sur 4 octets)
 * @param  data   Pointeur vers le buffer source
 * @param  len    Longueur en octets (doit être multiple de 4)
 * @retval HAL_OK    si l'écriture a réussi
 * @retval HAL_ERROR si addr non alignée, len non multiple de 4, ou erreur HAL_FLASH_Program
 */
HAL_StatusTypeDef flash_write_words(uint32_t addr, const uint8_t *data, uint32_t len);

#endif /* FLASH_HELPER_H */