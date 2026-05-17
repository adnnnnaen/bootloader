#ifndef FREERTOS_H
#define FREERTOS_H

#include "cmsis_os.h"

/* Handles exposés  */
extern osThreadId_t BlinkHandle;
extern osThreadId_t ButtonHandle;
extern osThreadId_t UartButtonHandle;
extern osSemaphoreId_t ButtonSem;
extern osMessageQueueId_t UartQueue;

/* Fonction d'init appelée depuis main */
void MX_FREERTOS_Init(void);

/* Fonctions des tâches  */
void StartBlinkTask(void *argument);
void StartButtonTask(void *argument);
void StartUartBlinkTask(void *argument);

#endif /* FREERTOS_H */