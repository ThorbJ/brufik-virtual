// The built package, as the shop receives it. Tests drive the firmware from inside this package, so what is tested is what ships.
//   BRUFIK_PACKAGE=<file>  another package file (default build/brufik.vdpkg.tgz, made by `npm run package`)
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { readPackage } from 'esp32-virtual-hardware/package';
import { loadFirmwareFile } from 'esp32-virtual-hardware/interface/node';

const repo = fileURLToPath(new URL('../../', import.meta.url));
export const packageFile = path.resolve(process.env.BRUFIK_PACKAGE || path.join(repo, 'build/brufik.vdpkg.tgz'));

let cached;
/** { archive, description, files } of the package (read once). */
export async function brufikPackage() {
  if (!cached) {
    let archive;
    try { archive = new Uint8Array(readFileSync(packageFile)); } catch { throw new Error(`no package at ${packageFile}: run npm run package first`); }
    cached = { archive, ...(await readPackage(archive)) };
  }
  return cached;
}

/** { description, factory }: the firmware of the package, unpacked into a temporary folder and loaded for createVirtualDevice. */
export async function brufikFirmware() {
  const { description, files } = await brufikPackage();
  const dir = path.join(tmpdir(), `brufik-virtual-${process.pid}`);
  for (const [p, bytes] of files) {
    mkdirSync(path.dirname(path.join(dir, p)), { recursive: true });
    writeFileSync(path.join(dir, p), bytes);
  }
  return { description, factory: await loadFirmwareFile(path.join(dir, description.runtime.module)) };
}
