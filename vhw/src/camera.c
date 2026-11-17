/*
 * 虚拟 OV3660 摄像头：esp_camera_fb_get() 通过接口向外界要一帧 JPEG（vdev_camera_grab）。
 * 外界没提供（例如 Node 测试或没开摄像头权限）就像真驱动超时一样返回 NULL。
 * 网页里由运行时把电脑的摄像头接上（camera 通道）。
 */
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "esp_camera.h"
#include "vdev_io.h"

#define CAM_BUF_CAP (256 * 1024)

const resolution_info_t resolution[] = {
    {96, 96, ASPECT_RATIO_1X1}, {160, 120, ASPECT_RATIO_4X3}, {128, 128, ASPECT_RATIO_1X1}, {176, 144, ASPECT_RATIO_5X4},
    {240, 176, ASPECT_RATIO_3X2}, {240, 240, ASPECT_RATIO_1X1}, {320, 240, ASPECT_RATIO_4X3}, {320, 320, ASPECT_RATIO_1X1},
    {400, 296, ASPECT_RATIO_4X3}, {480, 320, ASPECT_RATIO_3X2}, {640, 480, ASPECT_RATIO_4X3}, {800, 600, ASPECT_RATIO_4X3},
    {1024, 768, ASPECT_RATIO_4X3}, {1280, 720, ASPECT_RATIO_16X9}, {1280, 1024, ASPECT_RATIO_5X4}, {1600, 1200, ASPECT_RATIO_4X3},
};
static camera_sensor_info_t k_ov3660_info = {CAMERA_OV3660, "OV3660", 0x3C, OV3660_PID, FRAMESIZE_QXGA, true};

static int s_inited;
static camera_config_t s_cfg;
static sensor_t s_sensor;
static camera_fb_t s_fb;
static uint8_t* s_buf;
static int s_psram_mode;

static int ok_level(sensor_t* s, int v) { (void)s; (void)v; return 0; }
static int set_framesize(sensor_t* s, framesize_t f) { s->status.framesize = f; s_cfg.frame_size = f; return 0; }
static int set_quality(sensor_t* s, int q) { s->status.quality = (uint8_t)q; return 0; }
static int set_hmirror(sensor_t* s, int e) { s->status.hmirror = (uint8_t)e; return 0; }
static int set_vflip(sensor_t* s, int e) { s->status.vflip = (uint8_t)e; return 0; }
static int set_xclk(sensor_t* s, int timer, int mhz) { (void)timer; s->xclk_freq_hz = mhz * 1000000; return 0; }
static int set_pixformat(sensor_t* s, pixformat_t f) { s->pixformat = f; return 0; }
static int set_gainceiling(sensor_t* s, gainceiling_t g) { (void)s; (void)g; return 0; }
static int get_reg(sensor_t* s, int r, int m) { (void)s; (void)r; (void)m; return 0; }
static int set_reg(sensor_t* s, int r, int m, int v) { (void)s; (void)r; (void)m; (void)v; return 0; }

esp_err_t esp_camera_init(const camera_config_t* config) {
  if (!config) return ESP_ERR_INVALID_ARG;
  s_cfg = *config;
  memset(&s_sensor, 0, sizeof s_sensor);
  s_sensor.id = (sensor_id_t){0x7F, 0xA2, OV3660_PID, 0x00};
  s_sensor.slv_addr = 0x3C;
  s_sensor.pixformat = config->pixel_format;
  s_sensor.xclk_freq_hz = config->xclk_freq_hz;
  s_sensor.status.framesize = config->frame_size;
  s_sensor.status.quality = (uint8_t)config->jpeg_quality;
  s_sensor.set_pixformat = set_pixformat;
  s_sensor.set_framesize = set_framesize;
  s_sensor.set_quality = set_quality;
  s_sensor.set_hmirror = set_hmirror;
  s_sensor.set_vflip = set_vflip;
  s_sensor.set_xclk = set_xclk;
  s_sensor.set_gainceiling = set_gainceiling;
  s_sensor.get_reg = get_reg;
  s_sensor.set_reg = set_reg;
  int (**fns[])(sensor_t*, int) = {
      &s_sensor.set_contrast, &s_sensor.set_brightness, &s_sensor.set_saturation, &s_sensor.set_sharpness, &s_sensor.set_denoise,
      &s_sensor.set_colorbar, &s_sensor.set_whitebal, &s_sensor.set_gain_ctrl, &s_sensor.set_exposure_ctrl, &s_sensor.set_aec2,
      &s_sensor.set_awb_gain, &s_sensor.set_agc_gain, &s_sensor.set_aec_value, &s_sensor.set_special_effect, &s_sensor.set_wb_mode,
      &s_sensor.set_ae_level, &s_sensor.set_dcw, &s_sensor.set_bpc, &s_sensor.set_wpc, &s_sensor.set_raw_gma, &s_sensor.set_lenc,
  };
  for (size_t i = 0; i < sizeof fns / sizeof fns[0]; i++) *fns[i] = ok_level;
  if (!s_buf) s_buf = malloc(CAM_BUF_CAP);
  s_inited = s_buf != NULL;
  return s_inited ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t esp_camera_deinit(void) { s_inited = 0; return ESP_OK; }

camera_fb_t* esp_camera_fb_get(void) {
  if (!s_inited) return NULL;
  framesize_t fs = s_cfg.frame_size < (int)(sizeof resolution / sizeof resolution[0]) ? s_cfg.frame_size : FRAMESIZE_QVGA;
  const uint32_t w = resolution[fs].width, h = resolution[fs].height;
  const uint32_t fmt = s_cfg.pixel_format == PIXFORMAT_JPEG ? VDEV_CAMERA_JPEG : VDEV_CAMERA_RGB565;
  size_t len = vdev_camera_grab(w, h, fmt, s_buf, CAM_BUF_CAP);
  if (len == 0) return NULL;
  s_fb.buf = s_buf;
  s_fb.len = len;
  s_fb.width = w;
  s_fb.height = h;
  s_fb.format = s_cfg.pixel_format;
  gettimeofday(&s_fb.timestamp, NULL);
  return &s_fb;
}

void esp_camera_fb_return(camera_fb_t* fb) { (void)fb; }
sensor_t* esp_camera_sensor_get(void) { return s_inited ? &s_sensor : NULL; }
camera_sensor_info_t* esp_camera_sensor_get_info(sensor_id_t* id) { return id && id->PID == OV3660_PID ? &k_ov3660_info : NULL; }
esp_err_t esp_camera_save_to_nvs(const char* key) { (void)key; return ESP_OK; }
esp_err_t esp_camera_load_from_nvs(const char* key) { (void)key; return ESP_OK; }
void esp_camera_return_all(void) {}
esp_err_t esp_camera_set_psram_mode(bool on) { s_psram_mode = on; return ESP_OK; }
bool esp_camera_get_psram_mode(void) { return s_psram_mode; }
bool esp_camera_available_frames(void) { return true; }

bool frame2jpg(camera_fb_t* fb, uint8_t q, uint8_t** out, size_t* len) { (void)fb; (void)q; (void)out; (void)len; return false; }
bool fmt2jpg(uint8_t* s, size_t sl, uint16_t w, uint16_t h, pixformat_t f, uint8_t q, uint8_t** o, size_t* ol) {
  (void)s; (void)sl; (void)w; (void)h; (void)f; (void)q; (void)o; (void)ol;
  return false;
}
bool fmt2rgb888(const uint8_t* s, size_t sl, pixformat_t f, uint8_t* rgb) { (void)s; (void)sl; (void)f; (void)rgb; return false; }
