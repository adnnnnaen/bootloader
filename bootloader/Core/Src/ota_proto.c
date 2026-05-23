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
    uint8_t sof; 
    do {
       if (HAL_UART_Receive(&huart2, &sof, 1, 5000) != HAL_OK){
            return false;
        }
    }while(sof != OTA_SOF);

    if (HAL_UART_Receive(&huart2, &out->seq, 1, 1000) != HAL_OK){
        return false;
    }
    if (HAL_UART_Receive(&huart2, (uint8_t*)&out->len, 2, 1000) != HAL_OK){
        return false;
    }

    if (out->len > sizeof(out->data)) return false;

    if (HAL_UART_Receive(&huart2, out->data, out->len, 2000) != HAL_OK){
        return false;
    }

    uint16_t crc_recu;
    if (HAL_UART_Receive(&huart2, (uint8_t*)&crc_recu, 2, 1000) != HAL_OK) return false;

    /* Recalculer le CRC16 sur SEQ + LEN + DATA */
    uint8_t crc_buf[3 + sizeof(out->data)];
    crc_buf[0] = out->seq;
    crc_buf[1] = (uint8_t)(out->len & 0xFF);          // LEN low
    crc_buf[2] = (uint8_t)((out->len >> 8) & 0xFF);   // LEN high
    for (uint16_t i = 0; i < out->len; i++) {
            crc_buf[3 + i] = out->data[i];
    }

    uint16_t crc_calc = crc16_calc(crc_buf, 3 + out->len);

    return (crc_calc == crc_recu);
    
}

void ota_run_session(void) {
    LOG_BOOT("OTA mode running ... ");
    while(1){}
}