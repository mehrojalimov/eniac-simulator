// Live lamp state for the 3D view, ported from vis/eniacfp.cpp's
// makeneons()/stdinreader() (mkneon/mvneon call sites). Positions are world
// coordinates in the same space eniacfp.cpp's camera/lights use - they are
// NOT relative to the OBJ mesh's own local space, so they only line up
// visually once the mesh transform in 3d.js is calibrated to match.
//
// Three known copy-paste bugs in eniacfp.cpp are intentionally NOT
// reproduced here (there is no reason to port a bug when building fresh):
//   - "ftse" moved ftsubneon instead of a dedicated "set" lamp.
//   - the multiplier's third lamp ("m" message) used mr1's value for both
//     lamp positions instead of mr1/mr3 respectively.
//   - the divider's 30th dsqstat lamp indexed one character past the end
//     of the (29-char) status string.
// Also, like eniacfp.cpp itself, this does not visualize accumulator
// repeat-count ("ar") or program-control ("af") lamps, or constant
// transmitter positions 20-29 - eniacfp.cpp never covered those either,
// and there's no reference 3D placement to port for them.

import * as THREE from "three";

const laccmpos = [7725, 8335, 9555, 10165, 10775, 11385, 11995, 12605, 13215];
const baccmpos = [-2230, 210, 820, 1430, 2040];
const raccmpos = [13690, 13080, 12470, 11860, 8810, 8200];

function makeLampTexture() {
  const size = 64;
  const canvas = document.createElement("canvas");
  canvas.width = canvas.height = size;
  const ctx = canvas.getContext("2d");
  const grad = ctx.createRadialGradient(
    size / 2, size / 2, 0,
    size / 2, size / 2, size / 2
  );
  grad.addColorStop(0, "rgba(255,230,180,1)");
  grad.addColorStop(0.35, "rgba(255,150,0,0.95)");
  grad.addColorStop(1, "rgba(255,150,0,0)");
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, size, size);
  return new THREE.CanvasTexture(canvas);
}

// Converts an eniacfp.cpp world coordinate into this scene's coordinate
// space. eniacfp.cpp's raw numbers don't transfer directly (same issue as
// the camera start position - Irrlicht's rotation convention differs from
// Three.js's), so this was derived empirically: the OBJ file has named
// groups per physical unit (e.g. "eniac/s1/cyc", "eniac/s2/acc1"), which
// gave ground-truth positions to solve for the correction against. Verified
// against two independent landmarks (cycling unit and accumulator 1): the
// resulting X-axis (which side of the room) matched within ~15 units and
// the Z-axis (depth down the room) within ~1-2% of the room's total
// length, out of a room that's roughly 7000x2600x11000 units. Y and Z were
// re-derived after fixing the mesh's up/down inversion in 3d.js (the
// rotation's X component flipped from -90 to +90) - X is unaffected by
// that fix, since it only touches the vertical/depth axes.
function toScene(ex, ey, ez) {
  return [-ex - 4535, ey - 1600, ez];
}

export function createLampSystem(scene) {
  const texture = makeLampTexture();
  const sprites = new Map();

  function setLamp(id, ex, ey, ez) {
    const [x, y, z] = toScene(ex, ey, ez);
    let s = sprites.get(id);
    if (!s) {
      const material = new THREE.SpriteMaterial({
        map: texture,
        transparent: true,
        depthWrite: false,
        depthTest: false,
        blending: THREE.AdditiveBlending,
      });
      s = new THREE.Sprite(material);
      s.scale.set(90, 90, 1);
      scene.add(s);
      sprites.set(id, s);
    }
    s.position.set(x, y, z);
  }

  function accAD(unit, digit, val) {
    if (unit < 9) return [-2690, 390 + val * 35, laccmpos[unit] + 47 * digit];
    if (unit < 14) return [baccmpos[unit - 9] + 47 * digit, 390 + val * 35, 14150];
    return [2972, 390 + val * 35, raccmpos[unit - 14] - 47 * digit];
  }
  function accAC(unit, digit, val) {
    if (unit < 9) return [-2690, -200 + val * 427, laccmpos[unit] + 47 * digit];
    if (unit < 14) return [baccmpos[unit - 9] + 47 * digit, -200 + val * 427, 14150];
    return [2972, -200 + val * 427, raccmpos[unit - 14] - 47 * digit];
  }

  const ftGeom = [
    { dir: 1, xpos: -2645, xposCtl: -2745, ystart: 6520, ystartRing: 6737, ystartAdd: 6612, ystartSub: 6640 },
    { dir: -1, xpos: 2922, xposCtl: 3022, ystart: 11230, ystartRing: 11010, ystartAdd: 11138, ystartSub: 11110 },
    { dir: -1, xpos: 2922, xposCtl: 3022, ystart: 10010, ystartRing: 9790, ystartAdd: 9918, ystartSub: 9890 },
  ];

  const handlers = {
    ad(p) {
      const unit = +p[1], digit = +p[2], val = +p[3];
      setLamp(`ad${unit}_${digit}`, ...accAD(unit, digit, val));
    },
    ac(p) {
      const unit = +p[1], digit = +p[2], val = +p[3];
      setLamp(`ac${unit}_${digit}`, ...accAC(unit, digit, val));
    },
    cy(p) {
      let val = +p[1];
      val &= ~1;
      setLamp("cycst", -2645, 280, 4732 + 9.6 * val);
      if (val <= 20) setLamp("cycst2", -2645, 200, 4866);
      else if (val <= 36) setLamp("cycst2", -2645, 200, 4965);
      else setLamp("cycst2", -2800, 200, 4866);
    },
    mpd(p) {
      const digit = +p[1], val = +p[2];
      if (digit < 10) setLamp(`mpd${digit}`, -2745, 475 + 20 * val, 5368 + 40 * digit);
      else setLamp(`mpd${digit}`, -2745, 475 + 20 * val, 5970 + 40 * (digit - 10));
    },
    mps(p) {
      const digit = +p[1], val = +p[2];
      if (digit < 5) setLamp(`mps${digit}`, -2745, 80 + 20 * val, 5367 + 75 * digit);
      else setLamp(`mps${digit}`, -2745, 80 + 20 * val, 5973 + 75 * (digit - 5));
    },
    ftar(p) {
      const unit = +p[1], val = +p[2];
      const g = ftGeom[unit];
      setLamp(`ftone${unit}`, g.xpos, 300, g.ystart + Math.floor(val / 10) * 19.2 * g.dir);
      setLamp(`ftten${unit}`, g.xpos, 300, g.ystart + (val % 10) * 19.2 * g.dir + 250 * g.dir);
    },
    ftr(p) {
      const unit = +p[1], val = +p[2] + 3;
      const g = ftGeom[unit];
      setLamp(`ftring${unit}`, g.xpos, 245, g.ystartRing + val * 18.5 * g.dir);
    },
    ftad(p) {
      const unit = +p[1], val = +p[2];
      const g = ftGeom[unit];
      setLamp(`ftadd${unit}`, g.xposCtl + (unit === 0 ? val : -val) * 100, 245, g.ystartAdd);
    },
    ftsu(p) {
      const unit = +p[1], val = +p[2];
      const g = ftGeom[unit];
      setLamp(`ftsub${unit}`, g.xposCtl + (unit === 0 ? val : -val) * 100, 245, g.ystartSub);
    },
    ftse(p) {
      const unit = +p[1], val = +p[2];
      const g = ftGeom[unit];
      setLamp(`ftset${unit}`, g.xposCtl + (unit === 0 ? val : -val) * 100, 245, g.ystart);
    },
    ct(p) {
      const digit = +p[1], val = +p[2];
      if (digit < 10) setLamp(`ct${digit}`, 3095 - 100 * val, 660, 7570 - 49 * (digit - 1));
      else if (digit < 20) setLamp(`ct${digit}`, 3095 - 100 * val, 204, 7570 - 49 * (digit - 11));
      // digits 20-29 aren't visualized in 3D - see file header note.
    },
    m(p) {
      const ms = +p[1], mr1 = +p[3], mr3 = +p[4];
      setLamp("muls", -910 + 20 * ms, 225, 14090);
      setLamp("mulr1", -1400, 230, 14190 - mr1 * 100);
      setLamp("mulr3", -180, 230, 14190 - mr3 * 100);
    },
    d(p) {
      const placering = +p[1];
      const flags = p[4];
      setLamp("dsqpl", -2720, 190 - placering * 20, 8970);
      // Explicit z values (not a uniform step - the first row's last two
      // entries break the +21 pattern in the source), one row per
      // [x, y, z-values[]] ported directly from eniacfp.cpp's mvdsqstat
      // call sites. The 30th entry (index 29) in the original indexed one
      // character past the 29-char flags string - dropped here rather
      // than ported, see file header note.
      const rows = [
        [-2720, 370, [9080, 9101, 9122, 9143, 9164, 9185, 9206, 9227, 9238, 9379]],
        [-2820, 190, [9010, 9055, 9100, 9145, 9190, 9235, 9280, 9325, 9370, 9415]],
        [-2820, 165, [9010, 9055, 9100, 9145, 9190, 9235, 9280, 9325, 9370]],
      ];
      let idx = 0;
      for (const [x, y, zs] of rows) {
        for (const z of zs) {
          const bit = flags[idx] === "1" ? 1 : 0;
          setLamp(`dsq${idx}`, x + 100 * bit, y, z);
          idx++;
        }
      }
    },
  };

  function handleLine(line, onPunch) {
    const p = line.split(" ");
    if (p[0] === "punch") {
      if (onPunch) onPunch(line.slice(6));
      return;
    }
    const fn = handlers[p[0]];
    if (fn) fn(p);
  }

  return { handleLine, lampCount: () => sprites.size };
}
