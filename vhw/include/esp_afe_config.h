/*
 * ESP-SR 音频前端（AFE）配置（子集，字段名与 esp-sr 2.x 一致）。
 * esp-sr 只有芯片专用的闭源库，编不进浏览器；虚拟实现见 vhw/src/esp_afe_vhw.c：
 * 回声消除 / 降噪交给浏览器麦克风自带的处理（getUserMedia），VAD 用能量判据，接口和真库一样"成功"。
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct { char** model_name; char** model_info; void** model_data; int num; } srmodel_list_t;
typedef enum { AFE_TYPE_SR = 0, AFE_TYPE_VC, AFE_TYPE_VC_8K, AFE_TYPE_MAX } afe_type_t;
typedef enum { AFE_MODE_LOW_COST = 0, AFE_MODE_HIGH_PERF } afe_mode_t;
typedef enum { AFE_MEMORY_ALLOC_MORE_INTERNAL = 1, AFE_MEMORY_ALLOC_INTERNAL_PSRAM_BALANCE, AFE_MEMORY_ALLOC_MORE_PSRAM } afe_memory_alloc_mode_t;
typedef enum { AFE_AGC_MODE_WEBRTC = 0, AFE_AGC_MODE_WAKENET } afe_agc_mode_t;
typedef enum { AFE_NS_MODE_WEBRTC = 0, AFE_NS_MODE_NET } afe_ns_mode_t;
typedef enum { AEC_MODE_SR_LOW_COST = 0, AEC_MODE_SR_HIGH_PERF, AEC_MODE_VOIP_LOW_COST, AEC_MODE_VOIP_HIGH_PERF } aec_mode_t;
typedef enum { VAD_MODE_0 = 0, VAD_MODE_1, VAD_MODE_2, VAD_MODE_3, VAD_MODE_4 } vad_mode_t;
typedef enum { DET_MODE_90 = 0, DET_MODE_95, DET_MODE_2CH_90, DET_MODE_2CH_95, DET_MODE_3CH_90, DET_MODE_3CH_95 } det_mode_t;

typedef struct { int total_ch_num; int mic_num; int ref_num; int sample_rate; } afe_pcm_config_t;

typedef struct {
  bool aec_init;
  aec_mode_t aec_mode;
  int aec_filter_length;
  bool se_init;
  bool ns_init;
  char* ns_model_name;
  afe_ns_mode_t afe_ns_mode;
  bool vad_init;
  vad_mode_t vad_mode;
  char* vad_model_name;
  int vad_min_speech_ms;
  int vad_min_noise_ms;
  int vad_delay_ms;
  bool vad_mute_playback;
  bool vad_enable_channel_trigger;
  bool wakenet_init;
  char* wakenet_model_name;
  char* wakenet_model_name_2;
  det_mode_t wakenet_mode;
  bool agc_init;
  afe_agc_mode_t agc_mode;
  int agc_compression_gain_db;
  int agc_target_level_dbfs;
  afe_pcm_config_t pcm_config;
  afe_mode_t afe_mode;
  afe_type_t afe_type;
  int afe_perferred_core;
  int afe_perferred_priority;
  int afe_ringbuf_size;
  afe_memory_alloc_mode_t memory_alloc_mode;
  float afe_linear_gain;
  bool debug_init;
  bool fixed_first_channel;
  char* ch_format;
} afe_config_t;

afe_config_t* afe_config_init(const char* input_format, srmodel_list_t* models, afe_type_t type, afe_mode_t mode);
afe_config_t* afe_config_check(afe_config_t* cfg);
void afe_config_free(afe_config_t* cfg);
void afe_config_print(const afe_config_t* cfg);

#ifdef __cplusplus
}
#endif
