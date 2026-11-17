#!/usr/bin/env node
// One command from the pinned upstream source to the uploadable virtual device package:
//   1. fetch the upstream sources at their pinned commits (upstream.lock.json → .upstream/, never modified);
//   2. compile the OpenDeskBotV2 firmware (PlatformIO env deskbot_v2) as it is, with the libraries it pins, on the shared
//      virtual hardware (esp32-virtual-hardware) and Brufik's own device layer (vhw/);
//   3. pack firmware + licences + device description (device.template.json) into build/brufik.vdpkg.tgz, checking that every
//      node the description names is in the current Brufik Kit model (kit-model.json).
//
//   npm run package                  build and pack (refuses uncommitted changes: the package records this repository's commit)
//   npm run package -- --dev         allow uncommitted changes (the package still records HEAD: do not upload it)
//   npm run package -- --out=<dir>   write the package there instead of build/
//   npm run package -- --force       recompile everything
//   BRUFIK_UPSTREAM_CACHE=<dir>      fetch / reuse the upstream sources there instead of .upstream/
import { execFileSync } from 'node:child_process';
import { copyFileSync, existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { checkNodesInModel } from 'esp32-virtual-hardware/device-description';
import { buildPackage } from 'esp32-virtual-hardware/package';
import { compileAll, link } from 'esp32-virtual-hardware/tools/emcc-build.mjs';
import { firmwareLinkFlags, vhwCompileFlags, vhwUnits } from 'esp32-virtual-hardware/tools/firmware.mjs';
import { openUpstream } from 'esp32-virtual-hardware/tools/upstream.mjs';
import { kitModel } from './kit-model.mjs';

const args = process.argv.slice(2);
const flag = (name) => args.includes(`--${name}`);
const option = (name) => args.find((a) => a.startsWith(`--${name}=`))?.slice(name.length + 3);
const repo = fileURLToPath(new URL('./', import.meta.url)).replace(/\/$/, '');
const out = path.join(repo, 'build/brufik');
const packageDir = path.resolve(option('out') ?? path.join(repo, 'build'));
const git = (...a) => execFileSync('git', a, { cwd: repo }).toString().trim();
const template = JSON.parse(readFileSync(path.join(repo, 'device.template.json'), 'utf8'));

if (!flag('dev')) {
  const dirty = git('status', '--porcelain');
  if (dirty) throw new Error(`uncommitted changes; commit them first (or --dev):\n${dirty}`);
}

// ---------- 1. upstream sources ----------
const upstream = openUpstream({ root: repo, cacheEnv: 'BRUFIK_UPSTREAM_CACHE' });
console.log('upstream sources');
for (const name of upstream.sourcesFor('brufik')) upstream.ensure(name);
const cacheRoot = path.dirname(upstream.path('OpenDeskBotV2'));
const hw = path.join(upstream.path('OpenDeskBotV2'), 'hardware');
const fw = path.join(hw, 'firmware');
const core = path.join(upstream.path('arduino-esp32'), 'cores/esp32');
const arduinoLib = (n) => path.join(upstream.path('arduino-esp32'), 'libraries', n, 'src');
const lib = (n) => upstream.path(`libs/${n}`);
const version = readFileSync(path.join(hw, 'VERSION'), 'utf8').trim();

// ---------- 2. firmware ----------
// The header upstream's PlatformIO pre-script writes (hardware/scripts/gen_version.py → version_gen.h), same content.
const gen = path.join(out, 'gen');
mkdirSync(gen, { recursive: true });
const versionHeader = `// Generated from hardware/VERSION (as upstream gen_version.py does).\n#pragma once\n#define DESKBOT_VERSION "${version}"\n`;
if (!existsSync(path.join(gen, 'version_gen.h')) || readFileSync(path.join(gen, 'version_gen.h'), 'utf8') !== versionHeader) {
  writeFileSync(path.join(gen, 'version_gen.h'), versionHeader);
}
// The portable part of the Arduino core (String / Print / Stream), copied unchanged into the build folder: in its own folder a
// core file's #include "Arduino.h" would find the real Arduino.h (all of ESP-IDF); next to the copies it finds the virtual one.
const coreCopy = path.join(out, 'arduino-core');
mkdirSync(coreCopy, { recursive: true });
const CORE_FILES = ['WString.cpp', 'WString.h', 'Print.cpp', 'Print.h', 'Printable.h', 'Stream.cpp', 'Stream.h', 'StreamString.cpp',
  'StreamString.h', 'stdlib_noniso.c', 'stdlib_noniso.h', 'WCharacter.h', 'binary.h', 'pgmspace.h', 'WMath.cpp', 'esp_arduino_version.h',
  'wiring_private.h'];
for (const f of CORE_FILES) {
  const from = readFileSync(path.join(core, f));
  const to = path.join(coreCopy, f);
  if (!existsSync(to) || !readFileSync(to).equals(from)) copyFileSync(path.join(core, f), to);
}

const flags = vhwCompileFlags({
  arduino: true,
  virtualTime: template.runtime.virtualTime,
  // fixed virtual MAC (locally administered) → device id deskbot_02464d534e01
  mac: '02:46:4d:53:4e:01',
  includes: [path.join(repo, 'vhw/include')],
  prefixMap: { [repo]: 'brufik-virtual', [cacheRoot]: 'upstream', [out]: 'build' },
  defines: [
    // upstream platformio.ini [env:deskbot_v2]
    'BOARD_HAS_PSRAM', 'ARDUINO_USB_MODE=1', 'ARDUINO_USB_CDC_ON_BOOT=1', 'DESKBOT_BOARD_V2=1',
    'DESKBOT_CAMERA_HMIRROR=1', 'DESKBOT_CAMERA_VFLIP=1', 'DESKBOT_CAMERA_FB_COUNT=2', 'DESKBOT_CAMERA_GRAB_LATEST=1',
    'DESKBOT_CAMERA_PSRAM_DMA=1', 'DESKBOT_CAMERA_UPLINK_INTERVAL_DURING_LISTEN_MS=500',
    // what PlatformIO adds for Arduino-ESP32 3.3.2 on an ESP32-S3
    'ARDUINO=10812', 'ESP32=ESP32', 'ARDUINO_ARCH_ESP32', 'ESP_PLATFORM', 'CONFIG_IDF_TARGET_ESP32S3=1',
    'ARDUINO_ESP32S3_DEV', 'ARDUINO_BOARD="ESP32S3_DEV"', 'ARDUINO_VARIANT="esp32s3"',
    // third-party libraries: no S3 assembly / SIMD
    'NO_SIMD',
  ],
});
// After the shared headers (a shared header wins over these): generated header, core copies, libraries.
const after = [gen, coreCopy, arduinoLib('Preferences'), arduinoLib('FS'),
  lib('Adafruit-GFX-Library'), lib('Adafruit-ST7735-Library'), lib('Adafruit_BusIO'), path.join(lib('ArduinoJson'), 'src'),
  path.join(lib('JPEGDEC'), 'src'), path.join(lib('U8g2_for_Adafruit_GFX'), 'src'), path.join(lib('esp32_opus_arduino'), 'src'),
  path.join(lib('libfvad'), 'include')].map((i) => `-I${i}`);
const cc = [...flags.c, ...after];
const cxx = [...flags.cxx, ...after];

const units = [];
const add = (src, lang) => units.push({ src, lang, flags: lang === 'c' ? cc : cxx });
const addDir = (dir) => {
  for (const f of readdirSync(dir).sort()) {
    if (/\.(cpp|ino)$/.test(f)) add(path.join(dir, f), 'c++');
    else if (/\.c$/.test(f)) add(path.join(dir, f), 'c');
  }
};
addDir(fw); // the upstream firmware: every source file, as it is
addDir(coreCopy);
add(path.join(arduinoLib('Preferences'), 'Preferences.cpp'), 'c++');
add(path.join(arduinoLib('FS'), 'FS.cpp'), 'c++');
add(path.join(arduinoLib('FS'), 'vfs_api.cpp'), 'c++');
// third-party libraries at the versions platformio.ini lib_deps pins
for (const f of ['Adafruit_GFX.cpp', 'Adafruit_SPITFT.cpp', 'glcdfont.c']) add(path.join(lib('Adafruit-GFX-Library'), f), /\.c$/.test(f) ? 'c' : 'c++');
for (const f of ['Adafruit_ST77xx.cpp', 'Adafruit_ST7789.cpp']) add(path.join(lib('Adafruit-ST7735-Library'), f), 'c++');
add(path.join(lib('JPEGDEC'), 'src/JPEGDEC.cpp'), 'c++');
add(path.join(lib('U8g2_for_Adafruit_GFX'), 'src/U8g2_for_Adafruit_GFX.cpp'), 'c++');
add(path.join(lib('U8g2_for_Adafruit_GFX'), 'src/u8g2_fonts.c'), 'c');
addDir(path.join(lib('esp32_opus_arduino'), 'src'));
for (const d of ['src', 'src/signal_processing', 'src/vad']) addDir(path.join(lib('libfvad'), d)); // for the ESP-SR stand-in
// virtual hardware: shared (core, LEDC, I2S, ST7789, Arduino layer) and Brufik's own
for (const u of vhwUnits({ peripherals: ['ledc.c', 'i2s.c', 'panel_st7789.c'], arduino: true })) add(u.src, u.lang);
addDir(path.join(repo, 'vhw/src'));

console.log(`firmware OpenDeskBotV2 ${upstream.lock.sources.OpenDeskBotV2.commit.slice(0, 7)} (VERSION ${version}): ${units.length} compile units`);
const objs = await compileAll(units, { objDir: path.join(out, 'obj'), force: flag('force') });
const moduleFile = path.join(out, 'brufik.mjs');
await link(objs, {
  out: moduleFile,
  force: flag('force'),
  flags: firmwareLinkFlags({
    exportName: 'createBrufikFirmware',
    memoryMB: template.runtime.memoryMB,
    threads: template.runtime.threads,
    extra: [
      // upstream platformio.ini link option: soft-restart attribution
      '-Wl,--wrap=esp_restart',
      // device layer: copy the firmware's log frames to the interface's log channel (vhw/src/board.c), upstream unchanged
      '-Wl,--wrap=_Z22usb_transport_send_logPKcm',
      '-Wl,--error-limit=0',
    ],
  }),
});

// ---------- 3. package ----------
// Licences shipped with the firmware: this repository's (GPL-3.0), upstream's, and every compiled library's.
const LICENCES = [
  ['licenses/LICENSE', path.join(repo, 'LICENSE')],
  ['licenses/OpenDeskBotV2-LICENSE.md', path.join(upstream.path('OpenDeskBotV2'), 'LICENSE.md')],
  ['licenses/OpenDeskBotV2-firmware-LICENSE', path.join(fw, 'LICENSE')],
];
for (const name of upstream.sourcesFor('brufik')) {
  if (name === 'OpenDeskBotV2') continue;
  const dir = upstream.path(name);
  const file = upstream.lock.sources[name].licenceFile ?? readdirSync(dir).find((f) => /^(licen[cs]e|copying)(\.(md|txt))?$/i.test(f));
  if (!file) throw new Error(`${name} has no licence file at its root`);
  LICENCES.push([`licenses/${path.basename(name)}-${file}`, path.join(dir, file)]);
}
const files = new Map([
  ['firmware/brufik.mjs', moduleFile],
  ['firmware/brufik.wasm', path.join(out, 'brufik.wasm')],
  ...LICENCES,
].map(([p, f]) => [p, new Uint8Array(readFileSync(f))]));

const head = git('rev-parse', 'HEAD');
const description = structuredClone(template);
description.firmware.version = version;
description.firmware.upstream.commit = upstream.lock.sources.OpenDeskBotV2.commit;
description.firmware.virtualDevice.commit = head;
description.firmware.license.files = LICENCES.map(([p]) => p);

const nodeIssues = checkNodesInModel(description, await kitModel());
if (nodeIssues.length) throw new Error(`the description does not fit the Kit model:\n${nodeIssues.map((i) => `  ${i.path}: ${i.message}`).join('\n')}`);

const { archive } = await buildPackage({ description, files });
mkdirSync(packageDir, { recursive: true });
const packageFile = path.join(packageDir, 'brufik.vdpkg.tgz');
writeFileSync(packageFile, archive);
const sha = (await import('node:crypto')).createHash('sha256').update(archive).digest('hex');
console.log(`  ✓ ${packageFile}\n    ${archive.length} bytes, sha256 ${sha}\n    firmware ${version} @ ${description.firmware.upstream.commit.slice(0, 7)}, brufik-virtual @ ${head.slice(0, 7)}${flag('dev') ? ' (--dev: not for upload)' : ''}`);
