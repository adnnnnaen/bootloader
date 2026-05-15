/*
 * uart_log.h — Module de logging UART pour STM32 (FreeRTOS-safe).
 *
 * Deux familles de macros disponibles :
 *
 *   1. Par tâche : LOG_BLINK("msg"), LOG_SENSOR("msg"), etc.
 *      → Affiche : "[BLINK]  msg\r\n"
 *      Pratique pour identifier d'où vient un log sur le terminal.
 *
 *   2. Par niveau : LOG_INFO("msg"), LOG_WARN("msg"), LOG_ERROR("msg")
 *      → Affiche : "[INFO]  msg\r\n"
 *      Pratique pour filtrer par sévérité.
 *
 * Les deux familles peuvent être mélangées librement.
 *
 * Sortie : USART2 (115200 8N1, VCP ST-Link).
 *
 * Limitations :
 *   - NE PAS appeler depuis une ISR.
 *   - Buffer interne de 128 octets (messages plus longs tronqués).
 */

#ifndef UART_LOG_H
#define UART_LOG_H

#define BUFFER_SIZE 128

void uart_log_init(void);
void log_uart_simple(const char *tag, const char *fmt, ...);

/* --- Tags par niveau de sévérité --- */
#define LOG_INFO(fmt, ...)   log_uart_simple("[INFO]   ", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   log_uart_simple("[WARN]   ", fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)  log_uart_simple("[ERROR]  ", fmt, ##__VA_ARGS__)

/* --- Tags par tâche --- */
#define LOG_BLINK(fmt, ...)   log_uart_simple("[BLINK]  ",  fmt, ##__VA_ARGS__)
#define LOG_SENSOR(fmt, ...)  log_uart_simple("[SENSOR] ",  fmt, ##__VA_ARGS__)
#define LOG_UART_RX(fmt, ...) log_uart_simple("[UART_RX]", fmt, ##__VA_ARGS__)
#define LOG_PROTO(fmt, ...)   log_uart_simple("[PROTO]  ",  fmt, ##__VA_ARGS__)
#define LOG_BOOT(fmt, ...)    log_uart_simple("[BOOT]   ",  fmt, ##__VA_ARGS__)

#endif /* UART_LOG_H */