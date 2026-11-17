// The Brufik Kit model (GLB) that the device description's node names refer to, pinned by sha256 in kit-model.json.
// Downloaded once into .cache/ (git-ignored); BRUFIK_KIT_GLB=<file> uses a local copy instead (still checked).
import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = fileURLToPath(new URL('./', import.meta.url));
export const KIT_MODEL = JSON.parse(readFileSync(path.join(here, 'kit-model.json'), 'utf8'));

const sha256 = (bytes) => createHash('sha256').update(bytes).digest('hex');

/** The Kit model's bytes (Uint8Array). Throws when the bytes are not the pinned model. */
export async function kitModel() {
  const local = process.env.BRUFIK_KIT_GLB;
  const cache = path.join(here, '.cache', `kit-model-${KIT_MODEL.sha256.slice(0, 12)}.glb`);
  let bytes;
  if (local) bytes = readFileSync(local);
  else if (existsSync(cache)) bytes = readFileSync(cache);
  else {
    const res = await fetch(KIT_MODEL.url);
    if (!res.ok) throw new Error(`Kit model download failed: HTTP ${res.status} ${KIT_MODEL.url}`);
    bytes = Buffer.from(await res.arrayBuffer());
  }
  const got = sha256(bytes);
  if (got !== KIT_MODEL.sha256) throw new Error(`the Kit model is not the pinned one (sha256 ${got}, expected ${KIT_MODEL.sha256}): update kit-model.json and check the node names`);
  if (!local && !existsSync(cache)) { mkdirSync(path.dirname(cache), { recursive: true }); writeFileSync(cache, bytes); }
  return new Uint8Array(bytes);
}
