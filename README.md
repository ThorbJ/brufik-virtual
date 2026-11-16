# brufik-virtual

Brufik ([OpenDeskBotV2](https://github.com/ai-maker-lab/OpenDeskBotV2)) running its real firmware in the browser, on its 3D model, built on [esp32-virtual-hardware](https://github.com/ThorbJ/esp32-virtual-hardware).

中文：小歪（Brufik）的真固件在浏览器里运行。上游固件按固定提交原样编译，本仓库只放小歪专用的虚拟硬件和编译脚本。

## How it works

- The upstream firmware is built as it is, from a pinned commit (`1cf5050ee63b1fe4cae6b7815e565a0955f15bd6`); it is fetched by the build, not copied here.
- Device layer: ST7789P 240×284 screen, two servos (LEDC duty → angle), OV3660 camera (frames from the computer's camera), PDM microphone and I2S speaker, a stand-in for ESP-SR (voice activity detection with an open-source implementation).
- The brain stays where it is with the real Brufik: the upstream computer-side service, run by you on your computer with your own AI keys. The virtual Brufik connects to it like a real one. Until the upstream service accepts a virtual device connection, use [ThorbJ/OpenDeskBotV2](https://github.com/ThorbJ/OpenDeskBotV2), a fork that adds a local WebSocket transport beside USB.

## Status

Being moved here from Thorb's working experiment, where boot, conversation, expressions, servos and the camera run in desktop Chrome.

## License

GPL-3.0, the license of the upstream firmware it is built with. Not an official OpenDeskBot release.
