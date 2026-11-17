/*
 * 没有 Wi-Fi 的虚拟主板：esp_wifi / esp_netif / esp_event / mDNS 全部返回失败或空结果。
 * 上游只在 NVS 里配过网时才启动 Wi-Fi；虚拟设备从未配网，这些函数在正常运行中不会被调用。
 */
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "mdns.h"

ESP_EVENT_DEFINE_BASE(WIFI_EVENT);
ESP_EVENT_DEFINE_BASE(IP_EVENT);

esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }
esp_err_t esp_event_loop_delete_default(void) { return ESP_OK; }
esp_err_t esp_event_handler_register(esp_event_base_t b, int32_t id, esp_event_handler_t h, void* a) { (void)b; (void)id; (void)h; (void)a; return ESP_OK; }
esp_err_t esp_event_handler_unregister(esp_event_base_t b, int32_t id, esp_event_handler_t h) { (void)b; (void)id; (void)h; return ESP_OK; }
esp_err_t esp_event_handler_instance_register(esp_event_base_t b, int32_t id, esp_event_handler_t h, void* a, esp_event_handler_instance_t* i) {
  (void)b; (void)id; (void)h; (void)a;
  if (i) *i = NULL;
  return ESP_OK;
}
esp_err_t esp_event_handler_instance_unregister(esp_event_base_t b, int32_t id, esp_event_handler_instance_t i) { (void)b; (void)id; (void)i; return ESP_OK; }
esp_err_t esp_event_post(esp_event_base_t b, int32_t id, const void* d, size_t s, TickType_t w) { (void)b; (void)id; (void)d; (void)s; (void)w; return ESP_OK; }

esp_err_t esp_netif_init(void) { return ESP_OK; }
esp_netif_t* esp_netif_create_default_wifi_sta(void) { return NULL; }
esp_err_t esp_netif_get_ip_info(esp_netif_t* n, esp_netif_ip_info_t* info) { (void)n; (void)info; return ESP_FAIL; }
void esp_netif_destroy(esp_netif_t* n) { (void)n; }

esp_err_t esp_wifi_init(const wifi_init_config_t* c) { (void)c; return ESP_ERR_NOT_SUPPORTED; }
esp_err_t esp_wifi_deinit(void) { return ESP_OK; }
esp_err_t esp_wifi_set_mode(wifi_mode_t m) { (void)m; return ESP_ERR_WIFI_NOT_INIT; }
esp_err_t esp_wifi_set_config(wifi_interface_t i, wifi_config_t* c) { (void)i; (void)c; return ESP_ERR_WIFI_NOT_INIT; }
esp_err_t esp_wifi_start(void) { return ESP_ERR_WIFI_NOT_INIT; }
esp_err_t esp_wifi_stop(void) { return ESP_OK; }
esp_err_t esp_wifi_connect(void) { return ESP_ERR_WIFI_NOT_INIT; }
esp_err_t esp_wifi_disconnect(void) { return ESP_OK; }
esp_err_t esp_wifi_set_ps(wifi_ps_type_t t) { (void)t; return ESP_OK; }
esp_err_t esp_wifi_set_storage(wifi_storage_t s) { (void)s; return ESP_OK; }
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t* ap) { (void)ap; return ESP_ERR_WIFI_CONN; }

esp_err_t mdns_init(void) { return ESP_ERR_NOT_SUPPORTED; }
void mdns_free(void) {}
esp_err_t mdns_hostname_set(const char* h) { (void)h; return ESP_OK; }
esp_err_t mdns_netif_action(esp_netif_t* n, mdns_event_actions_t a) { (void)n; (void)a; return ESP_OK; }
mdns_search_once_t* mdns_query_async_new(const char* n, const char* s, const char* p, uint16_t t, uint32_t to, size_t m, void* f) {
  (void)n; (void)s; (void)p; (void)t; (void)to; (void)m; (void)f;
  return NULL;
}
bool mdns_query_async_delete(mdns_search_once_t* s) { (void)s; return true; }
bool mdns_query_async_get_results(mdns_search_once_t* s, uint32_t t, mdns_result_t** r, uint8_t* n) {
  (void)s; (void)t;
  if (r) *r = NULL;
  if (n) *n = 0;
  return true;
}
void mdns_query_results_free(mdns_result_t* r) { (void)r; }
