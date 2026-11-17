# brufik-virtual

Brufik ([OpenDeskBotV2](https://github.com/ai-maker-lab/OpenDeskBotV2)) running its real firmware in the browser, on its 3D model, built on [esp32-virtual-hardware](https://github.com/ThorbJ/esp32-virtual-hardware).

中文：小歪（Brufik）的真固件在浏览器里运行。上游固件按固定提交原样编译，本仓库只放小歪专用的虚拟硬件和编译脚本。

## How it works

- The upstream firmware is built as it is, from a pinned commit; the build fetches it, nothing of it is copied here.
- Device layer (`vhw/`): the V2 main board's wiring (ST7789P 240×284 screen on SPI, two servos on LEDC, USB Serial/JTAG FIFO), servo duty → angle, a virtual OV3660 camera (frames from the computer's camera), a stand-in for the closed ESP-SR library (voice activity detection with the open-source libfvad) and a board without Wi-Fi. Everything shared with other devices comes from esp32-virtual-hardware, pinned at `v0.2.0` (`407ef35`).
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

The script fetches the upstream sources at their pinned commits into `.upstream/` (`BRUFIK_UPSTREAM_CACHE=<dir>` for another folder), compiles about 200 files (about a minute the first time, incremental after), links the firmware module and packs it with its licences and the device description (`device.template.json`, format 2 of esp32-virtual-hardware). Before writing the package it checks the description against the current Brufik Kit model (every node it names, the paths to unnamed parts and the screen face) (`kit-model.json`: downloaded once into `.cache/`, pinned by sha256; `BRUFIK_KIT_GLB=<file>` for a local copy). Built firmware and packages are never committed.

## Tests

```sh
npm test    # about a minute, no browser
```

Through the virtual device interface only, from the firmware inside the built package (`BRUFIK_PACKAGE=<file>` for another one): the screen is not black after power on; the same inputs give the same frames twice; a power cycle boots again; the computer's `hello` gets a DBOT `hello_ack`; an expression + audio PB changes the screen and plays the tone on the speaker (chunks carry the device time); a motion PB moves both servos; a camera request sends the given JPEG up. The package itself: passes the device description validator, its files match their list, it fits the Kit model (nodes, paths, screen face), it names the pinned upstream and this repository, and the firmware holds no local folder paths. On the real Kit model, through the shared three.js viewer code in Node (`tests/kit-model.test.mjs`, dev dependency `three`): a test picture lands upright on the screen face seen from the front, and the head tilt rides on the head turn.

## Upstream patches

**None.** All 27 upstream firmware files and the libraries are compiled as they are. What the build does instead:

- Device headers and implementations come first on the include path; the portable Arduino core files (String / Print / Stream) are copied unchanged into `build/` so their `#include "Arduino.h"` finds the virtual Arduino layer.
- `version_gen.h` is generated from `hardware/VERSION` with the same content as upstream's PlatformIO pre-script.
- Link-time `--wrap`: `esp_restart` (upstream's own link option) and `usb_transport_send_log` (a copy of the firmware's log frames goes to the interface's log channel; the original is still called).
- Closed library replaced: ESP-SR → a stand-in with the same interface (`vhw/src/esp_afe_vhw.c`; echo cancellation and noise suppression are left to the browser's microphone, VAD by libfvad).

## Device description and approximations

- Kit `kit_brufik_v2`, default configuration (no hardware-changing options).
- Screen: node `lcd`; the face is the unnamed textured mesh `lcd` → `lcd-module` → child 17 → child 0 (`face.path [0, 17, 0]`, the only mesh under `lcd` with texture coordinates). Panel 240×284 RGB565 in its native scan direction, turned 270° clockwise (firmware rotation 3) to the 284×240 picture people see, filling the width of the 39 × 34 mm face. The face's texture coordinates run bottom-up (`v` grows upward; `u` runs to the viewer's right, model −X), so the picture is mirrored top-to-bottom (`flipY`). Checked by rendering a firmware frame (the "请先连接PC服务" screen) onto the face from the GLB's own vertex → texture mapping and in Chromium with the reference viewer: upright, not mirrored, eyes above the mouth.
- Servo X (id 0, 3.7 g servo in the body, through 30/36 gears): turns the neck, the 9 g servo and the U-bracket (`internal-pack` child 7, print-13) about the vertical neck axis, and with them the head (servo Y rides on it). **Approximation**: the servo uses 1000–2000 µs, about half its travel, so the head turns about 0.42° per servo degree, about ±29° at the firmware's limits; the real servo travel and gear teeth are not measured on a real Brufik.
- Servo Y (id 1, 9 g servo in the head, head tilt; `parent` 0): tilts the head shells, screen, screen board, head screws and the head frame parts (`internal-pack` children 8–11, print-14…17) about the 9 g servo's output shaft (the horn's origin in the Kit model, (15, 87.11, −2) mm, axis +X). Upstream drives it over the full 500–2500 µs travel, so the joint follows the logical angle 1:1: 110° (upper limit) looks up 20°, 78° (lower limit) looks down 12° (upstream `head.h`: logical 70 = "head fully down"). **Approximation**: not measured on a real Brufik; which internal parts belong to the neck and which to the head is read from where they sit in the Kit model.
- Computer connection: port 9022, path `/virtual-device`, protocol `DBOT v1`. Channels in: mic (16 kHz), camera, hostRx, hostPresent; out: screen, servo, speaker, hostTx, log.
- Runtime: 512 MB, 28 threads, virtual clock (deterministic).

## Known gaps

- **Gears**: the servo's small gear (`internal-pack` child 5, print-8) turns about the servo's own axis, not the neck's; it and the other body parts stay still. They sit inside the body and are not seen.
- **The 9 g servo** turns with the neck but does not tilt with the head (whether its body sits in the head or on the bracket is not known from the Kit model); it is inside the head shells.

## License

GPL-3.0 (GPL-3.0-only), the license of the upstream firmware it is built with. The package ships this licence, upstream's licence files and those of every compiled library. Not an official OpenDeskBot release.
