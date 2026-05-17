#include "app_freertos.h"
#include "main.h"
#include "uart_log.h"

/* Handles définis ici */
osThreadId_t BlinkHandle;
osThreadId_t ButtonHandle;
osSemaphoreId_t ButtonSem;

/* Attributs des tâches */
const osThreadAttr_t Blink_attributes = {
    .name = "Blink",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t Button_attributes = {
    .name = "Button",
    .stack_size = 128 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};

/**
 * @brief Initialise toutes les tâches, sémaphores, queues FreeRTOS.
 *        Appelé depuis main() avant osKernelStart().
 */
void MX_FREERTOS_Init(void)
{
    /* Sémaphores */
    ButtonSem = osSemaphoreNew(1, 0, NULL);
    
    /* Tâches */
    BlinkHandle  = osThreadNew(StartBlinkTask,  NULL, &Blink_attributes);
    ButtonHandle = osThreadNew(StartButtonTask, NULL, &Button_attributes);
}

/* ===== Implémentations des tâches ===== */

void StartBlinkTask(void *argument)
{
    for (;;) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        LOG_BLINK("LED BLINKS");
        osDelay(500);
    }
}

void StartButtonTask(void *argument)
{
    for (;;) {
        osSemaphoreAcquire(ButtonSem, osWaitForever);
        LOG_INFO("Bouton pressé !");
    }
}