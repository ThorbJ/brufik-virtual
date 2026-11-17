/* esp32-camera 的 sensor.h（子集）：OV3660 传感器描述。虚拟摄像头的寄存器设置全部"成功"但不生效。 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum { OV9650_PID = 0x96, OV7725_PID = 0x77, OV2640_PID = 0x26, OV3660_PID = 0x3660, OV5640_PID = 0x5640, OV7670_PID = 0x76 } camera_pid_t;
typedef enum { CAMERA_OV7725, CAMERA_OV2640, CAMERA_OV3660, CAMERA_OV5640, CAMERA_OV7670, CAMERA_MODEL_MAX, CAMERA_NONE } camera_model_t;
typedef enum { PIXFORMAT_RGB565, PIXFORMAT_YUV422, PIXFORMAT_YUV420, PIXFORMAT_GRAYSCALE, PIXFORMAT_JPEG, PIXFORMAT_RGB888, PIXFORMAT_RAW, PIXFORMAT_RGB444, PIXFORMAT_RGB555 } pixformat_t;
typedef enum {
  FRAMESIZE_96X96, FRAMESIZE_QQVGA, FRAMESIZE_128X128, FRAMESIZE_QCIF, FRAMESIZE_HQVGA, FRAMESIZE_240X240, FRAMESIZE_QVGA,
  FRAMESIZE_320X320, FRAMESIZE_CIF, FRAMESIZE_HVGA, FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA, FRAMESIZE_HD, FRAMESIZE_SXGA,
  FRAMESIZE_UXGA, FRAMESIZE_FHD, FRAMESIZE_P_HD, FRAMESIZE_P_3MP, FRAMESIZE_QXGA, FRAMESIZE_QHD, FRAMESIZE_WQXGA,
  FRAMESIZE_P_FHD, FRAMESIZE_QSXGA, FRAMESIZE_5MP, FRAMESIZE_INVALID
} framesize_t;
typedef struct { const camera_model_t model; const char* name; const uint16_t sccb_addr; const uint16_t pid; const framesize_t max_size; const bool support_jpeg; } camera_sensor_info_t;
typedef enum { ASPECT_RATIO_4X3, ASPECT_RATIO_3X2, ASPECT_RATIO_16X10, ASPECT_RATIO_5X3, ASPECT_RATIO_16X9, ASPECT_RATIO_21X9, ASPECT_RATIO_5X4, ASPECT_RATIO_1X1, ASPECT_RATIO_9X16 } aspect_ratio_t;
typedef enum { GAINCEILING_2X, GAINCEILING_4X, GAINCEILING_8X, GAINCEILING_16X, GAINCEILING_32X, GAINCEILING_64X, GAINCEILING_128X } gainceiling_t;
typedef struct { uint16_t width; uint16_t height; aspect_ratio_t aspect_ratio; } resolution_info_t;
extern const resolution_info_t resolution[];
extern const camera_sensor_info_t camera_sensor[];

typedef struct { uint8_t MIDH; uint8_t MIDL; uint16_t PID; uint8_t VER; } sensor_id_t;
typedef struct {
  framesize_t framesize;
  bool scale, binning;
  uint8_t quality;
  int8_t brightness, contrast, saturation, sharpness;
  uint8_t denoise, special_effect, wb_mode, awb, awb_gain, aec, aec2;
  int8_t ae_level;
  uint16_t aec_value;
  uint8_t agc, agc_gain, gainceiling, bpc, wpc, raw_gma, lenc, hmirror, vflip, dcw, colorbar;
} camera_status_t;

typedef struct _sensor sensor_t;
typedef struct _sensor {
  sensor_id_t id;
  uint8_t slv_addr;
  pixformat_t pixformat;
  camera_status_t status;
  int xclk_freq_hz;
  int (*init_status)(sensor_t* sensor);
  int (*reset)(sensor_t* sensor);
  int (*set_pixformat)(sensor_t* sensor, pixformat_t pixformat);
  int (*set_framesize)(sensor_t* sensor, framesize_t framesize);
  int (*set_contrast)(sensor_t* sensor, int level);
  int (*set_brightness)(sensor_t* sensor, int level);
  int (*set_saturation)(sensor_t* sensor, int level);
  int (*set_sharpness)(sensor_t* sensor, int level);
  int (*set_denoise)(sensor_t* sensor, int level);
  int (*set_gainceiling)(sensor_t* sensor, gainceiling_t gainceiling);
  int (*set_quality)(sensor_t* sensor, int quality);
  int (*set_colorbar)(sensor_t* sensor, int enable);
  int (*set_whitebal)(sensor_t* sensor, int enable);
  int (*set_gain_ctrl)(sensor_t* sensor, int enable);
  int (*set_exposure_ctrl)(sensor_t* sensor, int enable);
  int (*set_hmirror)(sensor_t* sensor, int enable);
  int (*set_vflip)(sensor_t* sensor, int enable);
  int (*set_aec2)(sensor_t* sensor, int enable);
  int (*set_awb_gain)(sensor_t* sensor, int enable);
  int (*set_agc_gain)(sensor_t* sensor, int gain);
  int (*set_aec_value)(sensor_t* sensor, int gain);
  int (*set_special_effect)(sensor_t* sensor, int effect);
  int (*set_wb_mode)(sensor_t* sensor, int mode);
  int (*set_ae_level)(sensor_t* sensor, int level);
  int (*set_dcw)(sensor_t* sensor, int enable);
  int (*set_bpc)(sensor_t* sensor, int enable);
  int (*set_wpc)(sensor_t* sensor, int enable);
  int (*set_raw_gma)(sensor_t* sensor, int enable);
  int (*set_lenc)(sensor_t* sensor, int enable);
  int (*get_reg)(sensor_t* sensor, int reg, int mask);
  int (*set_reg)(sensor_t* sensor, int reg, int mask, int value);
  int (*set_res_raw)(sensor_t* sensor, int startX, int startY, int endX, int endY, int offsetX, int offsetY, int totalX, int totalY, int outputX, int outputY, bool scale, bool binning);
  int (*set_pll)(sensor_t* sensor, int bypass, int mul, int sys, int root, int pre, int seld5, int pclken, int pclk);
  int (*set_xclk)(sensor_t* sensor, int timer, int xclk);
} sensor_t;

camera_sensor_info_t* esp_camera_sensor_get_info(sensor_id_t* id);

#ifdef __cplusplus
}
#endif
