#ifndef OTA_PROTO_H
#define OTA_PROTO_H

#include <stdint.h>
#include <stdbool.h>

/* Constantes du proto UART (suit notre format de trame a mettre dans le README.md)*/
#define OTA_SOF              0xAAU
#define OTA_ACK              0x06U
#define OTA_NAK              0x15U
#define OTA_CHUNK_SIZE       256U

/* Codes de commande (1er octet de DATA) */
#define CMD_PING             0x01U
#define CMD_BEGIN_OTA        0x03U
#define CMD_DATA             0x04U
#define CMD_END_OTA          0x05U

/* Représentation décodée d'un paquet reçu */
typedef struct {
    uint8_t  seq;
    uint16_t len;
    uint8_t  data[OTA_CHUNK_SIZE + 16];   /* payload + marge */
} ota_packet_t;

/* Tente de recevoir un paquet sur USART2.
 * Retourne true si paquet valide (CRC16 OK), false sinon (timeout, CRC KO).
 * Le bootloader appelle systématiquement send_ack/send_nak après. */
bool ota_receive_packet(ota_packet_t *out);

/* Envoyer ACK / NAK */
void ota_send_ack(void);
void ota_send_nak(void);

/* Entrée du mode OTA */
bool ota_check_enter_mode(uint32_t timeout_ms);

/* Boucle principale du mode OTA */
void ota_run_session(void);

#endif