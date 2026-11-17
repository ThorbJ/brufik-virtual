/* mDNS（子集）：只够 Wi-Fi 发现路径编译；虚拟设备没有网卡，mdns_init 返回失败。 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_netif.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MDNS_TYPE_A 0x0001
#define MDNS_TYPE_PTR 0x000C
#define MDNS_TYPE_TXT 0x0010
#define MDNS_TYPE_AAAA 0x001C
#define MDNS_TYPE_SRV 0x0021
#define MDNS_TYPE_ANY 0x00FF
typedef enum { MDNS_EVENT_ENABLE_IP4 = 1 << 1, MDNS_EVENT_ENABLE_IP6 = 1 << 2, MDNS_EVENT_ANNOUNCE_IP4 = 1 << 3, MDNS_EVENT_ANNOUNCE_IP6 = 1 << 4, MDNS_EVENT_DISABLE_IP4 = 1 << 5, MDNS_EVENT_DISABLE_IP6 = 1 << 6 } mdns_event_actions_t;
typedef struct mdns_search_once_s mdns_search_once_t;
typedef struct { const char* key; const char* value; } mdns_txt_item_t;
typedef struct mdns_ip_addr_s { esp_ip_addr_t addr; struct mdns_ip_addr_s* next; } mdns_ip_addr_t;
typedef struct mdns_result_s {
  struct mdns_result_s* next;
  esp_netif_t* esp_netif;
  uint32_t ttl;
  int ip_protocol;
  char* instance_name;
  char* service_type;
  char* proto;
  char* hostname;
  uint16_t port;
  mdns_txt_item_t* txt;
  uint8_t* txt_value_len;
  size_t txt_count;
  mdns_ip_addr_t* addr;
} mdns_result_t;
esp_err_t mdns_init(void);
void mdns_free(void);
esp_err_t mdns_hostname_set(const char* hostname);
esp_err_t mdns_netif_action(esp_netif_t* esp_netif, mdns_event_actions_t event_action);
mdns_search_once_t* mdns_query_async_new(const char* name, const char* service_type, const char* proto, uint16_t type, uint32_t timeout, size_t max_results, void* notifier);
bool mdns_query_async_delete(mdns_search_once_t* search);
bool mdns_query_async_get_results(mdns_search_once_t* search, uint32_t timeout, mdns_result_t** results, uint8_t* num_results);
void mdns_query_results_free(mdns_result_t* results);
#ifdef __cplusplus
}
#endif
