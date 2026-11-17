/* lwIP 的 BSD 套接字头 → emscripten 自带的 POSIX 头。Wi-Fi 路径只需编译通过，虚拟设备不会走到。 */
#pragma once
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
/* lwIP 独有 */
static inline char* inet_ntoa_r(struct in_addr addr, char* buf, int buflen) {
  return (char*)inet_ntop(AF_INET, &addr, buf, (socklen_t)buflen);
}
