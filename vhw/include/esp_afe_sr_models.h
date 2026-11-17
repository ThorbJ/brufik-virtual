#pragma once
#include "esp_afe_sr_iface.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_afe_sr_iface_t* esp_afe_handle_from_config(const afe_config_t* cfg);
#ifdef __cplusplus
}
#endif
