#ifndef USB_PRINTER_H
#define USB_PRINTER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "usb/usb_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef enum {
    PRINTER_EVENT_CONNECTED,
    PRINTER_EVENT_DISCONNECTED
} printer_event_t;

typedef void (*printer_event_cb_t)(printer_event_t event);

typedef struct {
    usb_host_client_handle_t client_hdl;
    usb_device_handle_t dev_hdl;
    const usb_intf_desc_t *intf_desc;
    uint8_t bulk_out_ep_addr;
    uint16_t bulk_out_mps;
    uint8_t bulk_in_ep_addr;
    uint16_t bulk_in_mps;
    SemaphoreHandle_t transfer_sem;
    SemaphoreHandle_t tx_mutex;
    bool is_opened;
    bool busy;
    bool dev_close_pending;
} printer_dev_t;

extern printer_dev_t g_printer_dev;

esp_err_t printer_host_init(void);
void printer_register_event_cb(printer_event_cb_t cb);
void printer_host_task(void *arg);
esp_err_t printer_send_data(const uint8_t *data, size_t len);

#endif // USB_PRINTER_H
