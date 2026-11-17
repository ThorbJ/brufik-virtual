// The description on the real Brufik Kit model, through the shared three.js viewer code (esp32-virtual-hardware
// runtime/three-nodes, the same hooks a page's viewer implements), in Node without WebGL: the picture lands on the screen face
// the right way up, and the head tilt rides on the head turn. Brufik faces −Z in the Kit model, +Y is up.
import assert from 'node:assert/strict';
import { test } from 'node:test';
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { MeshoptDecoder } from 'three/addons/libs/meshopt_decoder.module.js';
import { glbJson } from 'esp32-virtual-hardware/device-description';
import { createScreenImage, servoJointAngle } from 'esp32-virtual-hardware/runtime';
import { createNodeControls } from 'esp32-virtual-hardware/runtime/three-nodes';
import { kitModel } from '../kit-model.mjs';
import { brufikPackage } from './lib/package.mjs';

const { description: d } = await brufikPackage();

/** The Kit model loaded as a page loads it, minus its JPEG textures (Node has no image decoder; they play no part here). */
async function loadKit() {
  const bytes = await kitModel();
  const json = glbJson(bytes);
  const jl = new DataView(bytes.buffer, bytes.byteOffset).getUint32(12, true);
  delete json.textures; delete json.images; delete json.samplers;
  for (const m of json.materials ?? []) delete m.pbrMetallicRoughness?.baseColorTexture;
  const jb = new TextEncoder().encode(JSON.stringify(json));
  const pad = (jb.length + 3) & ~3;
  const bin = bytes.subarray(20 + jl);
  const glb = new Uint8Array(20 + pad + bin.length);
  const dv = new DataView(glb.buffer);
  dv.setUint32(0, 0x46546c67, true); dv.setUint32(4, 2, true); dv.setUint32(8, glb.length, true);
  dv.setUint32(12, pad, true); dv.setUint32(16, 0x4e4f534a, true);
  glb.set(jb, 20); glb.fill(0x20, 20 + jb.length, 20 + pad); glb.set(bin, 20 + pad);
  await MeshoptDecoder.ready;
  const gltf = await new GLTFLoader().setMeshoptDecoder(MeshoptDecoder).parseAsync(glb.buffer, '');
  gltf.scene.updateMatrixWorld(true);
  return { model: gltf.scene, nodes: createNodeControls(THREE, gltf.scene, gltf.parser) };
}

test('the panel picture shows upright on the screen face seen from the front', async () => {
  const { model, nodes } = await loadKit();
  // rotateCW 270 (upstream setRotation(3)): the panel's top-right corner is the top-left of the picture people see.
  const { width, height } = d.screen.panel;
  const pixels = new Uint16Array(width * height);
  for (let y = 0; y < 40; y++) for (let x = width - 40; x < width; x++) pixels[y * width + x] = 0xf800; // red
  const image = createScreenImage(d.screen);
  image.update({ width, height, pixels });
  nodes.setScreenImage({ node: d.screen.node, face: d.screen.face }, image);
  const face = nodes.faceMesh({ node: d.screen.node, face: d.screen.face });
  assert.ok(face.material.map?.isDataTexture, 'the picture is on the face mesh');
  model.updateMatrixWorld(true);

  // Look at the face from in front of Brufik (−Z side, looking +Z); the viewer's right is −X.
  const colourAt = (x, y) => {
    const ray = new THREE.Raycaster(new THREE.Vector3(x, y, -300), new THREE.Vector3(0, 0, 1));
    const hit = ray.intersectObject(model, true).find((h) => h.object === face);
    assert.ok(hit, `the face is visible at (${x}, ${y})`);
    const tx = Math.min(image.width - 1, Math.floor(hit.uv.x * image.width));
    const ty = Math.min(image.height - 1, Math.floor(hit.uv.y * image.height)); // flipY = false: v = 0 is the first row
    return [...image.data.subarray((ty * image.width + tx) * 4, (ty * image.width + tx) * 4 + 3)];
  };
  assert.deepEqual(colourAt(16, 94), [255, 0, 0], 'top-left as seen from the front is red');
  assert.deepEqual(colourAt(-16, 94), [0, 0, 0], 'top-right is not');
  assert.deepEqual(colourAt(16, 68), [0, 0, 0], 'bottom-left is not');
});

test('the head tilt rides on the head turn: servo Y on servo X, about the 9 g servo shaft, 110° (Y_MAX) looks up', async () => {
  const x = d.servos.find((s) => s.id === 0);
  const y = d.servos.find((s) => s.id === 1);
  assert.equal(y.parent, 0);
  // internal-pack's printed parts have no names: [7] print-13 (U-bracket, below the tilt axis) turns with the neck;
  // [8]–[11] print-14…17 (head frame, screen holder) tilt with the head; the others stay in the body.
  assert.deepEqual(x.nodes, ['neck-shaft', 'servo-9g', { node: 'internal-pack', path: [7] }]);
  for (const n of ['head-front', 'head-back', 'lcd', 'screen-pcb']) assert.ok(y.nodes.includes(n), `${n} tilts`);
  for (const k of [8, 9, 10, 11]) assert.ok(y.nodes.some((n) => n.node === 'internal-pack' && n.path[0] === k), `internal part ${k} tilts`);
  assert.equal(servoJointAngle(y, 110), 20); // full-travel 500–2500 µs: 1:1
  assert.equal(servoJointAngle(y, 78), -12);

  const { model, nodes } = await loadKit();
  const centre = (name) => new THREE.Box3().setFromObject(model.getObjectByName(name)).getCenter(new THREE.Vector3());
  const before = centre('lcd');
  const turn = (s, angle) => ({ pivot: s.pivot, axis: s.axis, angleDeg: servoJointAngle(s, angle) });
  nodes.setRotation(y.nodes, [turn(y, 110), turn(x, 90)]);
  const up = centre('lcd');
  assert.ok(up.y - before.y > 5, `the screen rises when Brufik looks up: ${before.y.toFixed(1)} → ${up.y.toFixed(1)}`);
  // turned to its left as well (X 160° = +29° about +Y; facing −Z, Brufik's left is −X): the tilted screen swings toward −X
  // and keeps its height
  nodes.setRotation(y.nodes, [turn(y, 110), turn(x, 160)]);
  const both = centre('lcd');
  assert.ok(up.x - both.x > 5, `the tilted head turns with the neck: x ${up.x.toFixed(1)} → ${both.x.toFixed(1)}`);
  assert.ok(Math.abs(both.y - up.y) < 0.5);
});
