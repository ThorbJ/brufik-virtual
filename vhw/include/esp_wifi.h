/*
 * Wi-Fi（子集）：虚拟设备没有无线电。所有调用返回"失败 / 未连接"；
 * 上游固件只在 NVS 里配过网时才走 Wi-Fi，虚拟设备从未配网，所以这条路径不会启动（小歪走 USB 串口）。
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#ifdef __cplusplus
extern "C" {
#endif
#define ESP_ERR_WIFI_NOT_INIT (ESP_ERR_WIFI_BASE + 1)
#define ESP_ERR_WIFI_NOT_STARTED (ESP_ERR_WIFI_BASE + 2)
#define ESP_ERR_WIFI_CONN (ESP_ERR_WIFI_BASE + 7)
typedef enum { WIFI_MODE_NULL = 0, WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA } wifi_mode_t;
typedef enum { WIFI_IF_STA = 0, WIFI_IF_AP = 1 } wifi_interface_t;
typedef enum { WIFI_PS_NONE, WIFI_PS_MIN_MODEM, WIFI_PS_MAX_MODEM } wifi_ps_type_t;
typedef enum { WIFI_STORAGE_FLASH, WIFI_STORAGE_RAM } wifi_storage_t;
typedef enum { WIFI_FAST_SCAN = 0, WIFI_ALL_CHANNEL_SCAN } wifi_scan_method_t;
typedef enum { WIFI_AUTH_OPEN = 0, WIFI_AUTH_WEP, WIFI_AUTH_WPA_PSK, WIFI_AUTH_WPA2_PSK, WIFI_AUTH_WPA_WPA2_PSK, WIFI_AUTH_ENTERPRISE, WIFI_AUTH_WPA3_PSK, WIFI_AUTH_WPA2_WPA3_PSK } wifi_auth_mode_t;
typedef enum {
  WIFI_EVENT_WIFI_READY = 0, WIFI_EVENT_SCAN_DONE, WIFI_EVENT_STA_START, WIFI_EVENT_STA_STOP, WIFI_EVENT_STA_CONNECTED,
  WIFI_EVENT_STA_DISCONNECTED, WIFI_EVENT_STA_AUTHMODE_CHANGE,
} wifi_event_t;
typedef struct { uint8_t ssid[32]; uint8_t ssid_len; uint8_t bssid[6]; uint8_t reason; int8_t rssi; } wifi_event_sta_disconnected_t;
typedef struct { wifi_auth_mode_t authmode; int8_t rssi_5g_adjustment; } wifi_scan_threshold_t;
typedef struct { bool capable; bool required; } wifi_pmf_config_t;
typedef struct {
  uint8_t ssid[32];
  uint8_t password[64];
  wifi_scan_method_t scan_method;
  bool bssid_set;
  uint8_t bssid[6];
  uint8_t channel;
  uint16_t listen_interval;
  int sort_method;
  wifi_scan_threshold_t threshold;
  wifi_pmf_config_t pmf_cfg;
} wifi_sta_config_t;
typedef union { wifi_sta_config_t sta; uint8_t ap_reserved[128]; } wifi_config_t;
typedef struct { uint8_t bssid[6]; uint8_t ssid[33]; uint8_t primary; int8_t rssi; wifi_auth_mode_t authmode; } wifi_ap_record_t;
typedef struct {
  void* event_handler;
  void* osi_funcs;
  void* wpa_crypto_funcs;
  int static_rx_buf_num;
  int dynamic_rx_buf_num;
  int tx_buf_type;
  int static_tx_buf_num;
  int dynamic_tx_buf_num;
  int rx_mgmt_buf_type;
  int rx_mgmt_buf_num;
  int cache_tx_buf_num;
  int csi_enable;
  int ampdu_rx_enable;
  int ampdu_tx_enable;
  int amsdu_tx_enable;
  int nvs_enable;
  int nano_enable;
  int rx_ba_win;
  int wifi_task_core_id;
  int beacon_max_len;
  int mgmt_sbuf_num;
  uint64_t feature_caps;
  bool sta_disconnected_pm;
  int espnow_max_encrypt_num;
  int tx_hetb_queue_num;
  bool dump_hesigb_enable;
  int magic;
} wifi_init_config_t;
#define WIFI_INIT_CONFIG_MAGIC 0x1F2F3F4F
#define WIFI_INIT_CONFIG_DEFAULT() { .static_rx_buf_num = 10, .dynamic_rx_buf_num = 32, .tx_buf_type = 1, .static_tx_buf_num = 0, .dynamic_tx_buf_num = 32, .nvs_enable = 1, .magic = WIFI_INIT_CONFIG_MAGIC }
esp_err_t esp_wifi_init(const wifi_init_config_t* config);
esp_err_t esp_wifi_deinit(void);
esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
esp_err_t esp_wifi_set_config(wifi_interface_t iface, wifi_config_t* conf);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_disconnect(void);
esp_err_t esp_wifi_set_ps(wifi_ps_type_t type);
esp_err_t esp_wifi_set_storage(wifi_storage_t storage);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t* ap_info);
#ifdef __cplusplus
}
#endif
