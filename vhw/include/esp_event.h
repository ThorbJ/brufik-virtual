/* ESP 事件循环（子集）：小歪只在 Wi-Fi 路径用；虚拟设备没有 Wi-Fi，注册成功但永远不会有事件。 */
#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef const char* esp_event_base_t;
typedef void* esp_event_loop_handle_t;
typedef void (*esp_event_handler_t)(void* arg, esp_event_base_t base, int32_t id, void* data);
typedef void* esp_event_handler_instance_t;
#define ESP_EVENT_ANY_BASE NULL
#define ESP_EVENT_ANY_ID (-1)
#define ESP_EVENT_DECLARE_BASE(id) extern esp_event_base_t const id
#define ESP_EVENT_DEFINE_BASE(id) esp_event_base_t const id = #id
ESP_EVENT_DECLARE_BASE(WIFI_EVENT);
ESP_EVENT_DECLARE_BASE(IP_EVENT);
esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_loop_delete_default(void);
esp_err_t esp_event_handler_register(esp_event_base_t base, int32_t id, esp_event_handler_t handler, void* arg);
esp_err_t esp_event_handler_unregister(esp_event_base_t base, int32_t id, esp_event_handler_t handler);
esp_err_t esp_event_handler_instance_register(esp_event_base_t base, int32_t id, esp_event_handler_t handler, void* arg, esp_event_handler_instance_t* inst);
esp_err_t esp_event_handler_instance_unregister(esp_event_base_t base, int32_t id, esp_event_handler_instance_t inst);
esp_err_t esp_event_post(esp_event_base_t base, int32_t id, const void* data, size_t size, TickType_t wait);
#ifdef __cplusplus
}
#endif
