#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/ble_store.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs_flash.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "store/config/ble_store_config.h"

#define TAG "ble_receiver"
#define DEVICE_NAME "ESP32S3-BLE-RX"
#define MESSAGE_SERVICE_UUID 0xFFF0
#define MESSAGE_RX_CHAR_UUID 0xFFF1
#define MESSAGE_TX_CHAR_UUID 0xFFF2
#define MESSAGE_MAX_LEN 180
static uint8_t s_addr_type;
static uint16_t s_tx_char_handle;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool s_tx_notify_enabled;
static char s_last_rx_message[MESSAGE_MAX_LEN + 1];
static uint16_t s_last_rx_len;
static char s_last_tx_message[MESSAGE_MAX_LEN + 32] = "BLE receiver ready";
static uint16_t s_last_tx_len = sizeof("BLE receiver ready") - 1;

void ble_store_config_init(void);
struct os_mbuf *ble_hs_mbuf_from_flat(const void *buf, uint16_t len);

static int gatt_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);
static int gap_event_handler(struct ble_gap_event *event, void *arg);
static void start_advertising(void);

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(MESSAGE_SERVICE_UUID),
        .characteristics =
            (struct ble_gatt_chr_def[]) {
                {
                    .uuid = BLE_UUID16_DECLARE(MESSAGE_TX_CHAR_UUID),
                    .access_cb = gatt_chr_access,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                    .val_handle = &s_tx_char_handle,
                },
                {
                    .uuid = BLE_UUID16_DECLARE(MESSAGE_RX_CHAR_UUID),
                    .access_cb = gatt_chr_access,
                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                },
                {0},
            },
    },
    {0},
};

static int append_text_response(struct os_mbuf *om, const char *text,
                                uint16_t text_len) {
    int rc = os_mbuf_append(om, text, text_len);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static bool is_trim_char(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static void update_tx_message(const char *text, uint16_t text_len) {
    int written;

    if (text_len == 3 && strncmp(text, "dog", 3) == 0) {
        written = snprintf(s_last_tx_message, sizeof(s_last_tx_message), "gou");
    } else if (text_len == 3 && strncmp(text, "cat", 3) == 0) {
        written = snprintf(s_last_tx_message, sizeof(s_last_tx_message), "mao");
    } else {
        written = snprintf(s_last_tx_message, sizeof(s_last_tx_message),
                           "Received: %.*s", text_len, text);
    }

    if (written < 0) {
        s_last_tx_message[0] = '\0';
        s_last_tx_len = 0;
        return;
    }

    s_last_tx_len = written;
    if (s_last_tx_len >= sizeof(s_last_tx_message)) {
        s_last_tx_len = sizeof(s_last_tx_message) - 1;
    }
}

static void notify_phone_if_possible(void) {
    struct os_mbuf *om;
    int rc;

    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || !s_tx_notify_enabled) {
        return;
    }

    om = ble_hs_mbuf_from_flat(s_last_tx_message, s_last_tx_len);
    if (om == NULL) {
        ESP_LOGW(TAG, "create notify payload failed");
        return;
    }

    rc = ble_gatts_notify_custom(s_conn_handle, s_tx_char_handle, om);
    if (rc != 0) {
        ESP_LOGW(TAG, "notify failed, rc=%d", rc);
    }
}

static int handle_rx_write(struct os_mbuf *om) {
    int rc;
    uint16_t payload_len;
    uint16_t text_start = 0;
    uint16_t text_end;
    uint16_t text_len;

    payload_len = OS_MBUF_PKTLEN(om);
    if (payload_len > MESSAGE_MAX_LEN) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, s_last_rx_message, sizeof(s_last_rx_message) - 1,
                             &s_last_rx_len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    s_last_rx_message[s_last_rx_len] = '\0';
    text_end = s_last_rx_len;
    while (text_start < text_end && is_trim_char(s_last_rx_message[text_start])) {
        text_start++;
    }
    while (text_end > text_start && is_trim_char(s_last_rx_message[text_end - 1])) {
        text_end--;
    }

    text_len = text_end - text_start;
    update_tx_message(s_last_rx_message + text_start, text_len);

    if (text_len > 0) {
        printf("%.*s\r\n", (int)text_len, s_last_rx_message + text_start);
        fflush(stdout);
    }

    notify_phone_if_possible();
    return 0;
}

static int gatt_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
    (void)conn_handle;
    (void)arg;

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (attr_handle == s_tx_char_handle) {
            return append_text_response(ctxt->om, s_last_tx_message, s_last_tx_len);
        }
        break;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        return handle_rx_write(ctxt->om);

    default:
        break;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static void start_advertising(void) {
    struct ble_gap_adv_params adv_params = {0};
    struct ble_hs_adv_fields fields = {0};
    ble_uuid16_t service_uuid = BLE_UUID16_INIT(MESSAGE_SERVICE_UUID);
    const char *name = ble_svc_gap_device_name();
    int rc;

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    fields.uuids16 = &service_uuid;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "set adv fields failed, rc=%d", rc);
        return;
    }

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                           gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "start advertising failed, rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG, "advertising started, device name: %s", DEVICE_NAME);
}

static void log_connection_desc(uint16_t conn_handle) {
    struct ble_gap_conn_desc desc;
    int rc = ble_gap_conn_find(conn_handle, &desc);

    if (rc != 0) {
        ESP_LOGW(TAG, "find conn desc failed, rc=%d", rc);
        return;
    }

    ESP_LOGI(TAG,
             "connected, handle=%d interval=%d latency=%d timeout=%d",
             desc.conn_handle, desc.conn_itvl, desc.conn_latency,
             desc.supervision_timeout);
}

static int gap_event_handler(struct ble_gap_event *event, void *arg) {
    (void)arg;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            s_conn_handle = event->connect.conn_handle;
            s_tx_notify_enabled = false;
            log_connection_desc(s_conn_handle);
        } else {
            ESP_LOGW(TAG, "connect failed, status=%d", event->connect.status);
            start_advertising();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnected, reason=%d", event->disconnect.reason);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        s_tx_notify_enabled = false;
        start_advertising();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        s_tx_notify_enabled = event->subscribe.cur_notify;
        ESP_LOGI(TAG, "subscribe event, attr_handle=%d notify=%d indicate=%d",
                 event->subscribe.attr_handle, event->subscribe.cur_notify,
                 event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "mtu updated, conn_handle=%d mtu=%d",
                 event->mtu.conn_handle, event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "advertising complete, reason=%d", event->adv_complete.reason);
        start_advertising();
        return 0;

    default:
        return 0;
    }
}

static void on_reset(int reason) {
    ESP_LOGW(TAG, "nimble reset, reason=%d", reason);
}

static void on_sync(void) {
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "ensure addr failed, rc=%d", rc);
        return;
    }

    rc = ble_hs_id_infer_auto(0, &s_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "infer addr type failed, rc=%d", rc);
        return;
    }

    start_advertising();
}

static void host_task(void *param) {
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void init_nvs(void) {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

static void init_gatt_server(void) {
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGE(TAG, "set device name failed, rc=%d", rc);
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "count gatt config failed, rc=%d", rc);
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "add gatt services failed, rc=%d", rc);
        ESP_ERROR_CHECK(ESP_FAIL);
    }
}

void app_main(void) {
    esp_err_t ret;

    esp_log_level_set("*", ESP_LOG_WARN);

    init_nvs();

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nimble init failed, ret=%d", ret);
        return;
    }

    init_gatt_server();

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_store_config_init();

    ESP_LOGI(TAG, "BLE write service ready");
    ESP_LOGI(TAG, "service UUID: 0x%04X, RX char: 0x%04X, TX char: 0x%04X",
             MESSAGE_SERVICE_UUID, MESSAGE_RX_CHAR_UUID, MESSAGE_TX_CHAR_UUID);

    nimble_port_freertos_init(host_task);
}
