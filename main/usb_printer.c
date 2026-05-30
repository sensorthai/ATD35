#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "usb/usb_helpers.h"
#include "usb_printer.h"

static const char *TAG = "usb_printer";

printer_dev_t g_printer_dev = {0};
static printer_event_cb_t g_event_cb = NULL;

static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg);
static void transfer_cb(usb_transfer_t *transfer);

static SemaphoreHandle_t s_init_sem = NULL;
static esp_err_t s_init_ret = ESP_FAIL;

/* -------------------------------------------------------------------------- */
/*  Background task – handles USB events (Pinned to Core 1)
 * -------------------------------------------------------------------------- */
void printer_host_task(void *arg)
{
    (void) arg;
    ESP_LOGI(TAG, "USB Host task active on Core %d", xPortGetCoreID());

    usb_host_config_t host_cfg = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    s_init_ret = usb_host_install(&host_cfg);
    if (s_init_ret != ESP_OK) {
        ESP_LOGE(TAG, "usb_host_install failed on Core %d: %s", xPortGetCoreID(), esp_err_to_name(s_init_ret));
        if (s_init_sem) xSemaphoreGive(s_init_sem);
        vTaskDelete(NULL);
        return;
    }

    const usb_host_client_config_t client_cfg = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {
            .client_event_callback = client_event_cb,
            .callback_arg = NULL,
        },
    };
    s_init_ret = usb_host_client_register(&client_cfg, &g_printer_dev.client_hdl);
    if (s_init_ret != ESP_OK) {
        ESP_LOGE(TAG, "usb_host_client_register failed: %s", esp_err_to_name(s_init_ret));
        if (s_init_sem) xSemaphoreGive(s_init_sem);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "USB Host Stack and Client Registered on Core %d", xPortGetCoreID());
    if (s_init_sem) xSemaphoreGive(s_init_sem);

    while (true) {
        uint32_t event_flags;
        esp_err_t ret = usb_host_lib_handle_events(pdMS_TO_TICKS(10), &event_flags);
        if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG, "usb_host_lib_handle_events error: %s", esp_err_to_name(ret));
        }
        if (g_printer_dev.client_hdl) {
            usb_host_client_handle_events(g_printer_dev.client_hdl, pdMS_TO_TICKS(10));
        }
    }
    vTaskDelete(NULL);
}

/* -------------------------------------------------------------------------- */
/*  Host Initialization
 * -------------------------------------------------------------------------- */
esp_err_t printer_host_init(void)
{
    s_init_sem = xSemaphoreCreateBinary();
    if (!s_init_sem) {
        return ESP_ERR_NO_MEM;
    }

    // Pin the task to Core 1 (xCoreID = 1) so that all interrupt allocations occur on CPU1
    BaseType_t task_ret = xTaskCreatePinnedToCore(printer_host_task, "usb_host", 4096, NULL, 5, NULL, 1);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create printer_host_task on Core 1");
        vSemaphoreDelete(s_init_sem);
        s_init_sem = NULL;
        return ESP_FAIL;
    }

    // Wait for the initialization on Core 1 to complete
    xSemaphoreTake(s_init_sem, portMAX_DELAY);
    vSemaphoreDelete(s_init_sem);
    s_init_sem = NULL;

    return s_init_ret;
}

void printer_register_event_cb(printer_event_cb_t cb)
{
    g_event_cb = cb;
}


/* -------------------------------------------------------------------------- */
/*  Client event callback – discovers Printer class interface
 * -------------------------------------------------------------------------- */
static void client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    ESP_LOGD(TAG, "client_event_cb: %s",
             (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) ? "NEW_DEV" :
             (event_msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) ? "DEV_GONE" : "OTHER");

    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        if (usb_host_device_open(g_printer_dev.client_hdl, event_msg->new_dev.address, &g_printer_dev.dev_hdl) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open device at address %d", event_msg->new_dev.address);
            return;
        }

        /* Get device descriptor for logging */
        const usb_device_desc_t *dev_desc;
        if (usb_host_get_device_descriptor(g_printer_dev.dev_hdl, &dev_desc) == ESP_OK) {
            ESP_LOGI(TAG, "USB Device connected: VID=0x%04X PID=0x%04X",
                     dev_desc->idVendor, dev_desc->idProduct);
        }

        /* Find Printer interface (class 0x07) */
        const usb_config_desc_t *cfg_desc;
        if (usb_host_get_active_config_descriptor(g_printer_dev.dev_hdl, &cfg_desc) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get config descriptor");
            return;
        }

        const usb_intf_desc_t *intf = NULL;
        for (uint8_t i = 0; i < cfg_desc->bNumInterfaces; ++i) {
            int offset = 0;
            const usb_intf_desc_t *cand = usb_parse_interface_descriptor(cfg_desc, i, 0, &offset);
            if (cand && cand->bInterfaceClass == 0x07) { // 0x07 = Printer class
                intf = cand;
                break;
            }
        }

        if (!intf) {
            ESP_LOGW(TAG, "No USB Printer interface found on device (Class 0x07)");
            usb_host_device_close(g_printer_dev.client_hdl, g_printer_dev.dev_hdl);
            return;
        }
        g_printer_dev.intf_desc = intf;

        /* Locate bulk endpoints */
        for (uint8_t e = 0; e < intf->bNumEndpoints; ++e) {
            int offset = 0;
            const usb_ep_desc_t *ep_desc = usb_parse_endpoint_descriptor_by_index(intf, e, cfg_desc->wTotalLength, &offset);
            if (!ep_desc) continue;
            if (USB_EP_DESC_GET_XFERTYPE(ep_desc) != USB_TRANSFER_TYPE_BULK)
                continue;

            if (ep_desc->bEndpointAddress & 0x80) { // IN
                g_printer_dev.bulk_in_ep_addr = ep_desc->bEndpointAddress;
                g_printer_dev.bulk_in_mps     = ep_desc->wMaxPacketSize;
                ESP_LOGI(TAG, "Found Bulk IN endpoint: 0x%02X (MPS: %d)",
                         g_printer_dev.bulk_in_ep_addr, g_printer_dev.bulk_in_mps);
            } else { // OUT
                g_printer_dev.bulk_out_ep_addr = ep_desc->bEndpointAddress;
                g_printer_dev.bulk_out_mps     = ep_desc->wMaxPacketSize;
                ESP_LOGI(TAG, "Found Bulk OUT endpoint: 0x%02X (MPS: %d)",
                         g_printer_dev.bulk_out_ep_addr, g_printer_dev.bulk_out_mps);
            }
        }

        if (usb_host_interface_claim(g_printer_dev.client_hdl, g_printer_dev.dev_hdl,
                                     intf->bInterfaceNumber, intf->bAlternateSetting) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to claim USB Printer interface");
            usb_host_device_close(g_printer_dev.client_hdl, g_printer_dev.dev_hdl);
            return;
        }

        if (g_printer_dev.transfer_sem == NULL) {
            g_printer_dev.transfer_sem = xSemaphoreCreateBinary();
        }
        if (g_printer_dev.tx_mutex == NULL) {
            g_printer_dev.tx_mutex = xSemaphoreCreateMutex();
        }

        g_printer_dev.busy = false;
        g_printer_dev.is_opened = true;
        ESP_LOGI(TAG, "USB Printer interface claimed successfully!");

        if (g_event_cb) {
            g_event_cb(PRINTER_EVENT_CONNECTED);
        }

    } else if (event_msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        ESP_LOGI(TAG, "USB Printer disconnected!");
        g_printer_dev.is_opened = false;

        if (g_printer_dev.dev_hdl) {
            usb_host_device_close(g_printer_dev.client_hdl, g_printer_dev.dev_hdl);
            g_printer_dev.dev_hdl = NULL;
        }

        if (g_event_cb) {
            g_event_cb(PRINTER_EVENT_DISCONNECTED);
        }
    }
}

/* -------------------------------------------------------------------------- */
/*  Transfer callback – releases semaphore
 * -------------------------------------------------------------------------- */
static void transfer_cb(usb_transfer_t *transfer)
{
    printer_dev_t *dev = (printer_dev_t *)transfer->context;
    BaseType_t higher = pdFALSE;
    xSemaphoreGiveFromISR(dev->transfer_sem, &higher);
    if (higher) {
        portYIELD_FROM_ISR();
    }
}

/* -------------------------------------------------------------------------- */
/*  Transmit data using USB Bulk OUT
 * -------------------------------------------------------------------------- */
esp_err_t printer_send_data(const uint8_t *data, size_t len)
{
    if (!g_printer_dev.is_opened) {
        ESP_LOGE(TAG, "Printer not connected");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_printer_dev.tx_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire TX mutex");
        return ESP_ERR_TIMEOUT;
    }

    g_printer_dev.busy = true;

    /* Flush out stale semaphores */
    while (xSemaphoreTake(g_printer_dev.transfer_sem, 0) == pdTRUE) {}

    esp_err_t ret = ESP_OK;

    usb_transfer_t *transfer = NULL;
    ret = usb_host_transfer_alloc(len, 0, &transfer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to allocate USB transfer of size %d: %s", (int)len, esp_err_to_name(ret));
        g_printer_dev.busy = false;
        xSemaphoreGive(g_printer_dev.tx_mutex);
        return ret;
    }

    memcpy(transfer->data_buffer, data, len);
    transfer->device_handle = g_printer_dev.dev_hdl;
    transfer->bEndpointAddress = g_printer_dev.bulk_out_ep_addr;
    transfer->num_bytes = len;
    transfer->callback = transfer_cb;
    transfer->context = &g_printer_dev;

    ESP_LOGI(TAG, "Submitting bulk OUT transfer of %d bytes...", (int)len);
    ret = usb_host_transfer_submit(transfer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "usb_host_transfer_submit failed: %s", esp_err_to_name(ret));
        usb_host_transfer_free(transfer);
        g_printer_dev.busy = false;
        xSemaphoreGive(g_printer_dev.tx_mutex);
        return ret;
    }

    // Wait up to 10 seconds for the transfer to complete
    if (xSemaphoreTake(g_printer_dev.transfer_sem, pdMS_TO_TICKS(10000)) != pdTRUE) {
        ESP_LOGE(TAG, "Bulk OUT transfer timeout");
        ret = ESP_ERR_TIMEOUT;
    } else {
        if (transfer->status != USB_TRANSFER_STATUS_COMPLETED) {
            ESP_LOGE(TAG, "Transfer failed with status %d", transfer->status);
            ret = ESP_FAIL;
        } else {
            ESP_LOGI(TAG, "Bulk OUT transfer completed successfully!");
        }
    }

    usb_host_transfer_free(transfer);
    g_printer_dev.busy = false;
    xSemaphoreGive(g_printer_dev.tx_mutex);

    return ret;
}
