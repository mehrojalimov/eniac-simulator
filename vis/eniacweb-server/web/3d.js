import * as THREE from "three";
import { OBJLoader } from "/vendor/three/OBJLoader.js";
import { MTLLoader } from "/vendor/three/MTLLoader.js";
import { createLampSystem } from "/3d-lamps.js";

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(
  60,
  window.innerWidth / window.innerHeight,
  1,
  25000
);
camera.position.set(0, 0, 1000);
camera.lookAt(0, 0, 20000);

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

window.addEventListener("resize", () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});

// World units here are large CAD-style numbers (thousands), not meters, so
// Three.js's physically-correct inverse-square light falloff (decay=2)
// would make any reasonable intensity read as pitch black over these
// distances. decay=0 (no distance attenuation) sidesteps that entirely.
scene.add(new THREE.AmbientLight(0xffffff, 2.2));
for (const pos of [
  [-1300, 2000, 9000],
  [-1300, 2000, 3000],
  [1300, 2000, 9000],
  [1300, 2000, 3000],
]) {
  const light = new THREE.PointLight(0xffffff, 1.2, 0, 0);
  light.position.set(...pos);
  scene.add(light);
}

// --- Movement: click-and-drag mouse look + WASD walking -----------------
// Deliberately NOT using pointer lock: a permanently-captured mouse means
// every HUD click (program select, INIT, etc.) needs an Esc-to-release
// round trip first, which fights the actual usage pattern of this page
// (frequent panel interaction interleaved with looking around). Drag-to-
// look keeps the mouse free the whole time - HUD buttons work normally -
// and only rotates the view while a drag is in progress, same as most
// browser-based walkthroughs (e.g. Street View).

window.__eniacCamera = camera;

let yaw = 0;
let pitch = 0;
const MOUSE_SENSITIVITY = 0.0035;

function applyLookRotation() {
  camera.quaternion.setFromEuler(new THREE.Euler(pitch, yaw, 0, "YXZ"));
}

let dragging = false;
let lastX = 0;
let lastY = 0;
renderer.domElement.addEventListener("mousedown", (e) => {
  dragging = true;
  lastX = e.clientX;
  lastY = e.clientY;
  renderer.domElement.classList.add("dragging");
});
window.addEventListener("mouseup", () => {
  dragging = false;
  renderer.domElement.classList.remove("dragging");
});
window.addEventListener("mousemove", (e) => {
  if (!dragging) return;
  yaw -= (e.clientX - lastX) * MOUSE_SENSITIVITY;
  pitch -= (e.clientY - lastY) * MOUSE_SENSITIVITY;
  const limit = Math.PI / 2 - 0.01;
  pitch = Math.max(-limit, Math.min(limit, pitch));
  lastX = e.clientX;
  lastY = e.clientY;
  applyLookRotation();
});

// Left/right turn the view (like eniacfp.cpp's own arrow-key controls -
// up/down walk, left/right turn) rather than strafing sideways: with
// look-around already on mouse-drag, keyboard left/right turning is the
// more useful pairing than a sideways strafe, and matches how a person
// would actually turn to face something while walking.
const move = { forward: false, back: false, turnLeft: false, turnRight: false };
const KEY_BINDINGS = {
  KeyW: "forward",
  ArrowUp: "forward",
  KeyS: "back",
  ArrowDown: "back",
  KeyA: "turnLeft",
  ArrowLeft: "turnLeft",
  KeyD: "turnRight",
  ArrowRight: "turnRight",
};
const WALK_SPEED = 1400; // world units/second, tuned relative to room scale
const TURN_SPEED = 2; // radians/second

window.addEventListener("keydown", (e) => {
  const dir = KEY_BINDINGS[e.code];
  if (dir) move[dir] = true;
});
window.addEventListener("keyup", (e) => {
  const dir = KEY_BINDINGS[e.code];
  if (dir) move[dir] = false;
});

// Same "walk, don't fly" behavior PointerLockControls provided: movement
// is always flattened onto the XZ plane regardless of look pitch.
const _forward = new THREE.Vector3();
function moveForward(distance) {
  camera.getWorldDirection(_forward);
  _forward.y = 0;
  _forward.normalize();
  camera.position.addScaledVector(_forward, distance);
}

const clock = new THREE.Clock();

function animate() {
  requestAnimationFrame(animate);
  const dt = Math.min(clock.getDelta(), 0.1);
  if (move.forward) moveForward(WALK_SPEED * dt);
  if (move.back) moveForward(-WALK_SPEED * dt);
  if (move.turnLeft) {
    yaw += TURN_SPEED * dt;
    applyLookRotation();
  }
  if (move.turnRight) {
    yaw -= TURN_SPEED * dt;
    applyLookRotation();
  }
  renderer.render(scene, camera);
}

// --- Model loading ------------------------------------------------------

const mtlLoader = new MTLLoader();
mtlLoader.setPath("/obj/");
mtlLoader.load(
  "eniact.mtl",
  (materials) => {
    materials.preload();
    const objLoader = new OBJLoader();
    objLoader.setMaterials(materials);
    objLoader.setPath("/obj/");
    objLoader.load(
      "eniact.obj",
      (object) => {
        // Rotation/position: eniacfp.cpp's own numbers
        // (node->setRotation(-90,180,0) / node->setPosition(-2300,-1600,4000),
        // Irrlicht, left-handed degrees) don't transfer as-is - the first
        // attempt at this (X=-90) produced a room shape that read fine in
        // isolated screenshots but was actually upside down (floor/ceiling
        // inverted) once walking around. X=+90 (below) is the corrected
        // sign, verified against the OBJ file's own named per-unit groups
        // (e.g. "eniac/s1/cyc", "eniac/s2/acc1") so that increasing scene Y
        // now actually means "up". eniacfp.cpp's raw world coordinates for
        // the camera and lamps still don't transfer directly even with
        // this fix (see toScene() in 3d-lamps.js and the starting-view
        // computation below) - those needed a separate empirical correction.
        object.rotation.set(
          THREE.MathUtils.degToRad(90),
          THREE.MathUtils.degToRad(180),
          0
        );
        object.position.set(-2300, -1600, 4000);
        object.updateMatrixWorld(true);
        object.traverse((child) => {
          if (child.isMesh && child.material) {
            const mats = Array.isArray(child.material) ? child.material : [child.material];
            for (const m of mats) m.side = THREE.DoubleSide;
          }
        });
        scene.add(object);

        const box = new THREE.Box3().setFromObject(object);
        window.__eniacBox = { min: box.min.toArray(), max: box.max.toArray() };

        // Starting view: stand near the initiating/cycling unit end, at
        // roughly eye height, looking down the room's long axis toward the
        // multiplier end. With the rotation fix above, the init/cycling
        // units sit near box.min.z (raw model "distance down the room" now
        // increases with scene Z), so that's the starting end, looking
        // toward box.max.z.
        const center = box.getCenter(new THREE.Vector3());
        const size = box.getSize(new THREE.Vector3());
        const eyeY = box.min.y + size.y * 0.6;
        camera.position.set(center.x, eyeY, box.min.z + size.z * 0.05);
        yaw = Math.PI; // face +Z (toward box.max.z)
        pitch = 0;
        applyLookRotation();

        window.__eniacModelLoaded = true;
      },
      undefined,
      (err) => console.error("OBJ load error", err)
    );
  },
  undefined,
  (err) => console.error("MTL load error", err)
);

animate();

// --- WebSocket session (mirrors web/app.js's session handling) ---------

const lampSystem = createLampSystem(scene);
window.__eniacLampSystem = lampSystem;

let ws = null;
const statusEl = document.getElementById("status");
const connectBtn = document.getElementById("connect-btn");
const programSelect = document.getElementById("program-select");
const cmdButtons = document.querySelectorAll("#hud button[data-cmd]");
const punchLog = document.getElementById("punchlog");

function setStatus(text, cls) {
  statusEl.textContent = text;
  statusEl.className = cls;
}

function setControlsEnabled(enabled) {
  cmdButtons.forEach((b) => (b.disabled = !enabled));
}

async function loadPrograms() {
  const res = await fetch("/api/programs");
  const names = await res.json();
  programSelect.innerHTML = "";
  for (const name of names) {
    const opt = document.createElement("option");
    opt.value = name;
    opt.textContent = name;
    programSelect.appendChild(opt);
  }
}

function startSession() {
  if (ws) {
    ws.close();
    ws = null;
  }
  punchLog.textContent = "";
  setControlsEnabled(false);

  const program = programSelect.value;
  if (!program) return;
  setStatus("connecting...", "status-connecting");

  const proto = location.protocol === "https:" ? "wss" : "ws";
  ws = new WebSocket(`${proto}://${location.host}/ws?program=${encodeURIComponent(program)}`);
  ws.onopen = () => {
    setStatus("connected", "status-connected");
    setControlsEnabled(true);
  };
  ws.onmessage = (ev) => {
    lampSystem.handleLine(ev.data, (cardText) => {
      punchLog.textContent += cardText + "\n";
      punchLog.parentElement.scrollTop = punchLog.parentElement.scrollHeight;
    });
  };
  ws.onclose = () => {
    setStatus("disconnected", "status-disconnected");
    setControlsEnabled(false);
    ws = null;
  };
  ws.onerror = () => ws && ws.close();
}

connectBtn.addEventListener("click", startSession);
cmdButtons.forEach((b) => {
  b.addEventListener("click", () => {
    if (ws && ws.readyState === WebSocket.OPEN) ws.send(b.dataset.cmd);
  });
});

setControlsEnabled(false);
loadPrograms();
