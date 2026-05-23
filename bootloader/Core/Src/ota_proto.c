#include "ota_proto.h"
#include "uart_log.h"
#include "crc.h"
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart2;

bool ota_check_enter_mode(uint32_t timeout_ms)
{
    uint8_t c = 0;
    if (HAL_UART_Receive(&huart2, &c, 1, timeout_ms) == HAL_OK && c == 'U') {
        LOG_BOOT("OTA mode entered ('U' received)");
        return true;
    }
    return false;
}

void ota_send_ack(void) {
    uint8_t b = OTA_ACK;
    HAL_UART_Transmit(&huart2, &b, 1, 100);
}
void ota_send_nak(void) {
    uint8_t b = OTA_NAK;
    HAL_UART_Transmit(&huart2, &b, 1, 100);
}

bool ota_receive_packet(ota_packet_t *out) {
    /* À implémenter */
    (void)out;
    return false;
}

void ota_run_session(void) {
    LOG_BOOT("OTA mode running ... ");
    while(1){}
}