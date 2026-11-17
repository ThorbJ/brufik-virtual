/*
 * esp32-camera 驱动 → 接口的摄像头通道。
 * esp_camera_fb_get() 向外界要一帧（按固件配置的分辨率和格式，JPEG），外界没给就像真驱动超时一样返回 NULL。
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>
#include "esp_err.h"
#include "driver/ledc.h"
#include "sensor.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { CAMERA_GRAB_WHEN_EMPTY, CAMERA_GRAB_LATEST } camera_grab_mode_t;
typedef enum { CAMERA_FB_IN_PSRAM, CAMERA_FB_IN_DRAM } camera_fb_location_t;
typedef enum { CONV_DISABLE, RGB_TO_YUV, YUV_TO_RGB, RGB_TO_YUV_HIGH_BW } camera_conv_mode_t;

typedef struct {
  int pin_pwdn;
  int pin_reset;
  int pin_xclk;
  union { int pin_sccb_sda; int pin_sscb_sda; };
  union { int pin_sccb_scl; int pin_sscb_scl; };
  int pin_d7, pin_d6, pin_d5, pin_d4, pin_d3, pin_d2, pin_d1, pin_d0;
  int pin_vsync;
  int pin_href;
  int pin_pclk;
  int xclk_freq_hz;
  ledc_timer_t ledc_timer;
  ledc_channel_t ledc_channel;
  pixformat_t pixel_format;
  framesize_t frame_size;
  int jpeg_quality;
  size_t fb_count;
  camera_fb_location_t fb_location;
  camera_grab_mode_t grab_mode;
  int sccb_i2c_port;
} camera_config_t;

typedef struct {
  uint8_t* buf;
  size_t len;
  size_t width;
  size_t height;
  pixformat_t format;
  struct timeval timestamp;
} camera_fb_t;

#define ESP_ERR_CAMERA_BASE 0x20000
#define ESP_ERR_CAMERA_NOT_DETECTED (ESP_ERR_CAMERA_BASE + 1)
#define ESP_ERR_CAMERA_FAILED_TO_SET_FRAME_SIZE (ESP_ERR_CAMERA_BASE + 2)
#define ESP_ERR_CAMERA_FAILED_TO_SET_OUT_FORMAT (ESP_ERR_CAMERA_BASE + 3)
#define ESP_ERR_CAMERA_NOT_SUPPORTED (ESP_ERR_CAMERA_BASE + 4)

esp_err_t esp_camera_init(const camera_config_t* config);
esp_err_t esp_camera_deinit(void);
camera_fb_t* esp_camera_fb_get(void);
void esp_camera_fb_return(camera_fb_t* fb);
sensor_t* esp_camera_sensor_get(void);
esp_err_t esp_camera_save_to_nvs(const char* key);
esp_err_t esp_camera_load_from_nvs(const char* key);
void esp_camera_return_all(void);
esp_err_t esp_camera_set_psram_mode(bool on);
bool esp_camera_get_psram_mode(void);
bool esp_camera_available_frames(void);

#ifdef __cplusplus
}
#endif
#include "img_converters.h"
