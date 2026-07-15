// ENIAC web control panel. Renders lamp state on top of the five section
// photos laid out side by side (see #panorama in index.html / SECTION_W
// below), and speaks the same line-oriented protocol the eniactk visualizer
// consumes (see vis/eniactk.go and src/engine.go). Lamp positions below are
// ported from eniactk.go's neonpos()/ray() (guimode 1-5, the flat per-section
// views) collapsed into one linear formula, since all 5 sections share one
// "world" coordinate system in units of 642 - see the implementation plan
// for the derivation.

const SECTION_W = 1900;
const SECTION_H = 950;
const SCALE = SECTION_W / (8 * 642);
const Y0 = SECTION_H / 4;

const accoff = [
  6 * 642, 7 * 642, 9 * 642, 10 * 642, 11 * 642, 12 * 642, 13 * 642, 14 * 642, 15 * 642,
  16 * 642, 20 * 642, 21 * 642, 22 * 642, 23 * 642,
  24 * 642, 25 * 642, 26 * 642, 27 * 642, 32 * 642, 33 * 642,
];
const ftuoff = [4, 28, 30];

const panorama = document.getElementById("panorama");
const lamps = new Map();

function setLamp(id, visible, xprime, yprime) {
  let el = lamps.get(id);
  if (!el) {
    el = document.createElement("div");
    el.className = "lamp";
    panorama.appendChild(el);
    lamps.set(id, el);
  }
  if (visible) {
    el.style.left = xprime * SCALE + "px";
    el.style.top = Y0 - yprime * SCALE + "px";
    el.style.display = "block";
  } else {
    el.style.display = "none";
  }
}

function neonpos(acc, dec, val) {
  return [accoff[acc] + dec * 49 + 63, 149 + val * 38];
}

// Static indicator lamps that are always lit, ported from eniactk's
// drawfixed(). They don't depend on any incoming message.
function drawFixed() {
  setLamp("ih1", true, 425, -1192);
  setLamp("ih2", true, 446, -1192);
  setLamp("cycst", true, 70 + 642, -132);
  setLamp("cych1", true, 406 + 642, -1192);
  setLamp("cych2", true, 429 + 642, -1192);
  setLamp("cych3", true, 456 + 642, -1192);
  setLamp("cych4", true, 479 + 642, -1192);
  for (let i = 1; i <= 20; i++) {
    setLamp(`acc${i}h1`, true, accoff[i - 1] + 395, -1196);
    setLamp(`acc${i}h2`, true, accoff[i - 1] + 420, -1196);
  }
  setLamp("ph1", true, 320 + 2 * 642, -1198);
  setLamp("ph2", true, 341 + 2 * 642, -1198);
  setLamp("ph3", true, 927 + 2 * 642, -1198);
  setLamp("ph4", true, 950 + 2 * 642, -1198);
  setLamp("dh1", true, 342 + 8 * 642, -1198);
  setLamp("dh2", true, 366 + 8 * 642, -1198);
  for (let i = 0; i < 3; i++) {
    setLamp(`mh${2 * i}`, true, 396 + (17 + i) * 642, -1196);
    setLamp(`mh${2 * i + 1}`, true, 420 + (17 + i) * 642, -1196);
  }
  setLamp("conh1", true, 427 + 34 * 642, -1197);
  setLamp("conh2", true, 448 + 34 * 642, -1197);
  setLamp("conh3", true, 1085 + 34 * 642, -1197);
  setLamp("conh4", true, 1106 + 34 * 642, -1197);
  setLamp("prh1", true, 1060 + 37 * 642, -1199);
  setLamp("prh2", true, 1082 + 37 * 642, -1199);
  for (let i = 0; i < 3; i++) {
    setLamp(`ft${i + 1}h1`, true, 428 + ftuoff[i] * 642, -1182);
    setLamp(`ft${i + 1}h2`, true, 450 + ftuoff[i] * 642, -1182);
    setLamp(`ft${i + 1}h3`, true, 1028 + ftuoff[i] * 642, -1182);
    setLamp(`ft${i + 1}h4`, true, 1052 + ftuoff[i] * 642, -1182);
  }
}

const cycleReadout = { cycle: "-", mode: "-" };
function renderCycleReadout() {
  document.getElementById("cycle-readout").textContent =
    `cycle: ${cycleReadout.cycle}   mode: ${cycleReadout.mode}`;
}

const handlers = {
  // Accumulators
  ad(p) {
    const unit = +p[1], dig = +p[2], d = +p[3];
    if (dig === 0) {
      setLamp(`accs${unit}`, true, accoff[unit] + 59, 149 + 38 * d);
    } else {
      const [x, y] = neonpos(unit, dig - 1, d);
      setLamp(`a${unit}d${dig - 1}`, true, x, y);
    }
  },
  ac(p) {
    const unit = +p[1], dig = +p[2], d = +p[3];
    setLamp(`aff${unit}d${dig}`, d === 1, accoff[unit] + dig * 49 + 63, -31);
  },
  ar(p) {
    const unit = +p[1], rep = +p[2];
    setLamp(`acc${unit}rep`, true, accoff[unit] + 90, -1310 + 20 * rep);
  },
  af(p) {
    const unit = +p[1], prog = +p[2], val = +p[3];
    const lit = val === 1;
    switch (prog) {
      case 0: setLamp(`acc${unit}ff1`, lit, accoff[unit] + 135, -1130); break;
      case 1: setLamp(`acc${unit}ff2`, lit, accoff[unit] + 135, -1150); break;
      case 2: setLamp(`acc${unit}ff3`, lit, accoff[unit] + 180, -1130); break;
      case 3: setLamp(`acc${unit}ff4`, lit, accoff[unit] + 180, -1150); break;
      default: setLamp(`acc${unit}ff${prog + 1}`, lit, accoff[unit] + 225 + 45 * (prog - 4), -1130);
    }
  },

  // Cycling unit
  cy(p) {
    const n = +p[1];
    setLamp("cycst", true, 122 + 642 + Math.floor(n / 2) * 20, 40);
    setLamp("cyccg", n >= 22 && n <= 36, 1010, -18);
    setLamp("cy10p", n < 20 && n % 2 === 1, 884, -18);
    cycleReadout.cycle = n;
    renderCycleReadout();
  },
  cm(p) {
    cycleReadout.mode = { P: "1 Pulse", A: "1 Add", C: "Cont." }[p[1]] || p[1];
    renderCycleReadout();
  },

  // Divider / square rooter
  d(p) {
    const plring = +p[1], prring = +p[2], progff = p[3], flags = p[4];
    const at = (n) => flags[n] === "1";
    setLamp("dplr", true, 8 * 642 + 88, -320 + 15 * plring);
    setLamp("dprr", true, 8 * 642 + 534, -1346 + 15 * prring);
    for (let i = 0; i < 8; i++) {
      const xs = [94, 139, 184, 229, 358, 403, 448, 493];
      setLamp(`dprogff${i + 1}`, progff[i] === "1", 8 * 642 + xs[i], -1156);
    }
    setLamp("ddivff", at(0), 8 * 642 + 417, 39);
    setLamp("dclrff", at(1), 8 * 642 + 400, 39);
    setLamp("dilockff", at(2), 8 * 642 + 383, 39);
    setLamp("ddpgamma", at(3), 8 * 642 + 356, 39);
    setLamp("dngamma", at(4), 8 * 642 + 339, 39);
    setLamp("dpsrcff", flags[5] === "0", 8 * 642 + 286, 39);
    setLamp("dpringff", flags[6] === "0", 8 * 642 + 269, 39);
    setLamp("ddenomff", flags[7] === "0", 8 * 642 + 252, 39);
    setLamp("dnumrplus", at(8), 8 * 642 + 235, 39);
    // Ported as-is from eniactk.go, which re-reads flags[0] here rather than
    // a distinct "numrmin" index - a pre-existing quirk in the reference
    // visualizer, kept for parity rather than silently "corrected".
    setLamp("dnumrmin", at(0), 8 * 642 + 218, 39);
    setLamp("dqalpha", at(10), 8 * 642 + 140, -135);
    setLamp("dsac", at(11), 8 * 642 + 186, -135);
    setLamp("dm2", at(12), 8 * 642 + 234, -135);
    setLamp("dm1", at(13), 8 * 642 + 286, -135);
    setLamp("dnac", at(14), 8 * 642 + 334, -135);
    setLamp("dda", at(15), 8 * 642 + 384, -135);
    setLamp("dnalpha", at(16), 8 * 642 + 430, -135);
    setLamp("ddalpha", at(17), 8 * 642 + 480, -135);
    setLamp("dans2", at(27), 8 * 642 + 530, -135);
    setLamp("dans4", at(29), 8 * 642 + 578, -135);
    setLamp("ddgamma", at(18), 8 * 642 + 140, -156);
    setLamp("dnpgamma", at(19), 8 * 642 + 186, -156);
    setLamp("dp2", at(20), 8 * 642 + 234, -156);
    setLamp("dp1", at(21), 8 * 642 + 286, -156);
    setLamp("dsalpha", at(22), 8 * 642 + 334, -156);
    setLamp("dds", at(23), 8 * 642 + 384, -156);
    setLamp("dnbeta", at(24), 8 * 642 + 430, -156);
    setLamp("ddbeta", at(25), 8 * 642 + 480, -156);
    setLamp("dans1", at(26), 8 * 642 + 530, -156);
    setLamp("dans3", at(28), 8 * 642 + 580, -156);
  },

  // Multiplier
  m(p) {
    const stage = +p[1], flags = p[2], mr1 = +p[3], mr3 = +p[4];
    setLamp("mstage", true, 18 * 642 + 188 + stage * 20, -36);
    setLamp("mr1", mr1 === 1, 17 * 642 + 312, 29);
    setLamp("mr3", mr3 === 1, 19 * 642 + 351, 29);
    for (let i = 0; i < 24; i++) {
      let xpos = 642 * (17 + Math.floor(i / 8)) + 92 + 41 * (i % 8);
      if (i % 8 >= 4) xpos += 255;
      setLamp(`mi${i + 1}`, flags[i] === "1", xpos, -1156);
    }
  },

  // Master programmer
  mps(p) {
    const stage = +p[1], val = +p[2];
    const x = stage < 5 ? 82 + 99 * stage + 2 * 642 : 723 + 99 * (stage - 5) + 2 * 642;
    setLamp(`ps${stage}`, true, x, -223 + val * 19);
  },
  mpi(p) {
    const stage = +p[1], val = +p[2];
    const x = stage < 5 ? 95 + stage * 82 + 2 * 642 : 695 + (stage - 5) * 82 + 2 * 642;
    setLamp(`pi${stage}`, val > 0, x, -1130);
  },
  mpd(p) {
    const decade = +p[1], val = +p[2];
    const x = decade < 10 ? 131 + 49 * decade + 2 * 642 : 774 + 49 * (decade - 10) + 2 * 642;
    setLamp(`pd${decade}`, true, x, 191 + val * 19);
  },

  // Function tables
  ftar(p) {
    const unit = +p[1], arg = +p[2];
    setLamp(`ft${unit}a1`, true, 88 + ftuoff[unit] * 642 + 20 * (arg % 10), 40);
    setLamp(`ft${unit}a10`, true, 347 + ftuoff[unit] * 642 + 20 * Math.floor(arg / 10), 40);
  },
  ftr(p) {
    const unit = +p[1], ring = +p[2] + 3;
    setLamp(`ft${unit}r`, true, 308 + ftuoff[unit] * 642 + 20 * ring, -25);
  },
  ftad(p) {
    const unit = +p[1], val = +p[2];
    setLamp(`ft${unit}add`, val === 1, 190 + ftuoff[unit] * 642, -25);
  },
  ftsu(p) {
    const unit = +p[1], val = +p[2];
    setLamp(`ft${unit}subt`, val === 1, 210 + ftuoff[unit] * 642, -25);
  },
  ftse(p) {
    const unit = +p[1], val = +p[2];
    setLamp(`ft${unit}aset`, val === 1, 107 + ftuoff[unit] * 642, -25);
  },

  // Constant transmitter. Note: engine.go emits this with prefix "ct" (see
  // src/engine.go), not "c" - eniactk.go's own "c" case is unreachable dead
  // code in the reference implementation, so we wire up the real prefix.
  ct(p) {
    const prog = +p[1], ff = +p[2];
    const row = Math.floor(prog / 10), col = prog % 10;
    const x = 90 + 34 * 642 + Math.round(col * 48.6);
    const y = [362, -119, -1150][row];
    setLamp(`ct${prog}`, ff === 1, x, y);
  },

  // Punch/printer output
  punch(p, raw) {
    const log = document.getElementById("punchlog");
    log.textContent += raw.slice(6) + "\n";
    log.parentElement.scrollTop = log.parentElement.scrollHeight;
  },
};

function handleLine(line) {
  const p = line.split(" ");
  const fn = handlers[p[0]];
  if (fn) fn(p, line);
}

// --- WebSocket session management -----------------------------------------

let ws = null;
const statusEl = document.getElementById("status");
const connectBtn = document.getElementById("connect-btn");
const programSelect = document.getElementById("program-select");
const cmdButtons = document.querySelectorAll("button[data-cmd]");

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
  lamps.forEach((el) => (el.style.display = "none"));
  document.getElementById("punchlog").textContent = "";
  cycleReadout.cycle = "-";
  cycleReadout.mode = "-";
  renderCycleReadout();
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
  ws.onmessage = (ev) => handleLine(ev.data);
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
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(b.dataset.cmd);
    }
  });
});

drawFixed();
setControlsEnabled(false);
loadPrograms();
