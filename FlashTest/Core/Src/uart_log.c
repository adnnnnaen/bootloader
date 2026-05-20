/*
 * uart_log.c — Implémentation du module de logging UART.
 *
 * Flux d'une transmission :
 *   1. Acquisition du mutex (bloquant si une autre tâche logue déjà).
 *   2. Copie du tag dans le buffer.
 *   3. Formatage du message utilisateur via vsnprintf à la suite du tag.
 *   4. Ajout de "\r\n" à la fin.
 *   5. Transmission synchrone via HAL_UART_Transmit sur huart2.
 *   6. Libération du mutex.
 *
 * Mode dégradé : si uart_log_init() n'a pas encore été appelée (par exemple
 * lors de logs très tôt dans main(), avant osKernelStart), la fonction
 * fonctionne sans mutex. C'est sûr puisqu'à ce stade une seule tâche
 * d'exécution existe.
 */

#include "uart_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "main.h"


extern UART_HandleTypeDef huart2;

static char buf[BUFFER_SIZE];


void log_uart_simple(const char *tag, const char *fmt, ...)
{

    /* --- Copie du tag dans le buffer --- */
    size_t pos = 0;
    while (tag[pos] != 0 && pos < BUFFER_SIZE - 3) {
        buf[pos] = tag[pos];
        pos++;
    }

    /* --- Formatage du message utilisateur après le tag --- */
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(&buf[pos], BUFFER_SIZE - 2 - pos, fmt, args);
    va_end(args);

    if (n < 0) {
        const char *marker = "<vsnprintf err>";
        size_t mlen = strlen(marker);
        for (size_t i = 0; i < mlen && pos + i < BUFFER_SIZE - 2; i++) {
            buf[pos + i] = marker[i];
        }
        n = (int)mlen;
    }
    else if (n >= (int)(BUFFER_SIZE - pos - 2)) {
        n = (int)(BUFFER_SIZE - pos - 2 - 1);
    }

    pos += n;
    buf[pos++] = '\r';
    buf[pos++] = '\n';

    HAL_UART_Transmit(&huart2, (uint8_t *)buf, pos, HAL_MAX_DELAY);
}
