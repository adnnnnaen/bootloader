#ifndef BOOT_H
#define BOOT_H

#include <stdint.h>

#define APP_SLOT_A_ADDR 0x08008000U

/* Saute vers l'application située à app_addr */
void jump_to_app(uint32_t app_addr);

#endif /* BOOT_H */