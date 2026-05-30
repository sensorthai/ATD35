/**
 * @file card_reader.h
 * @brief Magnetic Card Reader (Driver's License) via UART
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UART Pin Configuration */
#define CARD_READER_TXD   (UART_PIN_NO_CHANGE)
#define CARD_READER_RXD   (42)
#define CARD_READER_RTS   (UART_PIN_NO_CHANGE)
#define CARD_READER_CTS   (UART_PIN_NO_CHANGE)
#define CARD_UART_PORT_NUM (UART_NUM_2)
#define CARD_UART_BAUD     (9600)
#define CARD_BUF_SIZE      (2048)

/* Parsed license data */
typedef struct {
    char card_id[16];       // 13-digit national ID
    char firstname[64];
    char lastname[64];
    char prefix[32];
    char expire_date[16];
    char birth_date[16];
} license_data;

/**
 * @brief Initialize UART for card reader
 */
void card_reader_init(void);

/**
 * @brief Start the card reader background task
 *        When a card is swiped, calls the callback with parsed data.
 */
void card_reader_start(void);

/**
 * @brief Register callback for when a card is successfully read
 */
typedef void (*card_read_cb_t)(const license_data *lic);
void card_reader_register_cb(card_read_cb_t cb);

#ifdef __cplusplus
}
#endif
