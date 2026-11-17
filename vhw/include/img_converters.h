/* esp32-camera 图像转换（子集）。虚拟摄像头直接给 JPEG，RGB565→JPEG 软编码只在诊断构建用到，这里返回失败。 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_camera.h"
#ifdef __cplusplus
extern "C" {
#endif
bool frame2jpg(camera_fb_t* fb, uint8_t quality, uint8_t** out, size_t* out_len);
bool fmt2jpg(uint8_t* src, size_t src_len, uint16_t width, uint16_t height, pixformat_t format, uint8_t quality, uint8_t** out, size_t* out_len);
bool fmt2rgb888(const uint8_t* src_buf, size_t src_len, pixformat_t format, uint8_t* rgb_buf);
#ifdef __cplusplus
}
#endif
