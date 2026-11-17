/*
 * ESP-SR AFE（VC 模式：AEC + NS + VAD + AGC）的虚拟实现，接口与 esp-sr 一致，"创建成功"。
 *   - 回声消除 / 降噪：浏览器 getUserMedia 自带（echoCancellation / noiseSuppression），这里直通麦克风声道；
 *   - AGC：按配置的线性增益放大并限幅；
 *   - VAD：开源 WebRTC VAD（libfvad，BSD-3），按配置的 vad_mode 和 min_speech_ms / min_noise_ms 做进出迟滞；
 *     判的是增益前的麦克风声道（真库的 VAD 也在 AGC 之前）。
 * 输入每帧交错 [mic, ref]，块长 512 样本（32 ms，同真库 VC 模式）；输出单声道 512 样本。
 */
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_afe_sr_models.h"
#include "esp_err.h"
#include "fvad.h"
#include "vdev_io.h"
#if VHW_VIRTUAL_TIME
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

#define CHUNK 512
#define RING (CHUNK * 16)
#define VAD_FRAME 160 /* libfvad 每次判 10 ms（16 kHz） */

struct esp_afe_sr_data_t {
  afe_config_t cfg;
  pthread_mutex_t m;
  pthread_cond_t cv;
  int16_t ring[RING];
  uint32_t head, tail;
  int16_t out[CHUNK];
  afe_fetch_result_t res;
  int speech;
  int speech_ms, silence_ms;
  Fvad* vad;
  int16_t vad_buf[VAD_FRAME];
  int vad_fill;
};

afe_config_t* afe_config_init(const char* fmt, srmodel_list_t* models, afe_type_t type, afe_mode_t mode) {
  (void)models;
  afe_config_t* c = calloc(1, sizeof *c);
  if (!c) return NULL;
  int mics = 0, refs = 0;
  for (const char* p = fmt ? fmt : "MR"; *p; p++) { if (*p == 'M') mics++; else if (*p == 'R') refs++; }
  c->pcm_config = (afe_pcm_config_t){mics + refs, mics, refs, 16000};
  c->afe_type = type;
  c->afe_mode = mode;
  c->aec_init = refs > 0;
  c->ns_init = true;
  c->vad_init = true;
  c->agc_init = true;
  c->afe_linear_gain = 1.0f;
  c->afe_ringbuf_size = 50;
  return c;
}
afe_config_t* afe_config_check(afe_config_t* c) {
  if (c && c->pcm_config.ref_num == 0) c->aec_init = false;
  return c;
}
void afe_config_free(afe_config_t* c) { free(c); }
void afe_config_print(const afe_config_t* c) { (void)c; }

static esp_afe_sr_data_t* create(afe_config_t* cfg) {
  esp_afe_sr_data_t* a = calloc(1, sizeof *a);
  if (!a) return NULL;
  a->cfg = *cfg;
  pthread_mutex_init(&a->m, NULL);
  pthread_cond_init(&a->cv, NULL);
  a->vad = fvad_new();
  /* ESP-SR VAD_MODE_0..4（越大越严）→ WebRTC VAD 0..3 */
  const int mode = cfg->vad_mode > 3 ? 3 : (int)cfg->vad_mode;
  if (!a->vad || fvad_set_sample_rate(a->vad, 16000) != 0 || fvad_set_mode(a->vad, mode) != 0) {
    if (a->vad) fvad_free(a->vad);
    free(a);
    return NULL;
  }
  vdev_logf("[vhw] ESP-SR AFE (virtual): AEC/NS = browser, VAD = WebRTC (libfvad) mode %d, gain=%.1f\n", mode, (double)cfg->afe_linear_gain);
  return a;
}

static int feed(esp_afe_sr_data_t* a, const int16_t* in) {
  const int ch = a->cfg.pcm_config.total_ch_num > 0 ? a->cfg.pcm_config.total_ch_num : 1;
  pthread_mutex_lock(&a->m);
  for (int i = 0; i < CHUNK; i++) {
    if (a->head - a->tail >= RING) a->tail++; /* 满了丢最旧的 */
    a->ring[a->head++ % RING] = in[i * ch]; /* 第 0 声道 = 麦克风 */
  }
  pthread_cond_broadcast(&a->cv);
  pthread_mutex_unlock(&a->m);
  return CHUNK;
}

static afe_fetch_result_t* fetch_with_delay(esp_afe_sr_data_t* a, TickType_t ticks) {
  pthread_mutex_lock(&a->m);
#if VHW_VIRTUAL_TIME
  /* 虚拟时间：一次只跑一个任务，不能在条件变量上干等（送数据的任务轮不到）；按设备时钟每 4 ms 看一次 */
  for (uint32_t waited = 0, limit = ticks == portMAX_DELAY ? 60000u : (uint32_t)ticks; a->head - a->tail < CHUNK && waited < limit; waited += 4) {
    pthread_mutex_unlock(&a->m);
    vTaskDelay(4);
    pthread_mutex_lock(&a->m);
  }
#else
  if (a->head - a->tail < CHUNK) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t ns = (uint64_t)ts.tv_nsec + (uint64_t)(ticks == portMAX_DELAY ? 60000 : ticks) * 1000000ull;
    ts.tv_sec += (time_t)(ns / 1000000000ull);
    ts.tv_nsec = (long)(ns % 1000000000ull);
    while (a->head - a->tail < CHUNK)
      if (pthread_cond_timedwait(&a->cv, &a->m, &ts) != 0) break;
  }
#endif
  memset(&a->res, 0, sizeof a->res);
  if (a->head - a->tail < CHUNK) {
    pthread_mutex_unlock(&a->m);
    a->res.ret_value = ESP_FAIL;
    return &a->res;
  }
  const float gain = a->cfg.agc_init && a->cfg.afe_linear_gain > 0 ? a->cfg.afe_linear_gain : 1.0f;
  double energy = 0;
  int voiced = 0, judged = 0;
  for (int i = 0; i < CHUNK; i++) {
    const int16_t raw = a->ring[a->tail++ % RING];
    a->vad_buf[a->vad_fill++] = raw;
    if (a->vad_fill == VAD_FRAME) {
      a->vad_fill = 0;
      judged++;
      if (fvad_process(a->vad, a->vad_buf, VAD_FRAME) == 1) voiced++;
    }
    float v = (float)raw * gain;
    if (v > 32767.f) v = 32767.f;
    if (v < -32768.f) v = -32768.f;
    a->out[i] = (int16_t)v;
    energy += (double)v * v;
  }
  pthread_mutex_unlock(&a->m);
  const double rms = sqrt(energy / CHUNK);
  const float db = rms > 1 ? (float)(20.0 * log10(rms / 32768.0)) : -96.f;
  /* 这一块（32 ms，含 3 个 10 ms 判定）多数判有声就算有声；连续 min_speech_ms 进入、连续 min_noise_ms 退出 */
  const int chunk_ms = CHUNK * 1000 / 16000;
  if (judged > 0 && voiced * 2 > judged) { a->speech_ms += chunk_ms; a->silence_ms = 0; } else { a->silence_ms += chunk_ms; a->speech_ms = 0; }
  if (!a->speech && a->speech_ms >= (a->cfg.vad_min_speech_ms > 0 ? a->cfg.vad_min_speech_ms : 128)) a->speech = 1;
  if (a->speech && a->silence_ms >= (a->cfg.vad_min_noise_ms > 0 ? a->cfg.vad_min_noise_ms : 500)) a->speech = 0;
  a->res.data = a->out;
  a->res.data_size = CHUNK * (int)sizeof(int16_t);
  a->res.data_volume = db;
  a->res.vad_state = a->speech ? VAD_SPEECH : VAD_SILENCE;
  a->res.ret_value = ESP_OK;
  return &a->res;
}

static afe_fetch_result_t* fetch(esp_afe_sr_data_t* a) { return fetch_with_delay(a, portMAX_DELAY); }
static int reset_buffer(esp_afe_sr_data_t* a) { pthread_mutex_lock(&a->m); a->tail = a->head; pthread_mutex_unlock(&a->m); return 0; }
static int reset_vad(esp_afe_sr_data_t* a) {
  pthread_mutex_lock(&a->m);
  fvad_reset(a->vad);
  fvad_set_sample_rate(a->vad, 16000);
  fvad_set_mode(a->vad, a->cfg.vad_mode > 3 ? 3 : (int)a->cfg.vad_mode);
  a->vad_fill = 0;
  a->speech = 0;
  a->speech_ms = a->silence_ms = 0;
  pthread_mutex_unlock(&a->m);
  return 0;
}
static int feed_chunk(esp_afe_sr_data_t* a) { (void)a; return CHUNK; }
static int fetch_chunk(esp_afe_sr_data_t* a) { (void)a; return CHUNK; }
static int feed_ch(esp_afe_sr_data_t* a) { return a->cfg.pcm_config.total_ch_num; }
static int fetch_ch(esp_afe_sr_data_t* a) { (void)a; return 1; }
static int samp_rate(esp_afe_sr_data_t* a) { (void)a; return 16000; }
static int noop(esp_afe_sr_data_t* a) { (void)a; return 0; }
static int noop_thr(esp_afe_sr_data_t* a, int i, float t) { (void)a; (void)i; (void)t; return 0; }
static int noop_idx(esp_afe_sr_data_t* a, int i) { (void)a; (void)i; return 0; }
static void print_pipeline(esp_afe_sr_data_t* a) { (void)a; vdev_logf("[vhw] AFE pipeline: |AEC(browser)| -> |NS(browser)| -> |VAD(WebRTC/libfvad)| -> |AGC(gain)|\n"); }
static void destroy(esp_afe_sr_data_t* a) { if (a) fvad_free(a->vad); free(a); }

static esp_afe_sr_iface_t s_iface = {
    .create_from_config = create,
    .feed = feed,
    .fetch = fetch,
    .fetch_with_delay = fetch_with_delay,
    .reset_buffer = reset_buffer,
    .get_feed_chunksize = feed_chunk,
    .get_fetch_chunksize = fetch_chunk,
    .get_feed_channel_num = feed_ch,
    .get_fetch_channel_num = fetch_ch,
    .get_samp_rate = samp_rate,
    .set_wakenet_threshold = noop_thr,
    .reset_wakenet_threshold = noop_idx,
    .disable_wakenet = noop, .enable_wakenet = noop,
    .disable_aec = noop, .enable_aec = noop,
    .disable_se = noop, .enable_se = noop,
    .disable_vad = noop, .enable_vad = noop, .reset_vad = reset_vad,
    .disable_ns = noop, .enable_ns = noop,
    .disable_agc = noop, .enable_agc = noop,
    .print_pipeline = print_pipeline,
    .destroy = destroy,
};

esp_afe_sr_iface_t* esp_afe_handle_from_config(const afe_config_t* cfg) { (void)cfg; return &s_iface; }
