/* esp_netif（子集）：虚拟设备没有网卡，只够 Wi-Fi 路径编译；该路径在未配网时从不运行。 */
#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "esp_event.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct esp_netif_obj esp_netif_t;
typedef struct { uint32_t addr; } esp_ip4_addr_t;
typedef struct { uint32_t addr[4]; uint8_t zone; } esp_ip6_addr_t;
typedef struct { esp_ip4_addr_t ip, netmask, gw; } esp_netif_ip_info_t;
#define ESP_IPADDR_TYPE_V4 0
#define ESP_IPADDR_TYPE_V6 6
#define ESP_IPADDR_TYPE_ANY 46
typedef struct { union { esp_ip6_addr_t ip6; esp_ip4_addr_t ip4; } u_addr; uint8_t type; } esp_ip_addr_t;
#define esp_ip4_addr_get_byte(ipaddr, idx) (((const uint8_t*)(&(ipaddr)->addr))[idx])
#define esp_ip4_addr1(ipaddr) esp_ip4_addr_get_byte(ipaddr, 0)
#define esp_ip4_addr2(ipaddr) esp_ip4_addr_get_byte(ipaddr, 1)
#define esp_ip4_addr3(ipaddr) esp_ip4_addr_get_byte(ipaddr, 2)
#define esp_ip4_addr4(ipaddr) esp_ip4_addr_get_byte(ipaddr, 3)
#define IP2STR(ipaddr) esp_ip4_addr1(ipaddr), esp_ip4_addr2(ipaddr), esp_ip4_addr3(ipaddr), esp_ip4_addr4(ipaddr)
#define IPSTR "%d.%d.%d.%d"
typedef enum { IP_EVENT_STA_GOT_IP, IP_EVENT_STA_LOST_IP, IP_EVENT_AP_STAIPASSIGNED, IP_EVENT_GOT_IP6, IP_EVENT_ETH_GOT_IP, IP_EVENT_ETH_LOST_IP, IP_EVENT_PPP_GOT_IP, IP_EVENT_PPP_LOST_IP } ip_event_t;
typedef struct { esp_netif_t* esp_netif; esp_netif_ip_info_t ip_info; bool ip_changed; } ip_event_got_ip_t;
esp_err_t esp_netif_init(void);
esp_netif_t* esp_netif_create_default_wifi_sta(void);
esp_err_t esp_netif_get_ip_info(esp_netif_t* netif, esp_netif_ip_info_t* info);
void esp_netif_destroy(esp_netif_t* netif);
#ifdef __cplusplus
}
#endif
