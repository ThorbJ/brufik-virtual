// Plays Brufik's computer-side program in tests (DBOT v1), using only the interface's hostRx / hostTx / hostPresent.
import assert from 'node:assert/strict';
import { createVirtualDevice } from 'esp32-virtual-hardware/interface';
import { CHANNEL, DbotDecoder, encodeFrame } from './dbot.mjs';

export const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

/**
 * Power on, say hello as the computer does (the computer speaks first in DBOT v1) and wait for hello_ack.
 * setup(dev) runs before power on (e.g. a camera source: the firmware checks the camera at boot).
 * Returns { dev, ack, frames, send(frame), controls(type), close() }; frames: every device frame after the handshake, with
 * the wall time it arrived (at); controls(type): the JSON messages of that type.
 */
export async function connectedBrufik(description, factory, { setup, clientNonce = 4242 } = {}) {
  const dev = createVirtualDevice(description, factory);
  setup?.(dev);
  const decoder = new DbotDecoder();
  const frames = [];
  let ack;
  if (process.env.DEBUG_BRUFIK) dev.on('log', (l) => process.stdout.write(l));
  dev.on('hostTx', (bytes) => {
    for (const f of decoder.push(bytes)) {
      if (f.json?.type === 'hello_ack') ack = f.json;
      if (f.text === undefined) frames.push({ ...f, at: Date.now() });
    }
  });
  dev.setHostPresent(true);
  await dev.powerOn();
  const hello = encodeFrame({ channel: CHANNEL.CONTROL_JSON, json: { type: 'hello', protocol: 1, client: 'deskbot-pc', client_nonce: clientNonce } });
  const deadline = Date.now() + 30_000;
  while (!ack && Date.now() < deadline) { dev.hostWrite(hello); await sleep(500); }
  assert.ok(ack, 'no hello_ack within 30 s');
  let seq = 1;
  const send = (frame) => dev.hostWrite(encodeFrame({ ...frame, sequence: seq++, epoch: ack.session_epoch }));
  send({ channel: CHANNEL.CONTROL_JSON, json: { type: 'heartbeat' } });
  // upstream keeps the session alive with a heartbeat every 2 s (hello_ack heartbeat_ms)
  const hb = setInterval(() => send({ channel: CHANNEL.CONTROL_JSON, json: { type: 'heartbeat' } }), 2000);
  const controls = (type) => frames.filter((f) => f.json?.type === type).map((f) => ({ ...f.json, at: f.at }));
  const close = async () => { clearInterval(hb); await dev.powerOff(); };
  return { dev, ack, frames, send, controls, close };
}

/** Waits until check() is truthy (polling), up to timeoutMs. */
export async function until(check, timeoutMs = 15_000) {
  const deadline = Date.now() + timeoutMs;
  while (!check() && Date.now() < deadline) await sleep(100);
  return check();
}
