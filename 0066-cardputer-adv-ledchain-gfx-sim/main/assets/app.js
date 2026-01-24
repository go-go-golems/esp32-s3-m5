function $(id) { return document.getElementById(id); }
function qsa(sel, root) { return Array.prototype.slice.call((root || document).querySelectorAll(sel)); }

function safeJsonParse(s) {
  try { return JSON.parse(s); } catch (e) { return null; }
}

const state = {
  pollMs: 500,
  pollTimer: 0,
  wsOk: false,
  activeScreen: 'dashboard',
  lastStatus: null,
  lastPollAt: 0,
  lastFrameAt: 0,
  fps: 0,
  presets: [],
};

function setWsPill(ok, msg) {
  const pill = $('ws_status');
  if (!pill) return;
  pill.classList.remove('ok', 'bad');
  pill.classList.add(ok ? 'ok' : 'bad');
  pill.textContent = `ws: ${msg}`;
  state.wsOk = !!ok;
  if ($('dash_ws')) $('dash_ws').textContent = ok ? `connected (${msg})` : msg;
}

function nowIsoLocal() {
  const d = new Date();
  const pad = (x) => String(x).padStart(2, '0');
  return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
}

function setLastUpdate() {
  if ($('last_update')) $('last_update').textContent = `Last update: ${nowIsoLocal()}`;
}

function setHost() {
  const h = `${location.protocol}//${location.host}`;
  if ($('host')) $('host').textContent = `Host: ${h}`;
  if ($('dash_host')) $('dash_host').textContent = h;
}

function setPollLabel() {
  if ($('dash_poll')) $('dash_poll').textContent = `every ${state.pollMs}ms`;
}

function showScreen(name) {
  qsa('.tab').forEach((b) => b.classList.toggle('active', b.dataset.screen === name));
  qsa('.screen').forEach((s) => s.classList.toggle('hidden', s.id !== `screen_${name}`));
  state.activeScreen = String(name || 'dashboard');
  if (name === 'patterns') renderPatternEditor(getPatternType());
}

function bindTabs() {
  qsa('.tab').forEach((b) => {
    b.addEventListener('click', () => {
      const name = b.dataset.screen;
      showScreen(name);
      history.replaceState(null, '', `#${name}`);
    });
  });
}

function getNumber(id, fallback) {
  const el = $(id);
  if (!el) return fallback;
  const v = Number(el.value);
  return Number.isFinite(v) ? v : fallback;
}

function clamp(v, lo, hi) {
  if (!Number.isFinite(v)) return lo;
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

function getPatternType() {
  const el = $('pat_type') || $('qc_type');
  return el ? String(el.value || 'off') : 'off';
}

function setValueIfNotFocused(el, next) {
  if (!el) return;
  if (document.activeElement === el) return;
  el.value = String(next);
}

function syncPatternType(type) {
  setValueIfNotFocused($('qc_type'), type);
  setValueIfNotFocused($('pat_type'), type);
  renderPatternEditor(type);
}

function syncGlobalsFromStatus(s) {
  if (!s || !s.pattern) return;
  const type = s.pattern.type || 'off';
  const bri = s.pattern.global_brightness_pct || 25;
  const frameMs = s.frame_ms || 16;
  setValueIfNotFocused($('qc_type'), type);
  setValueIfNotFocused($('pat_type'), type);
  setValueIfNotFocused($('qc_bri'), bri);
  setValueIfNotFocused($('pat_bri'), bri);
  setValueIfNotFocused($('qc_frame_ms'), frameMs);
  setValueIfNotFocused($('pat_frame_ms'), frameMs);
}

function summarizeStatus(s) {
  if (!s || !s.pattern) return '(no status yet)';
  const p = s.pattern;
  const bits = [];
  bits.push(`Pattern: ${p.type}`);
  bits.push(`Brightness: ${p.global_brightness_pct}%`);
  bits.push(`Frame: ${s.frame_ms}ms`);
  bits.push(`LED count: ${s.led_count}`);
  if (p.rainbow) bits.push(`Params (rainbow): speed=${p.rainbow.speed} sat=${p.rainbow.sat} spread=${p.rainbow.spread}`);
  if (p.chase) bits.push(`Params (chase): speed=${p.chase.speed} tail=${p.chase.tail} gap=${p.chase.gap} trains=${p.chase.trains} dir=${p.chase.dir} fade=${p.chase.fade} fg=${p.chase.fg} bg=${p.chase.bg}`);
  if (p.breathing) bits.push(`Params (breathing): speed=${p.breathing.speed} color=${p.breathing.color} min=${p.breathing.min} max=${p.breathing.max} curve=${p.breathing.curve}`);
  if (p.sparkle) bits.push(`Params (sparkle): speed=${p.sparkle.speed} density=${p.sparkle.density} fade=${p.sparkle.fade} mode=${p.sparkle.mode} color=${p.sparkle.color} bg=${p.sparkle.bg}`);
  return bits.join('\n');
}

async function fetchStatus() {
  const res = await fetch('/api/status', { cache: 'no-store' });
  const j = await res.json();
  state.lastStatus = j;
  setLastUpdate();
  syncGlobalsFromStatus(j);
  if ($('dash_status')) $('dash_status').textContent = summarizeStatus(j);
  if ($('sys_status')) $('sys_status').textContent = JSON.stringify(j, null, 2);
  return j;
}

function drawPreview(canvasId, bytes, ledCount) {
  const el = $(canvasId);
  if (!el) return;
  const ctx = el.getContext('2d');
  const cols = 10;
  const rows = Math.max(1, Math.ceil(ledCount / cols));
  const w = el.width;
  const h = el.height;
  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = '#07101f';
  ctx.fillRect(0, 0, w, h);

  const pad = 3;
  const cellW = Math.floor(w / cols);
  const cellH = Math.floor(h / rows);
  for (let i = 0; i < ledCount; i++) {
    const col = i % cols;
    const row = Math.floor(i / cols);
    const x0 = col * cellW;
    const y0 = row * cellH;
    const r = bytes[i * 3 + 0] || 0;
    const g = bytes[i * 3 + 1] || 0;
    const b = bytes[i * 3 + 2] || 0;
    ctx.fillStyle = `rgb(${r},${g},${b})`;
    ctx.fillRect(x0 + pad, y0 + pad, cellW - 2 * pad, cellH - 2 * pad);
    ctx.strokeStyle = '#1f2a44';
    ctx.strokeRect(x0 + pad, y0 + pad, cellW - 2 * pad, cellH - 2 * pad);
  }
}

function updateFps() {
  const now = performance.now();
  const dt = now - (state.lastFrameAt || now);
  state.lastFrameAt = now;
  const inst = (dt > 1) ? (1000.0 / dt) : 0;
  state.fps = state.fps ? (0.85 * state.fps + 0.15 * inst) : inst;
  const s = state.fps ? state.fps.toFixed(0) : '-';
  if ($('dash_fps')) $('dash_fps').textContent = s;
  if ($('pat_fps')) $('pat_fps').textContent = s;
}

async function fetchFrame() {
  const res = await fetch('/api/frame', { cache: 'no-store' });
  const buf = await res.arrayBuffer();
  const bytes = new Uint8Array(buf);
  const ledCount = Number(res.headers.get('x-led-count') || bytes.length / 3) || (bytes.length / 3);
  drawPreview('dash_preview', bytes, ledCount);
  drawPreview('pat_preview', bytes, ledCount);
  updateFps();
}

async function postApply(body) {
  const res = await fetch('/api/apply', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify(body),
  });
  const j = await res.json().catch(() => ({ ok: false }));
  state.lastStatus = j;
  setLastUpdate();
  syncGlobalsFromStatus(j);
  if ($('dash_status')) $('dash_status').textContent = summarizeStatus(j);
  if ($('sys_status')) $('sys_status').textContent = JSON.stringify(j, null, 2);
  return j;
}

let applyTimer = 0;
function scheduleApply(reason) {
  // `reason` is for future debugging/telemetry; keep the signature stable.
  if (applyTimer) clearTimeout(applyTimer);
  applyTimer = setTimeout(() => {
    applyTimer = 0;
    applyNow().catch(() => {});
  }, 250);
}

function isAutoApplyOn() {
  const qc = $('qc_auto');
  const pat = $('pat_auto');
  return !!((qc && qc.checked) || (pat && pat.checked));
}

function buildApplyBody(includePatternParams) {
  const type = getPatternType();
  const bri = clamp(getNumber($('pat_bri') ? 'pat_bri' : 'qc_bri', 25), 1, 100);
  const frameMs = clamp(getNumber($('pat_frame_ms') ? 'pat_frame_ms' : 'qc_frame_ms', 16), 1, 1000);
  const body = { type, brightness_pct: bri, frame_ms: frameMs };
  if (!includePatternParams) return body;

  if (type === 'rainbow') {
    body.rainbow = {
      speed: clamp(getNumber('rain_speed', 5), 0, 20),
      sat: clamp(getNumber('rain_sat', 100), 0, 100),
      spread: clamp(getNumber('rain_spread', 10), 1, 50),
    };
  } else if (type === 'chase') {
    body.chase = {
      speed: clamp(getNumber('ch_speed', 128), 0, 255),
      tail: clamp(getNumber('ch_tail', 40), 1, 255),
      gap: clamp(getNumber('ch_gap', 8), 0, 255),
      trains: clamp(getNumber('ch_trains', 2), 1, 255),
      fg: String(($('ch_fg') && $('ch_fg').value) || '#FF8800'),
      bg: String(($('ch_bg') && $('ch_bg').value) || '#001020'),
      dir: clamp(getNumber('ch_dir', 0), 0, 2),
      fade: !!($('ch_fade') && $('ch_fade').checked),
    };
  } else if (type === 'breathing') {
    body.breathing = {
      speed: clamp(getNumber('br_speed', 6), 0, 20),
      color: String(($('br_color') && $('br_color').value) || '#00FFFF'),
      min: clamp(getNumber('br_min', 20), 0, 255),
      max: clamp(getNumber('br_max', 220), 0, 255),
      curve: clamp(getNumber('br_curve', 0), 0, 2),
    };
  } else if (type === 'sparkle') {
    body.sparkle = {
      speed: clamp(getNumber('sp_speed', 8), 0, 20),
      density: clamp(getNumber('sp_density', 55), 0, 100),
      fade: clamp(getNumber('sp_fade', 160), 1, 255),
      mode: clamp(getNumber('sp_mode', 1), 0, 2),
      color: String(($('sp_color') && $('sp_color').value) || '#FFFFFF'),
      bg: String(($('sp_bg') && $('sp_bg').value) || '#000010'),
    };
  }
  return body;
}

async function applyNow() {
  const include = (state.activeScreen === 'patterns');
  const body = buildApplyBody(include);
  return postApply(body);
}

function bindGlobalControls() {
  if ($('poll_ms')) {
    $('poll_ms').addEventListener('change', () => {
      state.pollMs = clamp(getNumber('poll_ms', 500), 100, 10000);
      setPollLabel();
      restartPoll();
    });
  }
  if ($('poll_now')) $('poll_now').addEventListener('click', () => pollOnce().catch(() => {}));

  const onGlobalsChanged = () => { if (isAutoApplyOn()) scheduleApply('globals'); };
  ['qc_type', 'qc_bri', 'qc_frame_ms'].forEach((id) => {
    const el = $(id);
    if (!el) return;
    el.addEventListener('change', () => {
      if (id === 'qc_type') syncPatternType(String(el.value || 'off'));
      onGlobalsChanged();
    });
  });
  ['pat_type', 'pat_bri', 'pat_frame_ms'].forEach((id) => {
    const el = $(id);
    if (!el) return;
    el.addEventListener('change', () => {
      if (id === 'pat_type') syncPatternType(String(el.value || 'off'));
      onGlobalsChanged();
    });
  });

  if ($('qc_apply')) $('qc_apply').addEventListener('click', () => applyNow().catch(() => {}));
  if ($('pat_apply')) $('pat_apply').addEventListener('click', () => applyNow().catch(() => {}));
  if ($('qc_reset')) $('qc_reset').addEventListener('click', () => syncGlobalsFromStatus(state.lastStatus || {}));
  if ($('pat_reset')) $('pat_reset').addEventListener('click', () => syncGlobalsFromStatus(state.lastStatus || {}));
}

function renderPatternEditor(type) {
  const host = $('pat_editor');
  if (!host) return;
  const t = String(type || 'off');

  if (t === 'off') {
    host.innerHTML = '<div class="muted">Pattern <span class="mono">off</span> has no parameters.</div>';
    return;
  }

  if (t === 'rainbow') {
    host.innerHTML =
      '<div class="kv">' +
      '<div class="k">Speed</div><div class="v"><input id="rain_speed" type="number" min="0" max="20" value="5" style="width:100px" /> <span class="muted">(0..20)</span></div>' +
      '<div class="k">Sat</div><div class="v"><input id="rain_sat" type="number" min="0" max="100" value="100" style="width:100px" /> <span class="muted">(0..100)</span></div>' +
      '<div class="k">Spread</div><div class="v"><input id="rain_spread" type="number" min="1" max="50" value="10" style="width:100px" /> <span class="muted">(1..50)</span></div>' +
      '</div>';
  } else if (t === 'chase') {
    host.innerHTML =
      '<div class="kv">' +
      '<div class="k">Speed</div><div class="v"><input id="ch_speed" type="number" min="0" max="255" value="128" style="width:120px" /></div>' +
      '<div class="k">Tail</div><div class="v"><input id="ch_tail" type="number" min="1" max="255" value="40" style="width:120px" /></div>' +
      '<div class="k">Gap</div><div class="v"><input id="ch_gap" type="number" min="0" max="255" value="8" style="width:120px" /></div>' +
      '<div class="k">Trains</div><div class="v"><input id="ch_trains" type="number" min="1" max="255" value="2" style="width:120px" /></div>' +
      '<div class="k">Dir</div><div class="v"><select id="ch_dir"><option value="0">forward</option><option value="1">reverse</option><option value="2">bounce</option></select></div>' +
      '<div class="k">Fade</div><div class="v"><label class="muted"><input id="ch_fade" type="checkbox" checked /> fade tail</label></div>' +
      '<div class="k">FG</div><div class="v"><input id="ch_fg" type="color" value="#ff8800" /></div>' +
      '<div class="k">BG</div><div class="v"><input id="ch_bg" type="color" value="#001020" /></div>' +
      '</div>';
  } else if (t === 'breathing') {
    host.innerHTML =
      '<div class="kv">' +
      '<div class="k">Speed</div><div class="v"><input id="br_speed" type="number" min="0" max="20" value="6" style="width:100px" /> <span class="muted">(0..20)</span></div>' +
      '<div class="k">Color</div><div class="v"><input id="br_color" type="color" value="#00ffff" /></div>' +
      '<div class="k">Min</div><div class="v"><input id="br_min" type="number" min="0" max="255" value="20" style="width:100px" /></div>' +
      '<div class="k">Max</div><div class="v"><input id="br_max" type="number" min="0" max="255" value="220" style="width:100px" /></div>' +
      '<div class="k">Curve</div><div class="v"><select id="br_curve"><option value="0">sine</option><option value="1">linear</option><option value="2">ease</option></select></div>' +
      '</div>';
  } else if (t === 'sparkle') {
    host.innerHTML =
      '<div class="kv">' +
      '<div class="k">Speed</div><div class="v"><input id="sp_speed" type="number" min="0" max="20" value="8" style="width:100px" /> <span class="muted">(0..20)</span></div>' +
      '<div class="k">Density %</div><div class="v"><input id="sp_density" type="number" min="0" max="100" value="55" style="width:100px" /></div>' +
      '<div class="k">Fade</div><div class="v"><input id="sp_fade" type="number" min="1" max="255" value="160" style="width:100px" /></div>' +
      '<div class="k">Mode</div><div class="v"><select id="sp_mode"><option value="0">fixed</option><option value="1">random</option><option value="2">rainbow</option></select></div>' +
      '<div class="k">Color</div><div class="v"><input id="sp_color" type="color" value="#ffffff" /></div>' +
      '<div class="k">BG</div><div class="v"><input id="sp_bg" type="color" value="#000010" /></div>' +
      '</div>';
  } else {
    host.textContent = '(unknown pattern)';
  }

  // Seed from last status if it matches type (best-effort).
  const s = state.lastStatus;
  if (s && s.pattern && s.pattern.type === t && s.pattern[t]) {
    const p = s.pattern[t];
    if (t === 'rainbow') {
      if ($('rain_speed')) $('rain_speed').value = p.speed;
      if ($('rain_sat')) $('rain_sat').value = p.sat;
      if ($('rain_spread')) $('rain_spread').value = p.spread;
    } else if (t === 'chase') {
      if ($('ch_speed')) $('ch_speed').value = p.speed;
      if ($('ch_tail')) $('ch_tail').value = p.tail;
      if ($('ch_gap')) $('ch_gap').value = p.gap;
      if ($('ch_trains')) $('ch_trains').value = p.trains;
      if ($('ch_dir')) $('ch_dir').value = p.dir;
      if ($('ch_fade')) $('ch_fade').checked = !!p.fade;
      if ($('ch_fg')) $('ch_fg').value = String(p.fg || '#ff8800').toLowerCase();
      if ($('ch_bg')) $('ch_bg').value = String(p.bg || '#001020').toLowerCase();
    } else if (t === 'breathing') {
      if ($('br_speed')) $('br_speed').value = p.speed;
      if ($('br_color')) $('br_color').value = String(p.color || '#00ffff').toLowerCase();
      if ($('br_min')) $('br_min').value = p.min;
      if ($('br_max')) $('br_max').value = p.max;
      if ($('br_curve')) $('br_curve').value = p.curve;
    } else if (t === 'sparkle') {
      if ($('sp_speed')) $('sp_speed').value = p.speed;
      if ($('sp_density')) $('sp_density').value = p.density;
      if ($('sp_fade')) $('sp_fade').value = p.fade;
      if ($('sp_mode')) $('sp_mode').value = p.mode;
      if ($('sp_color')) $('sp_color').value = String(p.color || '#ffffff').toLowerCase();
      if ($('sp_bg')) $('sp_bg').value = String(p.bg || '#000010').toLowerCase();
    }
  }

  // Auto-apply when editing params.
  qsa('input,select', host).forEach((el) => {
    el.addEventListener('change', () => { if (isAutoApplyOn()) scheduleApply('pattern-param'); });
  });
}

async function jsEvalRaw(code) {
  const res = await fetch('/api/js/eval', {
    method: 'POST',
    headers: { 'content-type': 'text/plain; charset=utf-8' },
    body: code || '',
  });
  const text = await res.text();
  const j = safeJsonParse(text);
  return j || { ok: false, output: text, error: 'non-json response', timed_out: false };
}

async function jsEvalUi() {
  const code = ($('js_code') && $('js_code').value) || '';
  if ($('js_result')) $('js_result').textContent = 'running...';
  const j = await jsEvalRaw(code);
  const out = [];
  out.push(`ok=${j.ok} timed_out=${j.timed_out}`);
  if (j.error) out.push(`error:\n${j.error}`);
  if (j.output) out.push(`output:\n${j.output}`);
  if ($('js_result')) $('js_result').textContent = out.join('\n\n');
}

async function jsReset() {
  await fetch('/api/js/reset', { method: 'POST' });
}

function appendWsLine(line) {
  const box = $('ws_events');
  if (!box) return;
  const cur = box.textContent || '';
  const next = (cur.length > 12000) ? cur.slice(cur.length - 12000) : cur;
  box.textContent = (next ? (next + '\n') : '') + line;
  box.scrollTop = box.scrollHeight;
}

function connectWs() {
  const proto = (location.protocol === 'https:') ? 'wss' : 'ws';
  const url = `${proto}://${location.host}/ws`;
  let ws = null;

  function start() {
    try { ws = new WebSocket(url); } catch (e) { setWsPill(false, 'failed'); return; }
    ws.onopen = () => setWsPill(true, 'connected');
    ws.onclose = () => { setWsPill(false, 'disconnected'); setTimeout(start, 750); };
    ws.onerror = () => setWsPill(false, 'error');
    ws.onmessage = (ev) => {
      const s = String(ev.data || '');
      const j = safeJsonParse(s);
      appendWsLine(j ? JSON.stringify(j) : s);
    };
  }

  start();
}

async function gpioRefresh() {
  const res = await fetch('/api/gpio/status', { cache: 'no-store' });
  const j = await res.json().catch(() => null);
  if (!j || !j.ok) return;
  if ($('gpio_g3_num')) $('gpio_g3_num').textContent = String(j.G3.gpio);
  if ($('gpio_g4_num')) $('gpio_g4_num').textContent = String(j.G4.gpio);
  if ($('gpio_g3_level')) $('gpio_g3_level').textContent = j.G3.level ? 'HIGH' : 'LOW';
  if ($('gpio_g4_level')) $('gpio_g4_level').textContent = j.G4.level ? 'HIGH' : 'LOW';
}

async function gpioToggle(label) {
  await jsEvalRaw(`gpio.toggle(${JSON.stringify(label)});`);
  setTimeout(() => { gpioRefresh().catch(() => {}); }, 50);
}

async function gpioPulse(label, ms) {
  const m = clamp(Number(ms), 1, 5000);
  const code =
    `gpio.write(${JSON.stringify(label)}, 1);\n` +
    `setTimeout(function(){ gpio.write(${JSON.stringify(label)}, 0); }, ${m});\n`;
  await jsEvalRaw(code);
  setTimeout(() => { gpioRefresh().catch(() => {}); }, m + 25);
}

const PRESET_KEY = 'mled_presets_v1';
function loadPresets() {
  const s = localStorage.getItem(PRESET_KEY);
  const j = safeJsonParse(s || '');
  state.presets = Array.isArray(j) ? j : [];
}
function savePresets() {
  localStorage.setItem(PRESET_KEY, JSON.stringify(state.presets));
}

function statusToApply(s) {
  if (!s || !s.pattern) return null;
  const p = s.pattern;
  const body = {
    type: p.type,
    brightness_pct: p.global_brightness_pct,
    frame_ms: s.frame_ms,
  };
  if (p.type && p[p.type]) body[p.type] = p[p.type];
  return body;
}

function renderPresets() {
  const host = $('preset_list');
  if (!host) return;
  host.innerHTML = '';
  if (!state.presets.length) {
    host.innerHTML = '<div class="muted">No presets saved yet (stored in browser localStorage).</div>';
    return;
  }
  state.presets.forEach((p, idx) => {
    const row = document.createElement('div');
    row.className = 'row';
    row.style.justifyContent = 'space-between';

    const left = document.createElement('div');
    left.textContent = p.name || `preset-${idx + 1}`;
    left.className = 'mono';

    const right = document.createElement('div');
    right.className = 'row';

    const load = document.createElement('button');
    load.textContent = 'Load';
    load.className = 'primary';
    load.addEventListener('click', () => { postApply(p.apply).catch(() => {}); });

    const dup = document.createElement('button');
    dup.textContent = 'Duplicate';
    dup.addEventListener('click', () => {
      state.presets.splice(idx + 1, 0, { name: `${p.name || 'preset'} copy`, apply: p.apply });
      savePresets();
      renderPresets();
    });

    const del = document.createElement('button');
    del.textContent = 'Delete';
    del.addEventListener('click', () => {
      state.presets.splice(idx, 1);
      savePresets();
      renderPresets();
    });

    right.appendChild(load);
    right.appendChild(dup);
    right.appendChild(del);
    row.appendChild(left);
    row.appendChild(right);
    host.appendChild(row);
  });
}

function bindPresets() {
  if ($('preset_save')) $('preset_save').addEventListener('click', () => {
    const name = String(($('preset_name') && $('preset_name').value) || '').trim() || `preset-${state.presets.length + 1}`;
    const apply = statusToApply(state.lastStatus);
    if (!apply) return;
    state.presets.push({ name, apply });
    savePresets();
    renderPresets();
  });
  if ($('preset_export')) $('preset_export').addEventListener('click', () => {
    if ($('preset_blob')) $('preset_blob').value = JSON.stringify(state.presets, null, 2);
  });
  if ($('preset_import')) $('preset_import').addEventListener('click', () => {
    const txt = String(($('preset_blob') && $('preset_blob').value) || '');
    const j = safeJsonParse(txt);
    if (!Array.isArray(j)) return;
    state.presets = j;
    savePresets();
    renderPresets();
  });
}

async function sysRefreshJsMem() {
  const res = await fetch('/api/js/mem', { cache: 'no-store' });
  const text = await res.text();
  if ($('sys_jsmem')) $('sys_jsmem').textContent = text;
}

function bindJsLab() {
  if ($('js_run')) $('js_run').addEventListener('click', () => jsEvalUi().catch(() => {}));
  if ($('js_clear')) $('js_clear').addEventListener('click', () => { if ($('js_result')) $('js_result').textContent = ''; });
  if ($('ws_clear')) $('ws_clear').addEventListener('click', () => { if ($('ws_events')) $('ws_events').textContent = ''; });
  if ($('js_reset')) $('js_reset').addEventListener('click', () => jsReset().catch(() => {}));
  if ($('js_run_reset')) $('js_run_reset').addEventListener('click', async () => {
    if ($('js_result')) $('js_result').textContent = 'resetting...';
    await jsReset().catch(() => {});
    await jsEvalUi().catch(() => {});
  });
}

function installJsExamples() {
  const examples = [
    {
      name: 'Blink G3 (250ms)',
      code:
`// Blink G3 (250ms)\n` +
`var h = null;\n` +
`h = every(250, function(){ gpio.toggle('G3'); });\n` +
`emit('hint', { stop: 'cancel(h) or reset VM' });\n`,
    },
    {
      name: 'Cycle patterns',
      code:
`// Cycle patterns\n` +
`var i = 0;\n` +
`var pats = ['rainbow','chase','breathing','sparkle'];\n` +
`every(2500, function(){\n` +
`  sim.setPattern(pats[i % pats.length]);\n` +
`  i++;\n` +
`});\n`,
    },
    {
      name: 'Random sparkle colors',
      code:
`// Random sparkle colors\n` +
`sim.setPattern('sparkle');\n` +
`every(1200, function(){\n` +
`  var c = '#' + (Math.random()*0xFFFFFF|0).toString(16).padStart(6,'0');\n` +
`  sim.setSparkle({ speed: 8, density: 55, fade: 160, mode: 1, color: c, bg: '#000010' });\n` +
`  emit('sparkle', { color: c });\n` +
`});\n`,
    },
  ];

  const host = $('js_examples');
  if (!host) return;
  host.innerHTML = '';
  examples.forEach((ex) => {
    const b = document.createElement('button');
    b.className = 'ex';
    b.textContent = ex.name;
    b.addEventListener('click', () => {
      if ($('js_code')) $('js_code').value = ex.code;
    });
    host.appendChild(b);
  });

  const demo =
`// 0066 JS Lab demo (ES5)\n` +
`emit('log', { msg: 'hello from JS', ts: Date.now() });\n` +
`sim.setBrightness(25);\n` +
`sim.setPattern('rainbow');\n`;
  if ($('js_code') && !$('js_code').value) $('js_code').value = demo;
}

function bindGpio() {
  if ($('gpio_refresh')) $('gpio_refresh').addEventListener('click', () => gpioRefresh().catch(() => {}));
  if ($('gpio_g3_toggle')) $('gpio_g3_toggle').addEventListener('click', () => gpioToggle('G3').catch(() => {}));
  if ($('gpio_g4_toggle')) $('gpio_g4_toggle').addEventListener('click', () => gpioToggle('G4').catch(() => {}));
  if ($('gpio_g3_p100')) $('gpio_g3_p100').addEventListener('click', () => gpioPulse('G3', 100).catch(() => {}));
  if ($('gpio_g3_p500')) $('gpio_g3_p500').addEventListener('click', () => gpioPulse('G3', 500).catch(() => {}));
  if ($('gpio_g4_p100')) $('gpio_g4_p100').addEventListener('click', () => gpioPulse('G4', 100).catch(() => {}));
  if ($('gpio_g4_p500')) $('gpio_g4_p500').addEventListener('click', () => gpioPulse('G4', 500).catch(() => {}));
}

function bindSystem() {
  if ($('sys_jsmem_refresh')) $('sys_jsmem_refresh').addEventListener('click', () => sysRefreshJsMem().catch(() => {}));
}

async function pollOnce() {
  state.lastPollAt = Date.now();
  await fetchStatus().catch(() => {});
  await fetchFrame().catch(() => {});
  await gpioRefresh().catch(() => {});
}

function restartPoll() {
  if (state.pollTimer) clearInterval(state.pollTimer);
  state.pollTimer = setInterval(() => { pollOnce().catch(() => {}); }, state.pollMs);
}

function main() {
  setHost();
  state.pollMs = clamp(getNumber('poll_ms', 500), 100, 10000);
  setPollLabel();
  bindTabs();
  bindGlobalControls();
  bindJsLab();
  bindGpio();
  bindPresets();
  bindSystem();
  connectWs();
  loadPresets();
  renderPresets();
  installJsExamples();

  const hash = String(location.hash || '').replace(/^#/, '');
  showScreen(hash || 'dashboard');

  pollOnce().catch(() => {});
  restartPoll();
}

window.addEventListener('load', main);
