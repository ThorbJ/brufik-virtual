#pragma once
#include <stdint.h>
#include "esp_afe_config.h"
#include "freertos/FreeRTOS.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct esp_afe_sr_data_t esp_afe_sr_data_t;
typedef enum { VAD_SILENCE = 0, VAD_SPEECH = 1 } vad_state_t;
typedef enum { WAKENET_NO_DETECT = 0, WAKENET_CHANNEL_VERIFIED = -1, WAKENET_DETECTED = 1 } wakenet_state_t;

typedef struct {
  int16_t* data;
  int data_size;
  int vad_cache_size;
  int16_t* vad_cache;
  float data_volume;
  wakenet_state_t wakeup_state;
  int wake_word_index;
  int wakenet_model_index;
  vad_state_t vad_state;
  int trigger_channel_id;
  int wake_word_length;
  int ret_value;
  int16_t* raw_data;
  int raw_data_channels;
  void* reserved;
} afe_fetch_result_t;

typedef struct {
  esp_afe_sr_data_t* (*create_from_config)(afe_config_t* cfg);
  int (*feed)(esp_afe_sr_data_t* afe, const int16_t* in);
  afe_fetch_result_t* (*fetch)(esp_afe_sr_data_t* afe);
  afe_fetch_result_t* (*fetch_with_delay)(esp_afe_sr_data_t* afe, TickType_t ticks_to_wait);
  int (*reset_buffer)(esp_afe_sr_data_t* afe);
  int (*get_feed_chunksize)(esp_afe_sr_data_t* afe);
  int (*get_fetch_chunksize)(esp_afe_sr_data_t* afe);
  int (*get_feed_channel_num)(esp_afe_sr_data_t* afe);
  int (*get_fetch_channel_num)(esp_afe_sr_data_t* afe);
  int (*get_samp_rate)(esp_afe_sr_data_t* afe);
  int (*set_wakenet_threshold)(esp_afe_sr_data_t* afe, int index, float threshold);
  int (*reset_wakenet_threshold)(esp_afe_sr_data_t* afe, int index);
  int (*disable_wakenet)(esp_afe_sr_data_t* afe);
  int (*enable_wakenet)(esp_afe_sr_data_t* afe);
  int (*disable_aec)(esp_afe_sr_data_t* afe);
  int (*enable_aec)(esp_afe_sr_data_t* afe);
  int (*disable_se)(esp_afe_sr_data_t* afe);
  int (*enable_se)(esp_afe_sr_data_t* afe);
  int (*disable_vad)(esp_afe_sr_data_t* afe);
  int (*enable_vad)(esp_afe_sr_data_t* afe);
  int (*reset_vad)(esp_afe_sr_data_t* afe);
  int (*disable_ns)(esp_afe_sr_data_t* afe);
  int (*enable_ns)(esp_afe_sr_data_t* afe);
  int (*disable_agc)(esp_afe_sr_data_t* afe);
  int (*enable_agc)(esp_afe_sr_data_t* afe);
  void (*print_pipeline)(esp_afe_sr_data_t* afe);
  void (*destroy)(esp_afe_sr_data_t* afe);
} esp_afe_sr_iface_t;

#ifdef __cplusplus
}
#endif
