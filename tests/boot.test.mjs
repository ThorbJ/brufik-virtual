// Brufik boots: observed only through the virtual device interface (inputs in, outputs out), from the firmware in the package.
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { test } from 'node:test';
import { createVirtualDevice, isAllBlack } from 'esp32-virtual-hardware/interface';
import { brufikFirmware } from './lib/package.mjs';

const { description, factory } = await brufikFirmware();
const BOOT_TIMEOUT_MS = 30_000;

async function withDevice(fn) {
  const dev = createVirtualDevice(description, factory);
  try {
    await dev.powerOn();
    return await fn(dev);
  } finally {
    await dev.powerOff();
  }
}

test('after power on the upstream firmware logs its start and the screen stops being black (native 240×284 RGB565)', async () => {
  await withDevice(async (dev) => {
    await dev.waitForLog(/Initializing Deskbot/, BOOT_TIMEOUT_MS);
    const frame = await dev.waitForScreen((f) => !isAllBlack(f), BOOT_TIMEOUT_MS);
    assert.equal(frame.width, 240);
    assert.equal(frame.height, 284);
    assert.equal(frame.format, 'rgb565');
  });
});

const frameHash = (f) => createHash('sha256').update(new Uint8Array(f.pixels.buffer, f.pixels.byteOffset, f.pixels.byteLength)).digest('hex').slice(0, 16);

/** Every screen frame (device time + content hash) from power on to device time untilMs, the clock parked there. */
async function framesUntil(untilMs) {
  return withDevice(async (dev) => {
    const frames = [];
    dev.on('screen', (f) => frames.push(`${f.t}:${frameHash(f)}`));
    await dev.runUntil(untilMs, 120_000);
    return frames;
  });
}

test('the same inputs give the same screen frames twice (device time and content): boot → idle face → "connect the PC"', async () => {
  const first = await framesUntil(12_000);
  const second = await framesUntil(12_000);
  assert.ok(new Set(first.map((f) => f.split(':')[1])).size >= 2, `the screen should change while booting: ${first.join(' ')}`);
  assert.deepEqual(second, first);
});

test('a power cycle boots again (reloading the page = power cycle)', async () => {
  const dev = createVirtualDevice(description, factory);
  try {
    await dev.powerOn();
    await dev.waitForLog(/Initializing Deskbot/, BOOT_TIMEOUT_MS);
    await dev.restart();
    await dev.waitForLog(/Initializing Deskbot/, BOOT_TIMEOUT_MS);
    assert.equal(dev.bootCount, 2);
  } finally {
    await dev.powerOff();
  }
});
