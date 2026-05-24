#include "ota_proto.h"
#include "uart_log.h"
#include "crc.h"
#include "stm32f4xx_hal.h"
#include "metadata.h"
#include "flash_io.h"

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


void ota_send_ack(void)
{
    uint8_t b = OTA_ACK;
    HAL_UART_Transmit(&huart2, &b, 1, 100);
}


void ota_send_nak(void)
{
    uint8_t b = OTA_NAK;
    HAL_UART_Transmit(&huart2, &b, 1, 100);
}


bool ota_receive_packet(ota_packet_t *out)
{
    uint8_t sof;
    do {
        if (HAL_UART_Receive(&huart2, &sof, 1, 5000) != HAL_OK) return false;
    } while (sof != OTA_SOF);

    if (HAL_UART_Receive(&huart2, &out->seq, 1, 1000) != HAL_OK) return false;
    if (HAL_UART_Receive(&huart2, (uint8_t *)&out->len, 2, 1000) != HAL_OK) return false;

    if (out->len > sizeof(out->data)) return false;

    if (HAL_UART_Receive(&huart2, out->data, out->len, 2000) != HAL_OK) return false;

    uint16_t crc_recu;
    if (HAL_UART_Receive(&huart2, (uint8_t *)&crc_recu, 2, 1000) != HAL_OK) return false;

    uint8_t crc_buf[3 + sizeof(out->data)];
    crc_buf[0] = out->seq;
    crc_buf[1] = (uint8_t)(out->len & 0xFF);
    crc_buf[2] = (uint8_t)((out->len >> 8) & 0xFF);
    for (uint16_t i = 0; i < out->len; i++) {
        crc_buf[3 + i] = out->data[i];
    }

    uint16_t crc_calc = crc16_ccitt(crc_buf, 3 + out->len);

    return (crc_calc == crc_recu);
}


void ota_run_session(void)
{
    LOG_BOOT("entering OTA session loop");

    bootloader_meta_t meta;
    meta_read(&meta);

    uint8_t  inactive_slot  = (meta.active_slot == SLOT_A) ? SLOT_B : SLOT_A;
    uint32_t inactive_addr  = (inactive_slot == SLOT_A) ? SLOT_A_ADDR : SLOT_B_ADDR;
    uint32_t inactive_size  = 0;
    uint32_t received_bytes = 0;
    bool     in_transfer    = false;

    ota_packet_t pkt;
    while (1) {
        if (!ota_receive_packet(&pkt)) {
            ota_send_nak();
            return;
        }

        if (pkt.len < 1) {
            ota_send_nak();
            continue;
        }

        uint8_t cmd = pkt.data[0];
        switch (cmd) {

            case CMD_PING:
                ota_send_ack();
                break;

            case CMD_BEGIN_OTA:
                if (pkt.len != 1 + 4) { ota_send_nak(); break; }
                inactive_size = *(uint32_t *)&pkt.data[1];
                if (inactive_slot == SLOT_A) {
                    for (uint32_t s = FLASH_SECTOR_SLOT_A_FIRST; s <= FLASH_SECTOR_SLOT_A_LAST; s++) {
                        flash_erase(s);
                    }
                } else {
                    flash_erase(FLASH_SECTOR_SLOT_B);
                }
                received_bytes = 0;
                in_transfer = true;
                ota_send_ack();
                LOG_BOOT("BEGIN_OTA size=%lu, erased slot %c",
                         (unsigned long)inactive_size,
                         inactive_slot == SLOT_A ? 'A' : 'B');
                break;

            case CMD_DATA:
                if (!in_transfer || pkt.len < 1 + 4) { ota_send_nak(); break; }
                {
                    uint32_t offset = *(uint32_t *)&pkt.data[1];
                    uint32_t dlen   = pkt.len - 1 - 4;
                    uint32_t plen   = (dlen + 3U) & ~3U;
                    HAL_StatusTypeDef st = flash_write(inactive_addr + offset,
                                                       &pkt.data[5], plen);
                    if (st != HAL_OK) { ota_send_nak(); break; }
                    received_bytes += dlen;
                    ota_send_ack();
                }
                break;

            case CMD_END_OTA:
                if (pkt.len != 1 + 4) { ota_send_nak(); break; }
                {
                    uint32_t expected_crc = *(uint32_t *)&pkt.data[1];
                    uint32_t computed = crc32_of_flash(inactive_addr, received_bytes);
                    LOG_BOOT("END_OTA expected=%08lX computed=%08lX size=%lu",
                             (unsigned long)expected_crc,
                             (unsigned long)computed,
                             (unsigned long)received_bytes);

                    if (computed != expected_crc) {
                        LOG_BOOT("CRC mismatch, OTA aborted");
                        ota_send_nak();
                        return;
                    }

                    bootloader_meta_t newm;
                    meta_read(&newm);
                    if (inactive_slot == SLOT_A) {
                        newm.slot_a_size      = received_bytes;
                        newm.slot_a_crc       = expected_crc;
                        newm.slot_a_validated = 0;
                        newm.slot_a_version  += 0x100;
                    } else {
                        newm.slot_b_size      = received_bytes;
                        newm.slot_b_crc       = expected_crc;
                        newm.slot_b_validated = 0;
                        newm.slot_b_version   = newm.slot_a_version + 0x100;
                    }
                    newm.active_slot = inactive_slot;
                    newm.boot_count  = 0;

                    HAL_StatusTypeDef st = meta_write(&newm);
                    if (st != HAL_OK) {
                        LOG_BOOT("meta_write failed (%d)", st);
                        ota_send_nak();
                        return;
                    }

                    LOG_BOOT("OTA complete, switching to slot %c, resetting in 100 ms",
                             inactive_slot == SLOT_A ? 'A' : 'B');
                    ota_send_ack();
                    HAL_Delay(100);
                    NVIC_SystemReset();
                }
                break;

            default:
                ota_send_nak();
                break;
        }
    }
}