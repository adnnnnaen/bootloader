#include "boot.h"
#include "stm32f4xx_hal.h"
#include "uart_log.h"



typedef void (*pFunction)(void); 

void jump_to_app(uint32_t app_addr){
    LOG_BOOT("Jumping to the app adresse at 0x%08lX .. ", (unsigned long)app_addr);
    HAL_Delay(50);


    __disable_irq();

    HAL_RCC_DeInit(); // revenir a HSI -> l'app config sa propre clk
    HAL_DeInit(); // desaciver tous les periph activer par app bootloader


    SCB->VTOR = app_addr;
    __DSB();
    __ISB();


    uint32_t app_sp = *(uint32_t*)app_addr;
    __set_MSP(app_sp);

    uint32_t app_reset = *(uint32_t*)(app_addr + 4);
    pFunction app_entry = (pFunction)app_reset;
    app_entry();

    while (1)
    {
        /* on n'arrivera jamais ici */
    }
    
}

