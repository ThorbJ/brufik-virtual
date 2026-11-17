// Brufik ↔ the computer-side program (DBOT v1 bytes through the interface's hostRx / hostTx), no browser.
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { readFileSync } from 'node:fs';
import { CHANNEL, FLAG } from './lib/dbot.mjs';
import { connectedBrufik, sleep, until } from './lib/host.mjs';
import { brufikFirmware } from './lib/package.mjs';

const { description, factory } = await brufikFirmware();

test('the computer says hello, Brufik answers with a DBOT hello_ack (device id from its fixed virtual MAC)', async () => {
  const host = await connectedBrufik(description, factory, { clientNonce: 0x1234abcd });
  try {
    assert.equal(host.ack.device_id, 'deskbot_02464d534e01');
    assert.equal(host.ack.ack_client_nonce, 0x1234abcd);
    assert.equal(host.ack.protocol, 1);
    assert.equal(host.ack.transport, 'usb_cdc');
    assert.ok(host.ack.capabilities.includes('control_json'));
  } finally {
    await host.close();
  }
});

// RGB565 pure red and green (the panel may receive them byte-swapped)
const RED = 0xf800;
const GREEN = 0x07e0;
const countColour = (frame, c) => {
  const swapped = ((c & 0xff) << 8) | (c >> 8);
  let n = 0;
  for (const p of frame.pixels) if (p === c || p === swapped) n++;
  return n;
};
const rms = (s) => Math.sqrt(s.reduce((e, v) => e + v * v, 0) / Math.max(1, s.length));
const zeroCrossHz = (s, rate) => {
  let n = 0;
  for (let i = 1; i < s.length; i++) if ((s[i - 1] < 0) !== (s[i] < 0)) n++;
  return (n / 2) * rate / s.length;
};
const firstChannel = (events) => {
  const parts = events.map((e) => (e.channels <= 1 ? e.samples : Int16Array.from({ length: Math.floor(e.samples.length / e.channels) }, (_, i) => e.samples[i * e.channels])));
  const out = new Int16Array(parts.reduce((n, p) => n + p.length, 0));
  let o = 0;
  for (const p of parts) { out.set(p, o); o += p.length; }
  return out;
};

test('an expression + audio PB from the computer: the screen draws the sent faces and the speaker plays the sent 440 Hz tone', async () => {
  const host = await connectedBrufik(description, factory);
  const { dev, send, controls } = host;
  try {
    await sleep(2500); // past the boot chime and the first idle face after connecting
    const speaker = [];
    dev.on('speaker', (s) => speaker.push(s));
    const sawColour = { [RED]: false, [GREEN]: false };
    dev.on('screen', (f) => { for (const c of [RED, GREEN]) if (!sawColour[c] && countColour(f, c) > 1500) sawColour[c] = true; });

    // upstream PB format: service/docs/esp32_pb_protocol.md §3–§6 (anim[] faces + s16le audio after the JSON)
    const sr = 16000;
    const chunkMs = 1200;
    const pcm = Int16Array.from({ length: (chunkMs * sr) / 1000 }, (_, i) => Math.round(12000 * Math.sin((2 * Math.PI * 440 * i) / sr)));
    const bin = new Uint8Array(pcm.buffer);
    const face = (c) => ({ bg: [{ shape: 'rect', x: 0, y: 0, w: 284, h: 240, c: 0 }], eye_l: [{ shape: 'circle', x: 90, y: 100, r: 30, c }], eye_r: [{ shape: 'circle', x: 194, y: 100, r: 30, c }] });
    const req = 'voice-test-01';
    send({
      channel: CHANNEL.PB_WIRE,
      flags: FLAG.JSON | FLAG.ACK_REQUIRED,
      json: {
        type: 'pb_single', req, idx: 0, chunk_ms: chunkMs, pb_ver: 2, action: 'replace', level: 1,
        sr, fmt: 's16le', ch: 1, audio: { next_bin_len: bin.length },
        anim: [{ elements: face(RED), ms: 600 }, { elements: face(GREEN), ms: 600 }],
      },
    });
    for (let o = 0; o < bin.length; o += 2048) send({ channel: CHANNEL.PB_WIRE, payload: bin.subarray(o, o + 2048) });
    const played = () => controls('pb_ack').some((a) => a.req === req && a.phase === 'played');
    assert.ok(await until(played), `no played ack: ${JSON.stringify(controls('pb_ack'))}`);

    assert.ok(sawColour[RED], 'the first face (red eyes) was not drawn');
    assert.ok(sawColour[GREEN], 'the second face (green eyes) was not drawn');
    assert.equal(speaker[0]?.sampleRate, 16000);
    const times = speaker.map((s) => s.t);
    assert.ok(times.every((t, i) => Number.isFinite(t) && (i === 0 || t >= times[i - 1])), `speaker chunks carry the device time: ${times.slice(0, 5)}`);
    const mono = firstChannel(speaker);
    const win = 320; // 20 ms
    const loud = [];
    for (let o = 0; o + win <= mono.length; o += win) if (rms(mono.subarray(o, o + win)) > 1000) loud.push(...mono.subarray(o, o + win));
    const loudMs = (loud.length / 16000) * 1000;
    assert.ok(loudMs >= 1000 && loudMs <= 1600, `sound for ${loudMs} ms, about 1200 ms expected`);
    const hz = zeroCrossHz(loud, 16000);
    assert.ok(Math.abs(hz - 440) < 25, `speaker plays ${hz.toFixed(1)} Hz, about 440 Hz expected`);
  } finally {
    await host.close();
  }
});

test('a motion PB from the computer: the servo channel steps both servos to the targets and back (X left-right, Y tilt)', async () => {
  const host = await connectedBrufik(description, factory);
  const { dev, send, controls } = host;
  try {
    await sleep(1500); // centred at boot, then released (duty 0) 150 ms after arriving
    const servo = [];
    dev.on('servo', (s) => servo.push(s));
    const req = 'servo-test-01';
    // a motion-only PB (pb_single + servo[]), upstream service/docs/esp32_pb_protocol.md §4.3
    send({
      channel: CHANNEL.PB_WIRE,
      flags: FLAG.JSON | FLAG.ACK_REQUIRED,
      json: { type: 'pb_single', req, idx: 0, chunk_ms: 1600, pb_ver: 2, action: 'replace', level: 1, servo: [{ xm: 0, ym: 0, x: 130, y: 104, ms: 800 }, { xm: 0, ym: 0, x: 90, y: 90, ms: 800 }] },
    });
    assert.ok(await until(() => controls('pb_ack').some((a) => a.req === req && a.phase === 'played')), `no played ack: ${JSON.stringify(controls('pb_ack'))}`);
    const angles = (id) => servo.filter((e) => e.id === id && e.pulseUs > 0).map((e) => e.angle);
    const x = angles(0);
    const y = angles(1);
    assert.ok(Math.max(...x) >= 127, `X should reach about 130°: ${x.join(',')}`);
    assert.ok(Math.abs(x.at(-1) - 90) <= 2, `X should end centred: ${x.at(-1)}`);
    assert.ok(Math.max(...y) >= 101, `Y should reach about 104°: ${y.join(',')}`);
    assert.ok(Math.abs(y.at(-1) - 90) <= 2, `Y should end centred: ${y.at(-1)}`);
    assert.ok(x.length >= 20, `X should move in steps, got ${x.length} angles`);
    for (let i = 1; i < x.length; i++) assert.ok(Math.abs(x[i] - x[i - 1]) <= 2.2, `X jumped ${x[i] - x[i - 1]}° at step ${i}`);
  } finally {
    await host.close();
  }
});

test('when the firmware wants a camera frame it asks the interface for 320×240 JPEG and sends that JPEG to the computer', async () => {
  const jpeg = new Uint8Array(readFileSync(new URL('./fixtures/camera-test-320x240.jpg', import.meta.url)));
  const requests = [];
  const host = await connectedBrufik(description, factory, {
    setup: (dev) => dev.setCameraSource(async (req) => { requests.push({ ...req }); await sleep(5); return jpeg; }), // browsers encode JPEG asynchronously
  });
  const { send, frames, controls } = host;
  try {
    await sleep(1500);
    const before = frames.length;
    const req = 'camera-test-01';
    // upstream's camera fast path: pb_single camera_once (asr_chat_client.cpp)
    send({ channel: CHANNEL.PB_WIRE, flags: FLAG.JSON | FLAG.ACK_REQUIRED, json: { type: 'pb_single', req, idx: 0, camera_once: true } });
    const up = () => frames.slice(before).filter((f) => f.channel === CHANNEL.CAMERA_JPEG);
    assert.ok(await until(() => up().length), 'no CAMERA_JPEG frame went to the computer');
    assert.ok(controls('pb_ack').some((a) => a.req === req), 'camera_once not acknowledged');
    assert.deepEqual(up()[0].payload, jpeg, 'the JPEG sent up is not the one the camera gave');
    assert.deepEqual(requests.at(-1), { width: 320, height: 240, format: 'jpeg' });
  } finally {
    await host.close();
  }
});
