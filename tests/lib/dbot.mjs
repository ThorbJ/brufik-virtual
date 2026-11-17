// 小歪的电脑端协议 DBOT v1 的编解码（只用于测试：扮演电脑端；固件和上游服务各用自己的实现）。
// 帧格式（小端）：24 字节头 {magic "DBOT", version 1, header_size 24, channel, flags, sequence, session_epoch,
// payload_length, header_crc32(前 20 字节)} + payload + payload_crc32。
// 来源：上游 hardware/firmware/usb_transport.h、service/src/deskbot_server/infrastructure/serial/protocol.py。
// 浏览器和 Node 通用。

export const MAGIC = 0x544f4244;
export const HEADER_SIZE = 24;
export const MAX_PAYLOAD = 1024 * 1024;

export const CHANNEL = Object.freeze({ CONTROL_JSON: 1, PB_WIRE: 2, AUDIO_UP_OPUS: 3, AUDIO_DOWN_OPUS: 4, CAMERA_JPEG: 5, LOG: 6 });
export const CHANNEL_NAME = Object.freeze(Object.fromEntries(Object.entries(CHANNEL).map(([k, v]) => [v, k])));
export const FLAG = Object.freeze({ ACK_REQUIRED: 0x01, ACK: 0x02, ERROR: 0x04, END_STREAM: 0x08, JSON: 0x10, BEGIN_STREAM: 0x20, CANCEL_STREAM: 0x40 });

const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    t[n] = c >>> 0;
  }
  return t;
})();

/** CRC-32/IEEE（与 zlib.crc32 相同）。 */
export function crc32(bytes) {
  let c = 0xffffffff;
  for (let i = 0; i < bytes.length; i++) c = CRC_TABLE[(c ^ bytes[i]) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

const enc = new TextEncoder();
const dec = new TextDecoder();

/** 编一帧。json 给出时按 JSON 编 payload（与上游一样紧凑分隔）。 */
export function encodeFrame({ channel, flags = 0, sequence = 0, epoch = 0, payload, json }) {
  const body = json !== undefined ? enc.encode(JSON.stringify(json)) : (payload ?? new Uint8Array(0));
  const out = new Uint8Array(HEADER_SIZE + body.length + 4);
  const v = new DataView(out.buffer);
  v.setUint32(0, MAGIC, true);
  v.setUint8(4, 1);
  v.setUint8(5, HEADER_SIZE);
  v.setUint8(6, channel);
  v.setUint8(7, flags);
  v.setUint32(8, sequence >>> 0, true);
  v.setUint32(12, epoch >>> 0, true);
  v.setUint32(16, body.length, true);
  v.setUint32(20, crc32(out.subarray(0, 20)), true);
  out.set(body, HEADER_SIZE);
  v.setUint32(HEADER_SIZE + body.length, crc32(body), true);
  return out;
}

/**
 * 流式解码：push(bytes) 返回这次凑齐的帧数组。帧外的字节（握手前固件打印的纯文本日志）
 * 以 { text } 形式按出现顺序混在结果里。CONTROL 通道和带 JSON 标志的帧附带解析好的 json。
 */
export class DbotDecoder {
  constructor() { this.buf = new Uint8Array(0); }

  push(bytes) {
    const merged = new Uint8Array(this.buf.length + bytes.length);
    merged.set(this.buf);
    merged.set(bytes, this.buf.length);
    const b = merged;
    const out = [];
    let textStart = -1;
    const flushText = (end) => {
      if (textStart >= 0 && end > textStart) out.push({ text: dec.decode(b.subarray(textStart, end)) });
      textStart = -1;
    };
    let i = 0;
    while (i < b.length) {
      if (b.length - i < 4) break; // 可能是半个 magic，留到下次
      const v = new DataView(b.buffer, b.byteOffset + i);
      if (v.getUint32(0, true) !== MAGIC) { if (textStart < 0) textStart = i; i++; continue; }
      if (b.length - i < HEADER_SIZE) break;
      const ok = v.getUint8(4) === 1 && v.getUint8(5) === HEADER_SIZE && crc32(b.subarray(i, i + 20)) === v.getUint32(20, true);
      const len = v.getUint32(16, true);
      if (!ok || len > MAX_PAYLOAD) { if (textStart < 0) textStart = i; i++; continue; }
      if (b.length - i < HEADER_SIZE + len + 4) break;
      const payload = b.slice(i + HEADER_SIZE, i + HEADER_SIZE + len);
      if (crc32(payload) !== v.getUint32(HEADER_SIZE + len, true)) { if (textStart < 0) textStart = i; i++; continue; }
      flushText(i);
      const frame = { channel: v.getUint8(6), flags: v.getUint8(7), sequence: v.getUint32(8, true), epoch: v.getUint32(12, true), payload };
      if (frame.channel === CHANNEL.CONTROL_JSON || (frame.flags & FLAG.JSON)) {
        try { frame.json = JSON.parse(dec.decode(payload)); } catch { /* 不是 JSON 就只留原始字节 */ }
      }
      out.push(frame);
      i += HEADER_SIZE + len + 4;
    }
    // 已扫过的纯文本吐出；从 i 起可能是不完整的帧，留到下次
    flushText(i);
    this.buf = b.slice(i);
    return out;
  }
}

/** 一帧的单行摘要（通信日志用）。 */
export function describeFrame(f) {
  if (f.text !== undefined) return `文本 ${JSON.stringify(f.text.length > 120 ? `${f.text.slice(0, 120)}…` : f.text)}`;
  const name = CHANNEL_NAME[f.channel] ?? `CH${f.channel}`;
  const flags = Object.entries(FLAG).filter(([, bit]) => f.flags & bit).map(([k]) => k).join('|');
  let detail = `${f.payload.length} B`;
  if (f.json) {
    const s = JSON.stringify(f.json);
    detail = f.json.type ? `${f.json.type} ${s.length > 160 ? `${s.slice(0, 160)}…` : s}` : s.slice(0, 160);
  }
  return `${name}${flags ? `[${flags}]` : ''} #${f.sequence} ${detail}`;
}
