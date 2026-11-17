// The package seam: the file `npm run package` makes is what the shop's checker receives (device description format v1).
import assert from 'node:assert/strict';
import path from 'node:path';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';
import { checkNodesInModel, checkPackageFiles, glbNodeNames, validateDeviceDescription } from 'esp32-virtual-hardware/device-description';
import { servoJointAngle } from 'esp32-virtual-hardware/runtime';
import { kitModel } from '../kit-model.mjs';
import { brufikPackage } from './lib/package.mjs';

const repoRoot = fileURLToPath(new URL('../', import.meta.url)).replace(/\/$/, '');

const lock = (await import('../upstream.lock.json', { with: { type: 'json' } })).default;

test('the package passes the device description validator and its files match the list', async () => {
  const { description, files } = await brufikPackage();
  const { ok, issues } = validateDeviceDescription(description);
  assert.deepEqual(issues, []);
  assert.ok(ok);
  assert.deepEqual(await checkPackageFiles(description, files), []);
});

test('every node the description names is in the current Brufik Kit model', async () => {
  const { description } = await brufikPackage();
  assert.deepEqual(checkNodesInModel(description, glbNodeNames(await kitModel())), []);
});

test('the description names the pinned upstream firmware, this repository and the licence', async () => {
  const { description: d } = await brufikPackage();
  assert.equal(d.device.id, 'brufik');
  assert.deepEqual(d.appliesTo, { kit: 'kit_brufik_v2', options: [] });
  assert.deepEqual(d.firmware.upstream, { repo: lock.sources.OpenDeskBotV2.url, commit: lock.sources.OpenDeskBotV2.commit });
  assert.equal(d.firmware.version, '0.0.59');
  assert.equal(d.firmware.virtualDevice.repo, 'https://github.com/ThorbJ/brufik-virtual');
  assert.match(d.firmware.virtualDevice.commit, /^[0-9a-f]{40}$/);
  assert.equal(d.firmware.license.spdx, 'GPL-3.0-only');
  assert.equal(d.firmware.license.unofficial, true);
});

test('the computer connection is the one the OpenDeskBotV2 virtual device transport listens on', async () => {
  const { description: d } = await brufikPackage();
  assert.deepEqual(d.hostLink, { port: 9022, path: '/virtual-device', protocol: 'DBOT v1' });
  assert.deepEqual([...d.channels.in].sort(), ['camera', 'hostPresent', 'hostRx', 'mic']);
  assert.deepEqual([...d.channels.out].sort(), ['hostTx', 'log', 'screen', 'servo', 'speaker']);
});

test('the firmware files carry no local folder paths of the machine that built them', async () => {
  const { files } = await brufikPackage();
  const home = new TextEncoder().encode(process.env.HOME || '/Users/');
  for (const [p, bytes] of files) {
    if (!p.startsWith('firmware/')) continue;
    const text = Buffer.from(bytes).toString('latin1');
    for (const needle of [Buffer.from(home).toString('latin1'), '/private/tmp/', '/Users/', path.dirname(repoRoot)]) {
      assert.ok(!text.includes(needle), `${p} contains ${needle}`);
    }
  }
});

test('servo X turns the head and neck about the vertical neck axis, about ±29° at the firmware limits 20–160°', async () => {
  const { description: d } = await brufikPackage();
  const x = d.servos.find((s) => s.id === 0);
  assert.deepEqual(x.pivot, { node: 'neck-shaft' });
  assert.deepEqual(x.axis, [0, 1, 0]);
  for (const n of ['head-front', 'head-back', 'lcd', 'neck-shaft']) assert.ok(x.nodes.includes(n), `${n} turns with servo X`);
  // worked example: 160° → (160 − 90) × 0.5 (1000–2000 µs is half the servo's travel) × 30/36 (gears) = 29.17° toward Brufik's left (+Y)
  assert.ok(Math.abs(servoJointAngle(x, 160) - 29.17) < 0.01, `${servoJointAngle(x, 160)}`);
  assert.ok(Math.abs(servoJointAngle(x, 20) + 29.17) < 0.01, `${servoJointAngle(x, 20)}`);
  assert.equal(servoJointAngle(x, 90), 0);
});
