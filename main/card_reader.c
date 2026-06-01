/**
 * @file card_reader.c
 * @brief Magnetic Card Reader — UART RX + License Parsing
 */
#include "card_reader.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "card_reader";

static card_read_cb_t s_card_cb = NULL;
static uint8_t s_rx_buf[CARD_BUF_SIZE];

/* ========================================================================== */
/*  Helpers                                                                    */
/* ========================================================================== */
static void hex_to_ascii(const uint8_t *data, int len, char *out, int out_size)
{
    int j = 0;
    for (int i = 0; i < len && j < out_size - 1; i++) {
        if (data[i] >= 0x20 && data[i] <= 0x7E) {
            out[j++] = (char)data[i];
        }
    }
    out[j] = '\0';
}

static bool is_license_start(const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
        if (data[i] == 0x25) { // '%' = start sentinel of magnetic stripe
            return true;
        }
    }
    return false;
}

/* ========================================================================== */
/*  License Parser                                                             */
/* ========================================================================== */
static license_data *parse_license(uint8_t *data, int len)
{
    static license_data lic;
    memset(&lic, 0, sizeof(license_data));

    static char buffer[2048];
    hex_to_ascii(data, len, buffer, sizeof(buffer));

    ESP_LOGI(TAG, "Raw: %s", buffer);

    char *saveptr = NULL;
    char *track1 = strtok_r(buffer, "?", &saveptr);
    char *track2 = strtok_r(NULL, "?", &saveptr);

    if (track1) {
        char t1_token[128];
        strncpy(t1_token, track1, sizeof(t1_token) - 1);
        t1_token[sizeof(t1_token) - 1] = '\0';

        char *save1 = NULL;
        char *lastname  = strtok_r(t1_token, "% ^$", &save1);
        char *firstname = strtok_r(NULL, "$", &save1);
        char *prefix    = strtok_r(NULL, ".^", &save1);

        if (firstname) strncpy(lic.firstname, firstname, sizeof(lic.firstname) - 1);
        if (lastname)  strncpy(lic.lastname, lastname, sizeof(lic.lastname) - 1);
        if (prefix)    strncpy(lic.prefix, prefix, sizeof(lic.prefix) - 1);

        ESP_LOGI(TAG, "Name: %s %s %s", lic.prefix, lic.firstname, lic.lastname);
    }

    if (track2) {
        char t2_token[128];
        strncpy(t2_token, track2, sizeof(t2_token) - 1);
        t2_token[sizeof(t2_token) - 1] = '\0';

        char *save2 = NULL;
        char *type_card_id = strtok_r(t2_token, ";=", &save2);

        if (type_card_id != NULL && strlen(type_card_id) == 19) {
            char card_id[14];
            strncpy(card_id, type_card_id + 6, 13);
            card_id[13] = '\0';

            ESP_LOGI(TAG, "CARD ID: %s (len=%d)", card_id, strlen(card_id));

            if (strlen(card_id) == 13)
                strcpy(lic.card_id, card_id);
        } else {
            ESP_LOGW(TAG, "Could not parse card ID from track2");
        }

        char *dates = strtok_r(NULL, "=", &save2);
        if (dates != NULL && strlen(dates) == 12) {
            snprintf(lic.expire_date, sizeof(lic.expire_date), "%c%c/%c%c",
                     dates[2], dates[3], dates[0], dates[1]);
            snprintf(lic.birth_date, sizeof(lic.birth_date), "%c%c%c%c/%c%c/%c%c",
                     dates[4], dates[5], dates[6], dates[7],
                     dates[8], dates[9], dates[10], dates[11]);
        }
    }

    return &lic;
}

/* ========================================================================== */
/*  UART Card Reader Task                                                      */
/* ========================================================================== */
static void card_reader_task(void *arg)
{
    ESP_LOGI(TAG, "Card reader task started on UART%d (RXD=%d)", CARD_UART_PORT_NUM, CARD_READER_RXD);

    bool collecting = false;
    int total_len = 0;
    static uint8_t packet_buf[CARD_BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(CARD_UART_PORT_NUM, s_rx_buf, CARD_BUF_SIZE - 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            /* Debug: print raw hex bytes */
            ESP_LOGI(TAG, "RX %d bytes:", len);
            ESP_LOG_BUFFER_HEX(TAG, s_rx_buf, len > 64 ? 64 : len);

            /* Check if this is the start of a new card swipe */
            if (!collecting && is_license_start(s_rx_buf, len)) {
                collecting = true;
                total_len = 0;
            }

            if (collecting) {
                /* Append to packet buffer */
                int copy_len = len;
                if (total_len + copy_len > CARD_BUF_SIZE - 1) {
                    copy_len = CARD_BUF_SIZE - 1 - total_len;
                }
                memcpy(packet_buf + total_len, s_rx_buf, copy_len);
                total_len += copy_len;
            }
        } else if (collecting && total_len > 0) {
            /* Timeout with data collected = end of card swipe */
            ESP_LOGI(TAG, "Card data complete: %d bytes", total_len);

            license_data *lic = parse_license(packet_buf, total_len);

            if (lic && strlen(lic->card_id) == 13 && s_card_cb) {
                ESP_LOGI(TAG, "Valid license: ID=%s Name=%s %s",
                         lic->card_id, lic->firstname, lic->lastname);
                s_card_cb(lic);
            } else if (lic && strlen(lic->card_id) != 13) {
                ESP_LOGW(TAG, "Invalid card ID length: '%s'", lic->card_id);
            }

            collecting = false;
            total_len = 0;
        }
    }
}

/* ========================================================================== */
/*  Public API                                                                 */
/* ========================================================================== */
void card_reader_register_cb(card_read_cb_t cb)
{
    s_card_cb = cb;
}

void card_reader_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = CARD_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(CARD_UART_PORT_NUM, CARD_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CARD_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(CARD_UART_PORT_NUM, CARD_READER_TXD, CARD_READER_RXD,
                                  CARD_READER_RTS, CARD_READER_CTS));

    ESP_LOGI(TAG, "Card reader UART initialized (RXD=GPIO%d, baud=%d)", CARD_READER_RXD, CARD_UART_BAUD);

    /* Invert RX signal for direct TTL connection (no MAX232) */
    uart_set_line_inverse(CARD_UART_PORT_NUM, UART_SIGNAL_RXD_INV);
    ESP_LOGI(TAG, "RX signal inverted for TTL direct connection");
}

void card_reader_start(void)
{
    xTaskCreate(card_reader_task, "card_reader", 4096, NULL, 5, NULL);
}
