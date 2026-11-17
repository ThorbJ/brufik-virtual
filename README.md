# brufik-virtual

Brufik ([OpenDeskBotV2](https://github.com/ai-maker-lab/OpenDeskBotV2)) running its real firmware in the browser, on its 3D model, built on [esp32-virtual-hardware](https://github.com/ThorbJ/esp32-virtual-hardware).

中文：小歪（Brufik）的真固件在浏览器里运行。上游固件按固定提交原样编译，本仓库只放小歪专用的虚拟硬件和编译脚本。

## How it works

- The upstream firmware is built as it is, from a pinned commit; the build fetches it, nothing of it is copied here.
- Device layer (`vhw/`): the V2 main board's wiring (ST7789P 240×284 screen on SPI, two servos on LEDC, USB Serial/JTAG FIFO), servo duty → angle, a virtual OV3660 camera (frames from the computer's camera), a stand-in for the closed ESP-SR library (voice activity detection with the open-source libfvad) and a board without Wi-Fi. Everything shared with other devices comes from esp32-virtual-hardware, pinned at `v0.1.0` (`f78e31a`).
- The brain stays where it is with the real Brufik: the upstream computer-side service, run by you on your computer with your own AI keys. The virtual Brufik connects to it like a real one over `ws://127.0.0.1:9022/virtual-device` (DBOT v1 bytes as they are). Until the upstream service accepts a virtual device connection, use [ThorbJ/OpenDeskBotV2](https://github.com/ThorbJ/OpenDeskBotV2), a fork that adds this local WebSocket transport beside USB.

## Firmware version

| | |
|---|---|
| Upstream | [ai-maker-lab/OpenDeskBotV2](https://github.com/ai-maker-lab/OpenDeskBotV2) at [`1cf5050ee63b1fe4cae6b7815e565a0955f15bd6`](https://github.com/ai-maker-lab/OpenDeskBotV2/tree/1cf5050ee63b1fe4cae6b7815e565a0955f15bd6) |
| Firmware | `hardware/firmware`, `hardware/VERSION` **0.0.59**, PlatformIO env `deskbot_v2` |
| Toolchain it pins | pioarduino 55.03.32 = Arduino-ESP32 3.3.2; libraries as in `platformio.ini` `lib_deps` (all in `upstream.lock.json`) |

Why this commit: it is the version a Brufik built from Thorb's Brufik V2 kit runs. Thorb does not flash Brufik kits (the builder flashes the board following the upstream instructions); Thorb's kit records verified and pinned the upstream software at this commit, and the kit's published licence and software links point at it. When the kit moves to a newer upstream version, change the commit in `upstream.lock.json`, rebuild and upload the new package.

## Build the package (one command)

Needs Node 22+, git and Emscripten (`brew install emscripten`; tested with emcc 7.0.0).

```sh
npm install
npm run package                  # → build/brufik.vdpkg.tgz (about 0.5 MB)
npm run package -- --out=<dir>   # write it elsewhere
npm run package -- --dev         # allow uncommitted changes (the package records HEAD anyway: not for upload)
```

The script fetches the upstream sources at their pinned commits into `.upstream/` (`BRUFIK_UPSTREAM_CACHE=<dir>` for another folder), compiles about 200 files (about a minute the first time, incremental after), links the firmware module and packs it with its licences and the device description (`device.template.json`, format v1 of esp32-virtual-hardware). Before writing the package it checks that every node the description names is in the current Brufik Kit model (`kit-model.json`: downloaded once into `.cache/`, pinned by sha256; `BRUFIK_KIT_GLB=<file>` for a local copy). Built firmware and packages are never committed.

## Tests

```sh
npm test    # about a minute, no browser
```

Through the virtual device interface only, from the firmware inside the built package (`BRUFIK_PACKAGE=<file>` for another one): the screen is not black after power on; the same inputs give the same frames twice; a power cycle boots again; the computer's `hello` gets a DBOT `hello_ack`; an expression + audio PB changes the screen and plays the tone on the speaker; a motion PB moves both servos; a camera request sends the given JPEG up. The package itself: passes the device description validator, its files match their list, its nodes are in the Kit model, it names the pinned upstream and this repository, and the firmware holds no local folder paths.

## Upstream patches

**None.** All 27 upstream firmware files and the libraries are compiled as they are. What the build does instead:

- Device headers and implementations come first on the include path; the portable Arduino core files (String / Print / Stream) are copied unchanged into `build/` so their `#include "Arduino.h"` finds the virtual Arduino layer.
- `version_gen.h` is generated from `hardware/VERSION` with the same content as upstream's PlatformIO pre-script.
- Link-time `--wrap`: `esp_restart` (upstream's own link option) and `usb_transport_send_log` (a copy of the firmware's log frames goes to the interface's log channel; the original is still called).
- Closed library replaced: ESP-SR → a stand-in with the same interface (`vhw/src/esp_afe_vhw.c`; echo cancellation and noise suppression are left to the browser's microphone, VAD by libfvad).

## Device description and approximations

- Kit `kit_brufik_v2`, default configuration (no hardware-changing options).
- Screen: node `lcd`, panel 240×284 RGB565 in its native scan direction, turned 270° clockwise (firmware rotation 3) to the 284×240 picture people see, filling the width of the 39 × 34 mm face.
- Servo X (id 0, 3.7 g servo in the body, through 30/36 gears): turns the head, neck and the parts in the head about the vertical neck axis. **Approximation**: the servo uses 1000–2000 µs, about half its travel, so the head turns about 0.42° per servo degree, about ±29° at the firmware's limits; the real servo travel and gear teeth are not measured on a real Brufik.
- Servo Y (id 1, 9 g servo in the head, head tilt): **not shown yet** (see Known gaps). The firmware still drives it and the interface still reports it.
- Computer connection: port 9022, path `/virtual-device`, protocol `DBOT v1`. Channels in: mic (16 kHz), camera, hostRx, hostPresent; out: screen, servo, speaker, hostTx, log.
- Runtime: 512 MB, 28 threads, virtual clock (deterministic).

## Known gaps (need esp32-virtual-hardware or the Kit model)

- **Head tilt on top of the head turn**: format v1 turns each servo's nodes from their place in the Kit model, so one node cannot follow two servos; the 9 g servo rides on the neck the 3.7 g servo turns. Needs a way to mount one servo on another (format change in esp32-virtual-hardware).
- **Unnamed parts in the Kit model**: the 13 printed parts under `internal-pack` (gears, U-bracket, head frame) and the screen face mesh under `lcd` have no node names, so the description cannot point at them: the internal parts stay still and the viewer has to find the screen face itself (the first mesh under `lcd` is not the face). Needs names in the Kit model.
- **Screen face texture coordinates**: the face's texture coordinates run bottom-up (glTF `v` grows upward on the face), so the picture would show mirrored top-to-bottom; format v1 can only turn the picture, not mirror it. Needs either a Kit model with the usual texture direction or a mirror option in the format.

## License

GPL-3.0 (GPL-3.0-only), the license of the upstream firmware it is built with. The package ships this licence, upstream's licence files and those of every compiled library. Not an official OpenDeskBot release.
