/*
 * 小歪 V2 主板的虚拟接线（引脚号来自上游 hardware/firmware/deskbot_config.h，DESKBOT_BOARD_V2）：
 *   ST7789P 屏：SPI（FSPI），CS = GPIO6，DC = GPIO7，GRAM 240×320，露出 240×284（行偏移 0），IPS 需 INVON；
 *   舵机：LEDC 通道 6（X / 水平，GPIO16）、通道 7（Y / 俯仰，GPIO15），50 Hz；
 *   USB Serial/JTAG：协议帧直接写 FIFO → 接口 hostTx。
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hal/usb_serial_jtag_ll.h"
#include "vdev_io.h"
#include "vhw_panel_st7789.h"

void vhw_board_init(void) {
  static const vhw_st7789_config_t panel = {
      .spi_bus = 0,
      .cs_pin = 6,
      .dc_pin = 7,
      .gram_width = 240,
      .gram_height = 320,
      .visible_x = 0,
      .visible_y = 0,
      .visible_width = 240,
      .visible_height = 284,
      .ips_inverted = 1,
  };
  vhw_st7789_attach(&panel);
}

/* ---------- 舵机：占空比 → 角度（与上游 head.cpp 的脉宽映射互逆） ---------- */

#define SERVO_X_CHANNEL 6
#define SERVO_Y_CHANNEL 7

void vhw_ledc_on_duty(int channel, int gpio, uint32_t duty, uint32_t bits, uint32_t freq_hz) {
  (void)gpio;
  if (channel != SERVO_X_CHANNEL && channel != SERVO_Y_CHANNEL) return;
  if (freq_hz == 0 || bits == 0) return;
  const double period_us = 1e6 / (double)freq_hz;
  const uint32_t pulse_us = (uint32_t)((double)duty * period_us / (double)(1u << bits) + 0.5);
  if (duty == 0) {
    /* 占空比 0 = 舵机释放（保持最后角度），只报脉宽 0 */
    vdev_servo_emit(channel == SERVO_X_CHANNEL ? 0 : 1, 0, 0);
    return;
  }
  double deg;
  if (channel == SERVO_X_CHANNEL) {
    deg = ((double)pulse_us - 1000.0) / 1000.0 * 180.0; /* X：1000–2000 µs ↔ 0–180° */
  } else {
    double pwm = ((double)pulse_us - 500.0) / 2000.0 * 180.0; /* Y：500–2500 µs ↔ 0–180°，固件里镜像 */
    deg = 180.0 - pwm;                                          /* 换回固件的逻辑角度 */
  }
  vdev_servo_emit(channel == SERVO_X_CHANNEL ? 0 : 1, (int32_t)(deg * 1000.0), pulse_us);
}

/* ---------- USB Serial/JTAG FIFO ---------- */

bool usb_serial_jtag_ll_txfifo_writable(void) { return true; }
uint32_t usb_serial_jtag_ll_write_txfifo(const uint8_t* buf, uint32_t len) {
  vdev_host_write(buf, len, 100);
  return len;
}
void usb_serial_jtag_ll_txfifo_flush(void) {}
bool usb_serial_jtag_ll_rxfifo_data_available(void) { return vdev_ring_readable(VDEV_RING_HOST_RX) > 0; }
uint32_t usb_serial_jtag_ll_read_rxfifo(uint8_t* buf, uint32_t len) { return (uint32_t)vdev_ring_read(VDEV_RING_HOST_RX, buf, len); }

/* ---------- 日志：握手后固件把日志打成 DBOT LOG 帧发给电脑端；抄一份到接口日志通道 ---------- */

/* 上游是 C++ 函数 bool usb_transport_send_log(const char*, size_t)，链接名为 _Z22usb_transport_send_logPKcm；
 * build.mjs 用 -Wl,--wrap=_Z22usb_transport_send_logPKcm 让上游的调用先经过这里。 */
bool __real__Z22usb_transport_send_logPKcm(const char* text, size_t length);
bool __wrap__Z22usb_transport_send_logPKcm(const char* text, size_t length) {
  vdev_log_write(text, length);
  return __real__Z22usb_transport_send_logPKcm(text, length);
}
