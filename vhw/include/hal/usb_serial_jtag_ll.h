/*
 * USB Serial/JTAG 外设的底层 FIFO 访问（小歪的协议帧绕过 HWCDC 直接写 FIFO）→ 接口的 hostTx 字节流。
 * FIFO 永远可写（背压由接口环形缓冲负责）。
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool usb_serial_jtag_ll_txfifo_writable(void);
uint32_t usb_serial_jtag_ll_write_txfifo(const uint8_t* buf, uint32_t wr_len);
void usb_serial_jtag_ll_txfifo_flush(void);
bool usb_serial_jtag_ll_rxfifo_data_available(void);
uint32_t usb_serial_jtag_ll_read_rxfifo(uint8_t* buf, uint32_t rd_len);
#ifdef __cplusplus
}
#endif
