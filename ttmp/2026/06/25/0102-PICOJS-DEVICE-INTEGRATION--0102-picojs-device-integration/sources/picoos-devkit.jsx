import React, { useRef, useEffect, useState, useCallback } from "react";

/* ============================================================================
   picoOS devkit — an IDE + live emulator for prototyping the TUI DSL.
   Left: editor / presets.  Right: a simulated 40x30 PicoCalc LCD that runs
   the DSL for real (screen buffer + panels + widgets + reactivity + input).
   ============================================================================ */

const COLS = 40;
const ROWS = 30;

const PALETTE = {
  bg:   "#06100a",
  fg:   "#7df2a6",
  dim:  "#37714c",
  white:"#daffe9",
  cyan: "#62e7f0",
  green:"#7cf08a",
  amber:"#f4c95d",
  red:  "#f47a6b",
  blue: "#83b2f6",
  mag:  "#e79cf2",
};
const color = (c) => (c && PALETTE[c]) || c || PALETTE.fg;

const FRAMES = {
  rounded: "╭╮╰╯─│",
  single:  "┌┐└┘─│",
  double:  "╔╗╚╝═║",
  bold:    "┏┓┗┛━┃",
};

/* ----------------------------------------------------------------- screen */
function makeScreen(cols, rows) {
  const blank = () =>
    Array.from({ length: rows }, () =>
      Array.from({ length: cols }, () => ({ ch: " ", fg: "fg", bold: false, dim: false }))
    );
  let cells = blank();
  const inB = (x, y) => x >= 0 && y >= 0 && x < cols && y < cols && y < rows;
  const S = {
    cols, rows,
    get cells() { return cells; },
    clear() { cells = blank(); },
    set(x, y, ch, st = {}) {
      x = Math.round(x); y = Math.round(y);
      if (x < 0 || y < 0 || x >= cols || y >= rows) return;
      cells[y][x] = { ch: ch || " ", fg: st.fg || "fg", bold: !!st.bold, dim: !!st.dim };
    },
    text(x, y, str, st) {
      str = String(str ?? "");
      for (let i = 0; i < str.length; i++) S.set(x + i, y, str[i], st);
    },
    hline(x, y, w, ch, st) { for (let i = 0; i < w; i++) S.set(x + i, y, ch, st); },
    vline(x, y, h, ch, st) { for (let i = 0; i < h; i++) S.set(x, y + i, ch, st); },
    box(x, y, w, h, frame = "single", st) {
      const f = FRAMES[frame] || FRAMES.single;
      const [tl, tr, bl, br, hz, vt] = f.split("");
      S.set(x, y, tl, st); S.set(x + w - 1, y, tr, st);
      S.set(x, y + h - 1, bl, st); S.set(x + w - 1, y + h - 1, br, st);
      S.hline(x + 1, y, w - 2, hz, st); S.hline(x + 1, y + h - 1, w - 2, hz, st);
      S.vline(x, y + 1, h - 2, vt, st); S.vline(x + w - 1, y + 1, h - 2, vt, st);
    },
  };
  return S;
}

/* --------------------------------------------------------------- utilities */
const resolve = (v, ...a) => (typeof v === "function" ? v(...a) : v);
const padNum = (n, len, fill = "0") => String(n).padStart(len, fill);
const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
const fmtTime = (sec) => {
  sec = Math.max(0, Math.floor(sec));
  const m = Math.floor(sec / 60), s = sec % 60;
  return m + ":" + padNum(s, 2);
};
function resolveX(x, w, cw) {
  if (x === "right") return cw - w;
  if (x === "center") return Math.floor((cw - w) / 2);
  return x || 0;
}

/* ============================================================ DSL runtime */
function createRuntime() {
  let app = null;

  /* ---- mock device data; evolves over time so widgets feel alive ---- */
  const hist = { cpu: [], mem: [], tmp: [], load: [] };
  const pushHist = (k, v, max = 64) => {
    hist[k].push(v); if (hist[k].length > max) hist[k].shift();
  };
  const procs = [
    { pid: 1, name: "kernel", cpu: 2, mem: 18 },
    { pid: 7, name: "ui", cpu: 11, mem: 42 },
    { pid: 12, name: "music", cpu: 48, mem: 31 },
    { pid: 19, name: "netd", cpu: 4, mem: 9 },
    { pid: 23, name: "shell", cpu: 1, mem: 6 },
  ];

  // snake engine
  const SNK = { w: 19, h: 9, body: [], dir: { x: 1, y: 0 }, food: { x: 12, y: 3 }, dead: false, ate: false };
  const resetSnake = () => {
    SNK.body = [{ x: 4, y: 3 }, { x: 5, y: 3 }, { x: 6, y: 3 }];
    SNK.dir = { x: 1, y: 0 }; SNK.dead = false; SNK.ate = false; SNK.food = { x: 12, y: 3 };
  };
  resetSnake();
  const placeFood = () => {
    let p; do { p = { x: (Math.random() * SNK.w) | 0, y: (Math.random() * SNK.h) | 0 }; }
    while (SNK.body.some((s) => s.x === p.x && s.y === p.y));
    SNK.food = p;
  };

  // music engine
  const TRACKS = [
    { title: "Cosmic Drift", artist: "Nebula Theory", album: "Parallax", length: 252, liked: true },
    { title: "Solder Joints", artist: "Flux Core", album: "Reflow", length: 198, liked: false },
    { title: "Baud Rate", artist: "115200", album: "Serial", length: 221, liked: true },
  ];
  const player = { idx: 0, pos: 107, playing: true, vol: 70, shuffle: false };

  // chat
  const room = {
    online: 4,
    messages: [
      { user: "ada", text: "got it booting over serial", mine: false, t: -90 },
      { user: "ada", text: "115200 baud, clean", mine: false, t: -84 },
      { user: "you", text: "flashed yours yet?", mine: true, t: -60 },
      { user: "lin", text: "pushing the TUI lib now", mine: false, t: -20 },
      { user: "lin", text: "check the gauges 😄", mine: false, t: -8 },
    ],
  };
  const userColors = { ada: "cyan", lin: "amber", you: "green", flux: "mag" };

  // tiny filesystem
  const FS = {
    "/home/user": [
      { name: "projects/", dir: true, size: "" },
      { name: "music/", dir: true, size: "" },
      { name: "readme.md", dir: false, size: "1k", body: "# picoOS\na tiny text OS for the picocalc." },
      { name: "notes.txt", dir: false, size: "4k", body: "shopping\n- oat milk\n- coffee" },
      { name: "todo.md", dir: false, size: "812", body: "- ship the DSL\n- sleep" },
      { name: ".config", dir: false, size: "64", body: "theme=amber" },
    ],
  };
  let cwd = "/home/user";
  let selectedFile = FS[cwd][2];

  const cfg = { bright: 80, theme: "amber", font: "6x8", haptics: true, echo: false, sleep: "2 min" };

  let toast = "";

  let acc = 0;
  function evolve(dt) {
    acc += dt;
    if (acc >= 220) {
      acc = 0;
      const jit = (v, lo, hi, amt) => clamp(v + (Math.random() - 0.5) * amt, lo, hi);
      OS.metrics.cpu = Math.round(jit(OS.metrics.cpu, 8, 96, 26));
      OS.metrics.mem = Math.round(jit(OS.metrics.mem, 20, 88, 10));
      OS.metrics.tmp = Math.round(jit(OS.metrics.tmp, 38, 64, 6));
      OS.battery = clamp(OS.battery - 0.05, 0, 100);
      pushHist("cpu", OS.metrics.cpu); pushHist("mem", OS.metrics.mem);
      pushHist("tmp", OS.metrics.tmp); pushHist("load", OS.metrics.cpu);
      procs.forEach((p) => (p.cpu = clamp(Math.round(p.cpu + (Math.random() - 0.5) * 12), 0, 99)));
    }
    if (player.playing) {
      player.pos += dt / 1000;
      if (player.pos >= TRACKS[player.idx].length) { player.pos = 0; OS.next(); }
    }
  }
  for (let i = 0; i < 64; i++) { pushHist("cpu", 30 + ((Math.random() * 40) | 0)); pushHist("load", 30 + ((Math.random() * 40) | 0)); }

  const OS = {
    battery: 64,
    metrics: { cpu: 62, mem: 41, tmp: 48 },
    history: (k, n) => hist[k].slice(-n),
    processes: () => procs.slice(),
    clock(fmt = "HH:mm") {
      const d = new Date();
      const h = padNum(d.getHours(), 2), m = padNum(d.getMinutes(), 2), s = padNum(d.getSeconds(), 2);
      return fmt.replace("HH", h).replace("mm", m).replace("ss", s);
    },
    launch(name) { toast = "launch → " + name; },
    get toast() { return toast; },

    // music
    get library() { return { current: TRACKS[player.idx] }; },
    get position() { return player.pos; },
    get volume() { return player.vol; },
    set volume(v) { player.vol = clamp(v, 0, 100); },
    get mode() { return { get shuffle() { return player.shuffle; }, set shuffle(v) { player.shuffle = v; } }; },
    next() { player.idx = (player.idx + 1) % TRACKS.length; player.pos = 0; },
    prev() { player.idx = (player.idx - 1 + TRACKS.length) % TRACKS.length; player.pos = 0; },
    set playing(v) { player.playing = v; },
    get playing() { return player.playing; },
    fft(n) { return Array.from({ length: n }, () => (player.playing ? Math.random() : 0.05)); },

    // snake
    get snake() { return { cells: SNK.body, head: SNK.body[SNK.body.length - 1] }; },
    get food() { return SNK.food; },
    get ate() { return SNK.ate; },
    get dead() { return SNK.dead; },
    turn(tok) {
      const map = { "↑": { x: 0, y: -1 }, "↓": { x: 0, y: 1 }, "←": { x: -1, y: 0 }, "→": { x: 1, y: 0 } };
      const d = map[tok]; if (!d) return;
      if (d.x === -SNK.dir.x && d.y === -SNK.dir.y) return;
      SNK.dir = d;
    },
    step() {
      SNK.ate = false;
      if (SNK.dead) return;
      const head = SNK.body[SNK.body.length - 1];
      const nx = head.x + SNK.dir.x, ny = head.y + SNK.dir.y;
      if (nx < 0 || ny < 0 || nx >= SNK.w || ny >= SNK.h || SNK.body.some((s) => s.x === nx && s.y === ny)) {
        SNK.dead = true; return;
      }
      SNK.body.push({ x: nx, y: ny });
      if (nx === SNK.food.x && ny === SNK.food.y) { SNK.ate = true; placeFood(); }
      else SNK.body.shift();
    },
    reset() { resetSnake(); },

    // chat
    get room() { return room; },
    send(text) { if (text && text.trim()) room.messages.push({ user: "you", text: text.trim(), mine: true, t: 0 }); },
    colorOf: (u) => userColors[u] || "white",

    // files
    get cwd() { return cwd; },
    ls: (p) => FS[p] || [],
    cd(f) { if (FS[cwd + "/" + f.name.replace("/", "")]) cwd = cwd + "/" + f.name.replace("/", ""); },
    get selected() { return selectedFile; },
    select(f) { if (!f.dir) selectedFile = f; },

    // settings
    cfg,

    // calc — degrees-based trig, a few constants, parsed in a tiny scope
    eval(expr) {
      const e = String(expr)
        .replace(/×/g, "*").replace(/÷/g, "/").replace(/\^/g, "**")
        .replace(/√/g, "sqrt").replace(/π/g, "(pi)");
      const scope = {
        sin: (x) => Math.sin((x * Math.PI) / 180),
        cos: (x) => Math.cos((x * Math.PI) / 180),
        tan: (x) => Math.tan((x * Math.PI) / 180),
        sqrt: Math.sqrt, log: Math.log10, ln: Math.log,
        abs: Math.abs, round: Math.round, pi: Math.PI, e: Math.E,
      };
      const names = Object.keys(scope);
      const fn = new Function(...names, "return (" + e + ");");
      return fn(...names.map((k) => scope[k]));
    },

    _evolve: evolve,

    /* ---------------------------------------- the app/builder surface ---- */
    app(name) {
      app = new App(name);
      OS._app = app;
      return app;
    },
    _app: null,
  };

  /* ===================================================== App + Panel ===== */
  class App {
    constructor(name) {
      this.name = name;
      this._state = {};
      this._draws = [];      // {z, fn}
      this._keys = {};       // token -> fn
      this._timers = [];     // {ms, fn, acc}
      this._loops = [];      // {fps, fn, acc, paused}
      this._computes = [];
      this._focusables = [];
      this._hits = [];       // {x,y,w,h,onTap}
      this._statusbar = null;
      this._reserved = 1; // bottom row reserved for the status line
      this._mounted = false;
      this._panels = {};
      this._layout = null;
      // loop() is callable AND has .toggle()
      const self = this;
      const loopFn = (fps, fn) => { const L = { fps, fn, acc: 0, paused: false }; self._loops.push(L); return L; };
      loopFn.toggle = () => { const L = self._loops[self._loops.length - 1]; if (L) L.paused = !L.paused; };
      this.loop = loopFn;
    }
    state(o) { this._state = o; return o; }
    layout(fn) { const lb = new Layout(this); fn(lb); this._layout = lb; return this; }
    panel(id) {
      let rect = { x: 0, y: 0, w: COLS, h: ROWS - this._reserved };
      if (this._layout && this._layout.regions[id]) rect = this._layout.regions[id];
      const p = new Panel(this, rect);
      this._panels[id] = p;
      return p;
    }
    key(spec, fn) {
      String(spec).split("").forEach((c) => { if ("↑↓←→".includes(c)) this._keys[c] = fn; });
      if (!/[↑↓←→]/.test(spec)) this._keys[spec] = fn;
      return this;
    }
    on(evt, ms, fn) { if (evt === "tick") this._timers.push({ ms, fn, acc: 0 }); return this; }
    compute(fn) { this._computes.push(fn); return this; }
    statusbar(v) {
      this._statusbar = v;
      return this;
    }
    dispatch() { return this; }
    refresh() { return this; }
    exit() { this._exited = true; return this; }
    mount() { this._mounted = true; return this; }

    _fireKey(tok) {
      if (this._keys[tok]) { this._keys[tok](this, tok); return; }
      const f = this._focus();
      if (!f) return;
      if ("↑↓←→".includes(tok) && f.move) f.move(tok);
      else if (tok === "⏎" && f.activate) f.activate();
      else if (f.type) f.type(tok);
    }
    _focus() { return this._focusInput || this._focusables[0] || null; }
    _tap(cx, cy) {
      for (let i = this._hits.length - 1; i >= 0; i--) {
        const h = this._hits[i];
        if (cx >= h.x && cx < h.x + h.w && cy >= h.y && cy < h.y + h.h) { h.onTap(); return; }
      }
    }
    _frame(screen, dt) {
      this._timers.forEach((t) => { t.acc += dt; if (t.acc >= t.ms) { t.acc = 0; t.fn(this); } });
      this._loops.forEach((L) => { if (L.paused) return; L.acc += dt; const iv = 1000 / L.fps; while (L.acc >= iv) { L.acc -= iv; L.fn(this); } });
      this._computes.forEach((c) => { try { c(this); } catch (e) {} });
      this._hits = [];
      screen.clear();
      this._draws.slice().sort((a, b) => a.z - b.z).forEach((d) => d.fn(screen));
      if (this._statusbar) drawStatusbar(screen, this._statusbar);
    }
  }

  class Layout {
    constructor(app) { this.app = app; this.regions = {}; this._axis = null; this._segs = []; }
    _add(axis, size, id) {
      if (this._axis && this._axis !== axis) {/* single-axis only */}
      this._axis = axis; this._segs.push({ size, id }); this._recompute();
      return this;
    }
    row(size, id) { return this._add("rows", size, id); }
    col(size, id) { return this._add("cols", size, id); }
    cols(...pairs) { return this; }
    _recompute() {
      const total = this._axis === "cols" ? COLS : (ROWS - (this.app._reserved || 0));
      let fixed = 0, stars = 0;
      this._segs.forEach((s) => { if (s.size === "*") stars++; else fixed += parseInt(s.size, 10) || 0; });
      const starSize = stars ? Math.max(1, Math.floor((total - fixed) / stars)) : 0;
      let pos = 0;
      this._segs.forEach((s, i) => {
        let sz = s.size === "*" ? starSize : parseInt(s.size, 10) || 0;
        if (i === this._segs.length - 1) sz = total - pos; // last fills
        const rect = this._axis === "cols"
          ? { x: pos, y: 0, w: sz, h: ROWS - (this.app._reserved || 0) }
          : { x: 0, y: pos, w: COLS, h: sz };
        this.regions[s.id] = rect; pos += sz;
      });
    }
  }

  /* ======================================================== Panel ======== */
  class Panel {
    constructor(app, rect) {
      this.app = app; this.rect = rect;
      this._frame = null; this._fst = {}; this._title = null; this._titleR = null; this._footer = null;
      app._draws.push({ z: 0, fn: (s) => this._drawChrome(s) });
    }
    content() {
      const r = this.rect;
      return this._frame ? { x: r.x + 1, y: r.y + 1, w: r.w - 2, h: r.h - 2 } : { x: r.x, y: r.y, w: r.w, h: r.h };
    }
    _drawChrome(s) {
      const r = this.rect;
      if (this._frame) s.box(r.x, r.y, r.w, r.h, this._frame, { fg: this._fst.fg || "dim" });
      if (this._title != null) s.text(r.x + 2, r.y, " " + resolve(this._title).trim() + " ", { fg: "white", bold: true });
      if (this._titleR != null) { const t = " " + resolve(this._titleR).trim() + " "; s.text(r.x + r.w - 2 - t.length, r.y, t, { fg: "amber" }); }
      if (this._footer != null) { const t = " " + resolve(this._footer).trim() + " "; s.text(r.x + 2, r.y + r.h - 1, t, { fg: "dim" }); }
    }
    frame(f = "single", st) { this._frame = f; if (st) this._fst = st; return this; }
    title(t) { this._title = t; return this; }
    titleRight(t) { this._titleR = t; return this; }
    footer(t) { this._footer = t; return this; }
    _reg(z, fn) { this.app._draws.push({ z, fn }); }

    text(v) { return new Text(this, v); }
    gauge() { return new Gauge(this); }
    spark() { return new Spark(this); }
    table() { return new Table(this); }
    menu() { return new Menu(this, "grid"); }
    list() { return new Menu(this, "list"); }
    progress() { return new Progress(this); }
    row() { return new Row(this); }
    keypad() { return new Keypad(this); }
    pad() { return new Pad(this); }
    grid() { return new Grid(this); }
    form(fn) { const f = new Form(this); if (fn) fn(f); return f; }
    feed() { return new Feed(this); }
    input() { return new Input(this); }
    editor() { return new Editor(this); }
    viewer() { return new Viewer(this); }
  }

  /* ------- base for positioned widgets ------- */
  class W {
    constructor(panel, z = 1) {
      this.panel = panel; this._x = 0; this._y = 0; this._fg = null; this._bold = false; this._dim = false;
      panel._reg(z, (s) => this._draw(s));
    }
    at(x, y) { this._x = x; this._y = (y == null ? 0 : y); return this; }
    fg(c) { this._fg = c; return this; }
    bold() { this._bold = true; return this; }
    dim() { this._dim = true; return this; }
    abs(localX, w) {
      const c = this.panel.content();
      return { x: c.x + resolveX(localX, w, c.w), y: c.y + this._y, c };
    }
    _st(extra) { return { fg: this._fg || "fg", bold: this._bold, dim: this._dim, ...extra }; }
    _draw() {}
  }

  class Text extends W {
    constructor(p, v) { super(p); this._v = v; }
    _draw(s) {
      const str = String(resolve(this._v) ?? "");
      const { x, y } = this.abs(this._x, str.length);
      s.text(x, y, str, this._st());
    }
  }

  class Gauge extends W {
    constructor(p) { super(p); this._label = ""; this._val = 0; this._max = 100; this._w = 12; this._style = "bar"; this._pct = false; }
    label(l) { this._label = l; return this; }
    value(v) { this._val = v; return this; }
    max(m) { this._max = m; return this; }
    width(w) { this._w = w; return this; }
    style(s) { this._style = s; return this; }
    showPct() { this._pct = true; return this; }
    _draw(s) {
      const v = clamp(resolve(this._val), 0, resolve(this._max)), mx = resolve(this._max);
      const frac = mx ? v / mx : 0, fill = Math.round(frac * this._w);
      const ch = this._style === "blocks" ? ["▓", "░"] : ["█", "░"];
      const bar = ch[0].repeat(fill) + ch[1].repeat(this._w - fill);
      const lbl = this._label ? this._label.padEnd(4) + " " : "";
      const pct = this._pct || this._style ? " " + padNum(Math.round(frac * 100), 2, " ") + "%" : "";
      const str = lbl + bar + pct;
      const { x, y } = this.abs(this._x, str.length);
      s.text(x, y, lbl, this._st({ fg: this._fg || "dim" }));
      s.text(x + lbl.length, y, bar, this._st({ fg: this._fg || "green" }));
      s.text(x + lbl.length + bar.length, y, pct, this._st({ fg: "dim" }));
    }
  }

  class Spark extends W {
    constructor(p) { super(p); this._label = ""; this._data = []; this._min = 0; this._max = 100; this._glyphs = "▁▂▃▄▅▆▇█"; }
    label(l) { this._label = l; return this; }
    data(d) { this._data = d; return this; }
    range(a, b) { this._min = a; this._max = b; return this; }
    glyphs(g) { this._glyphs = g; return this; }
    width() { return this; }
    _draw(s) {
      const d = resolve(this._data) || [];
      const g = this._glyphs;
      const spark = d.map((v) => {
        const f = clamp((v - this._min) / (this._max - this._min || 1), 0, 1);
        return g[Math.min(g.length - 1, Math.round(f * (g.length - 1)))];
      }).join("");
      const lbl = this._label ? this._label.padEnd(5) + " " : "";
      const { x, y } = this.abs(this._x, lbl.length + spark.length);
      s.text(x, y, lbl, this._st({ fg: "dim" }));
      s.text(x + lbl.length, y, spark, this._st({ fg: this._fg || "cyan" }));
    }
  }

  class Table extends W {
    constructor(p) { super(p); this._cols = []; this._rows = []; this._sel = 0; this._marker = "›"; this._sortKey = null; this._sortDir = "desc"; this._accent = "cyan"; p.app._focusables.push(this); }
    columns(c) { this._cols = c; return this; }
    rows(r) { this._rows = r; return this; }
    select(i) { this._sel = i; return this; }
    marker(m) { this._marker = m; return this; }
    sortBy(k, d = "desc") { this._sortKey = k; this._sortDir = d; return this; }
    accent(a) { this._accent = a; return this; }
    _data() {
      let r = (resolve(this._rows) || []).slice();
      if (this._sortKey) r.sort((a, b) => (a[this._sortKey] - b[this._sortKey]) * (this._sortDir === "desc" ? -1 : 1));
      return r;
    }
    current() { return this._data()[this._sel]; }
    move(t) { const n = this._data().length; if (t === "↑") this._sel = (this._sel - 1 + n) % n; if (t === "↓") this._sel = (this._sel + 1) % n; }
    _draw(s) {
      const rows = this._data(), c = this.panel.content();
      const widths = this._cols.map((col) =>
        Math.max(col.length, ...rows.map((r) => String(r[col] ?? "").length)) + 1);
      const fmtRow = (vals) => vals.map((v, i) => String(v ?? "").padEnd(widths[i])).join("");
      let y = c.y + this._y;
      s.text(c.x + 2, y, fmtRow(this._cols), this._st({ fg: "dim" }));
      rows.forEach((r, i) => {
        const sel = i === this._sel;
        s.text(c.x, y + 1 + i, sel ? this._marker : " ", { fg: this._accent, bold: true });
        s.text(c.x + 2, y + 1 + i, fmtRow(this._cols.map((k) => r[k])), { fg: sel ? this._accent : "fg", bold: sel });
      });
    }
  }

  class Menu extends W {
    constructor(p, kind) { super(p); this._kind = kind; this._items = []; this._grid = 1; this._marker = "›"; this._accent = "cyan"; this._onPick = () => {}; this._render = null; this._footer = null; this._sel = 0; this._frame = null; this._title = null; p.app._focusables.push(this); }
    frame(f) { this._frame = f; return this; }
    title(t) { this._title = t; return this; }
    grid(n) { this._grid = n; return this; }
    items(v) { this._items = v; return this; }
    render(fn) { this._render = fn; return this; }
    marker(m) { this._marker = m; return this; }
    accent(a) { this._accent = a; return this; }
    onPick(fn) { this._onPick = fn; return this; }
    footer(f) { this._footer = f; return this; }
    filter() { return { prompt() {} }; }
    _list() { return resolve(this._items) || []; }
    move(t) { const n = this._list().length; const g = this._grid;
      if (t === "↑") this._sel = clamp(this._sel - g, 0, n - 1);
      if (t === "↓") this._sel = clamp(this._sel + g, 0, n - 1);
      if (t === "←") this._sel = clamp(this._sel - 1, 0, n - 1);
      if (t === "→") this._sel = clamp(this._sel + 1, 0, n - 1);
    }
    activate() { const it = this._list()[this._sel]; this._onPick(it); }
    _draw(s) {
      const items = this._list();
      let c = this.panel.content();
      let ox = c.x + this._x, oy = c.y + this._y, innerW = c.w - this._x;
      if (this._frame) {
        const h = this._kind === "grid" ? Math.ceil(items.length / this._grid) + 2 : items.length + 2;
        s.box(ox, oy, innerW, h, this._frame, { fg: "dim" });
        if (this._title) s.text(ox + 2, oy, " " + resolve(this._title).trim() + " ", { fg: "white" });
        ox += 1; oy += 1; innerW -= 2;
      }
      if (this._kind === "grid") {
        const colW = Math.floor(innerW / this._grid);
        items.forEach((it, i) => {
          const r = Math.floor(i / this._grid), cI = i % this._grid;
          const sel = i === this._sel;
          const label = (sel ? this._marker + " " : "  ") + String(it);
          const cx = ox + cI * colW, cy = oy + r;
          s.text(cx, cy, label, { fg: sel ? this._accent : "fg", bold: sel });
          this.panel.app._hits.push({ x: cx, y: cy, w: colW, h: 1, onTap: () => { this._sel = i; this.activate(); } });
        });
      } else {
        items.forEach((it, i) => {
          const sel = i === this._sel;
          const parts = this._render ? this._render(it) : [String(it)];
          const line = parts.length === 3
            ? parts[0] + " " + String(parts[1]).padEnd(innerW - 6) + String(parts[2]).padStart(4)
            : parts.join(" ");
          s.text(ox, oy + i, sel ? this._marker : " ", { fg: this._accent, bold: true });
          s.text(ox + 1, oy + i, " " + line, { fg: sel ? this._accent : "fg", bold: sel });
          this.panel.app._hits.push({ x: ox, y: oy + i, w: innerW, h: 1, onTap: () => { this._sel = i; this.activate(); } });
        });
      }
      if (this._footer) s.text(c.x, c.y + c.h - 1, resolve(this._footer), { fg: "dim" });
    }
  }

  class Progress extends W {
    constructor(p) { super(p); this._val = 0; this._max = 100; this._knob = "●"; this._track = "─"; this._fmt = null; this._labels = false; }
    value(v) { this._val = v; return this; }
    max(m) { this._max = m; return this; }
    knob(k) { this._knob = k; return this; }
    track(t) { this._track = t; return this; }
    fmt(f) { this._fmt = f; return this; }
    labels(b = true) { this._labels = b; return this; }
    _draw(s) {
      const c = this.panel.content();
      const v = resolve(this._val), mx = resolve(this._max) || 1;
      const left = this._labels ? fmtTime(v) + " " : "";
      const right = this._labels ? " " + fmtTime(mx) : "";
      const barW = c.w - this._x - left.length - right.length;
      const pos = clamp(Math.round((v / mx) * (barW - 1)), 0, barW - 1);
      let bar = "";
      for (let i = 0; i < barW; i++) bar += i === pos ? this._knob : this._track;
      const y = c.y + this._y;
      s.text(c.x + this._x, y, left, { fg: "dim" });
      s.text(c.x + this._x + left.length, y, bar, { fg: this._fg || "green" });
      s.text(c.x + this._x + left.length + barW, y, right, { fg: "dim" });
    }
  }

  class Row extends W {
    constructor(p) { super(p); this._segs = []; }
    button(label) { this._segs.push({ kind: "button", label, onTap: () => {} }); return this; }
    toggle(label) { this._segs.push({ kind: "toggle", label, bind: null }); return this; }
    spacer() { this._segs.push({ kind: "spacer" }); return this; }
    onTap(fn) { const seg = this._lastInteractive(); if (seg) seg.onTap = fn; return this; }
    bind(fn) { const seg = this._lastInteractive(); if (seg) seg.bind = fn; return this; }
    _lastInteractive() { for (let i = this._segs.length - 1; i >= 0; i--) if (this._segs[i].kind !== "spacer") return this._segs[i]; return null; }
    _draw(s) {
      const c = this.panel.content();
      let x = c.x + this._x, y = c.y + this._y;
      const right = [];
      this._segs.forEach((seg) => {
        if (seg.kind === "spacer") { x = c.x + c.w - this._estRight(); return; }
        const label = " " + resolve(seg.label) + " ";
        let on = false;
        if (seg.kind === "toggle" && seg.bind) on = !!resolve(seg.bind);
        s.text(x, y, label, { fg: on ? "amber" : (seg.kind === "button" ? "white" : "dim"), bold: seg.kind === "button" || on });
        this.panel.app._hits.push({ x, y, w: label.length, h: 1, onTap: () => { if (seg.kind === "button") seg.onTap(); else if (seg.bind && seg.bind._set) {} } });
        x += label.length + 1;
      });
    }
    _estRight() { let w = 0, seen = false; this._segs.forEach((s) => { if (s.kind === "spacer") seen = true; else if (seen) w += (" " + resolve(s.label) + " ").length + 1; }); return w; }
  }

  class Keypad extends W {
    constructor(p) { super(p); this._grid = []; this._frame = "single"; this._cw = 4; this._ch = 1; this._onKey = () => {}; }
    grid(g) { this._grid = g; return this; }
    frame(f) { this._frame = f; return this; }
    cell(w, h) { this._cw = w; this._ch = h; return this; }
    onKey(fn) { this._onKey = fn; return this; }
    _draw(s) {
      const c = this.panel.content();
      const ox = c.x + this._x, oy = c.y + this._y;
      const cw = this._cw + 1, chh = this._ch + 1;
      this._grid.forEach((rowArr, r) => rowArr.forEach((key, ci) => {
        const x = ox + ci * cw, y = oy + r * chh;
        s.box(x, y, cw + 1, chh + 1, this._frame, { fg: "dim" });
        const label = String(key);
        s.text(x + Math.floor((cw + 1 - label.length) / 2), y + 1, label, { fg: "white", bold: true });
        this.panel.app._hits.push({ x, y, w: cw + 1, h: chh + 1, onTap: () => this._onKey(key) });
      }));
    }
  }

  class Pad extends W {
    constructor(p) { super(p); this._chips = []; this._wrap = 3; this._onTap = () => {}; }
    chips(c) { this._chips = c; return this; }
    wrap(n) { this._wrap = n; return this; }
    onTap(fn) { this._onTap = fn; return this; }
    _draw(s) {
      const c = this.panel.content();
      const ox = c.x + this._x, oy = c.y + this._y;
      let col = 0, row = 0, x = ox;
      this._chips.forEach((chip) => {
        const label = "[" + chip + "]";
        if (col >= this._wrap) { col = 0; row++; x = ox; }
        s.text(x, oy + row, label, { fg: "cyan" });
        this.panel.app._hits.push({ x, y: oy + row, w: label.length, h: 1, onTap: () => this._onTap(chip) });
        x += label.length + 1; col++;
      });
    }
  }

  class Grid extends W {
    constructor(p) { super(p); this._w = 10; this._h = 10; this._cell = "· "; this._layers = []; }
    size(w, h) { this._w = w; this._h = h; return this; }
    cell(s) { this._cell = s; return this; }
    layer(name, fn, glyph) { this._layers.push({ name, fn, glyph }); return this; }
    render() { return this; }
    _draw(s) {
      const c = this.panel.content();
      const cw = this._cell.length;
      const ox = c.x + this._x, oy = c.y + this._y;
      for (let y = 0; y < this._h; y++)
        s.text(ox, oy + y, this._cell.repeat(this._w), { fg: "dim" });
      this._layers.forEach((L) => {
        const cells = resolve(L.fn) || [];
        cells.forEach((p) => { if (p) s.text(ox + p.x * cw, oy + p.y, L.glyph, { fg: L.name === "food" ? "amber" : L.name === "head" ? "white" : "green", bold: true }); });
      });
    }
  }

  class Form extends W {
    constructor(p) { super(p); this._fields = []; this._marker = "›"; this._accent = "amber"; this._sel = 0; p.app._focusables.push(this); }
    section(name) { this._fields.push({ kind: "section", name }); return this; }
    slider(name) { const f = { kind: "slider", name, obj: null, key: null, min: 0, max: 100, step: 5, suffix: "" }; this._fields.push(f); this._cur = f; return this; }
    select(name) { const f = { kind: "select", name, obj: null, key: null, options: [] }; this._fields.push(f); this._cur = f; return this; }
    toggle(name) { const f = { kind: "toggle", name, obj: null, key: null }; this._fields.push(f); this._cur = f; return this; }
    bind(obj, key) { if (this._cur) { this._cur.obj = obj; this._cur.key = key; } return this; }
    range(a, b) { if (this._cur) { this._cur.min = a; this._cur.max = b; } return this; }
    step(s) { if (this._cur) this._cur.step = s; return this; }
    fmt(s) { if (this._cur) this._cur.suffix = s === "%" ? "%" : s; return this; }
    options(o) { if (this._cur) this._cur.options = o; return this; }
    marker(m) { this._marker = m; return this; }
    accent(a) { this._accent = a; return this; }
    _selectable() { return this._fields.filter((f) => f.kind !== "section"); }
    move(t) {
      const sels = this._selectable(); const n = sels.length;
      if (t === "↑") this._sel = (this._sel - 1 + n) % n;
      if (t === "↓") this._sel = (this._sel + 1) % n;
      const f = sels[this._sel];
      if (!f || !f.obj) return;
      if (t === "←" || t === "→") {
        const dir = t === "→" ? 1 : -1;
        if (f.kind === "slider") f.obj[f.key] = clamp(f.obj[f.key] + dir * f.step, f.min, f.max);
        if (f.kind === "select") { const i = f.options.indexOf(f.obj[f.key]); f.obj[f.key] = f.options[(i + dir + f.options.length) % f.options.length]; }
      }
    }
    activate() { const f = this._selectable()[this._sel]; if (f && f.kind === "toggle" && f.obj) f.obj[f.key] = !f.obj[f.key]; }
    _draw(s) {
      const c = this.panel.content();
      let y = c.y + this._y, si = 0;
      this._fields.forEach((f) => {
        if (f.kind === "section") { s.text(c.x + 2, y, f.name, { fg: "white", bold: true }); y++; return; }
        const sel = si === this._sel;
        s.text(c.x + 2, y, sel ? this._marker : " ", { fg: this._accent, bold: true });
        s.text(c.x + 4, y, f.name.padEnd(13), { fg: sel ? this._accent : "fg" });
        const val = f.obj ? f.obj[f.key] : "";
        if (f.kind === "slider") {
          const w = 10, fill = Math.round(((val - f.min) / (f.max - f.min || 1)) * (w - 1));
          let bar = ""; for (let i = 0; i < w; i++) bar += i === fill ? "●" : "─";
          s.text(c.x + 17, y, bar, { fg: "green" });
          s.text(c.x + 28, y, val + (f.suffix || ""), { fg: "dim" });
        } else if (f.kind === "select") {
          s.text(c.x + 17, y, "‹ " + val + " ›", { fg: "amber" });
        } else if (f.kind === "toggle") {
          s.text(c.x + 17, y, val ? "[ on  ]" : "[ off ]", { fg: val ? "green" : "dim" });
        }
        si++; y++;
      });
    }
  }

  class Feed extends W {
    constructor(p) { super(p); this._src = []; this._bubble = () => "left"; this._name = (m) => m.user; this._accent = () => "white"; this._stick = "bottom"; }
    source(v) { this._src = v; return this; }
    bubble(fn) { this._bubble = fn; return this; }
    name(fn) { this._name = fn; return this; }
    accent(fn) { this._accent = fn; return this; }
    stick() { return this; }
    timeFormat() { return this; }
    push(m) { const arr = resolve(this._src); if (Array.isArray(arr)) arr.push(m); return this; }
    flash() { return this; }
    _draw(s) {
      const c = this.panel.content();
      const msgs = resolve(this._src) || [];
      const shown = msgs.slice(-Math.floor(c.h / 2));
      let y = c.y + Math.max(0, c.h - shown.length * 2);
      shown.forEach((m) => {
        const mine = this._bubble(m) === "right";
        const nm = this._name(m), txt = m.text;
        if (mine) {
          const line = txt + " ▕";
          s.text(c.x + c.w - line.length - nm.length - 1, y, line, { fg: "fg" });
          s.text(c.x + c.w - nm.length, y, nm, { fg: this._accent(m), bold: true });
        } else {
          s.text(c.x, y, nm.padEnd(5), { fg: this._accent(m), bold: true });
          s.text(c.x + 6, y, "▏ " + txt, { fg: "fg" });
        }
        y += 2;
      });
    }
  }

  class Input extends W {
    constructor(p) { super(p); this._ph = ""; this._marker = "›"; this._hint = ""; this._onSubmit = () => {}; this._buf = ""; p.app._focusInput = this; }
    placeholder(t) { this._ph = t; return this; }
    marker(m) { this._marker = m; return this; }
    submitHint(h) { this._hint = h; return this; }
    onSubmit(fn) { this._onSubmit = fn; return this; }
    type(tok) {
      if (tok === "⏎") { const r = this._onSubmit(this._buf); this._buf = typeof r === "string" ? r : ""; }
      else if (tok === "⌫") this._buf = this._buf.slice(0, -1);
      else if (tok.length === 1) this._buf += tok;
    }
    _draw(s) {
      const c = this.panel.content();
      const y = c.y + this._y;
      s.text(c.x, y, this._marker + " ", { fg: "green", bold: true });
      const show = this._buf || this._ph;
      s.text(c.x + 2, y, show + (this._buf ? "▌" : ""), { fg: this._buf ? "white" : "dim" });
      if (this._hint) s.text(c.x + c.w - this._hint.length, y, this._hint, { fg: "dim" });
    }
  }

  class Editor extends W {
    constructor(p) { super(p); this._lines = [""]; this._gutter = null; this._empty = "~"; this._cur = "block"; this._row = 0; this._col = 0; this._mode = "INSERT"; this._on = {}; p.app._focusInput = this; }
    open(name) { const f = (FS[cwd] || []).find((x) => x.name === name); this._lines = (f && f.body ? f.body : "").split("\n"); this._name = name; return this; }
    gutter(fn) { this._gutter = { numbers: true, w: 3, st: "dim" }; if (fn) try { fn({ numbers: () => this._gutter, style: () => this._gutter, width: (w) => { this._gutter.w = w; return this._gutter; } }); } catch (e) {} return this; }
    emptyMark(m) { this._empty = m; return this; }
    cursor(c) { this._cur = c; return this; }
    syntax() { return this; }
    on(evt, fn) { this._on[evt] = fn; return this; }
    mode(m) { this._mode = m === "normal" ? "NORMAL" : "INSERT"; return this; }
    save() { return { then: (cb) => { cb && cb(); return { catch() {} }; } }; }
    get file() { return this._name || "untitled"; }
    get dirty() { return this._dirty; }
    set dirty(v) { this._dirty = v; }
    get row() { return this._row + 1; }
    get col() { return this._col + 1; }
    type(tok) {
      if (this._mode === "NORMAL") { if (tok === "i") this._mode = "INSERT"; return; }
      const ln = this._lines;
      if (tok === "⏎") { const rest = ln[this._row].slice(this._col); ln[this._row] = ln[this._row].slice(0, this._col); ln.splice(this._row + 1, 0, rest); this._row++; this._col = 0; }
      else if (tok === "⌫") { if (this._col > 0) { ln[this._row] = ln[this._row].slice(0, this._col - 1) + ln[this._row].slice(this._col); this._col--; } else if (this._row > 0) { this._col = ln[this._row - 1].length; ln[this._row - 1] += ln[this._row]; ln.splice(this._row, 1); this._row--; } }
      else if (tok.length === 1) { ln[this._row] = ln[this._row].slice(0, this._col) + tok + ln[this._row].slice(this._col); this._col++; }
      if (this._on.change) this._on.change();
    }
    move(t) {
      if (t === "↑") this._row = clamp(this._row - 1, 0, this._lines.length - 1);
      if (t === "↓") this._row = clamp(this._row + 1, 0, this._lines.length - 1);
      if (t === "←") this._col = clamp(this._col - 1, 0, this._lines[this._row].length);
      if (t === "→") this._col = clamp(this._col + 1, 0, this._lines[this._row].length);
      this._col = clamp(this._col, 0, this._lines[this._row].length);
    }
    _draw(s) {
      const c = this.panel.content();
      const gw = this._gutter ? this._gutter.w + 1 : 0;
      for (let i = 0; i < c.h; i++) {
        const y = c.y + i;
        if (i < this._lines.length) {
          if (this._gutter) s.text(c.x, y, padNum(i + 1, this._gutter.w, " ") + " ", { fg: "dim" });
          s.text(c.x + gw, y, this._lines[i], { fg: "fg" });
          if (i === this._row) {
            s.set(c.x + gw + this._col, y, "▏", { fg: "green", bold: true });
          }
        } else {
          s.text(c.x, y, this._empty, { fg: "dim" });
        }
      }
    }
  }

  class Viewer extends W {
    constructor(p) { super(p); this._src = null; this._footer = null; }
    source(v) { this._src = v; return this; }
    syntax() { return this; }
    wrap() { return this; }
    footer(f) { this._footer = f; return this; }
    _draw(s) {
      const c = this.panel.content();
      const f = resolve(this._src);
      const body = (f && f.body) || "";
      s.text(c.x, c.y, (f && f.name) || "", { fg: "white", bold: true });
      s.text(c.x, c.y + 1, "─".repeat(Math.min(c.w, 10)), { fg: "dim" });
      body.split("\n").forEach((line, i) => s.text(c.x, c.y + 2 + i, line.slice(0, c.w), { fg: "fg" }));
      if (this._footer) s.text(c.x, c.y + c.h - 1, resolve(this._footer), { fg: "dim" });
    }
  }

  function drawStatusbar(s, v) {
    const y = ROWS - 1;
    s.hline(0, y, COLS, " ", { fg: "dim" });
    if (typeof v === "function") {
      const b = { _l: "", _c: "", _r: "", left(x) { this._l = resolve(x); return this; }, center(x) { this._c = resolve(x); return this; }, right(x) { this._r = resolve(x); return this; } };
      v(b);
      s.text(1, y, b._l, { fg: "amber" });
      s.text(Math.floor((COLS - String(b._c).length) / 2), y, b._c, { fg: "dim" });
      s.text(COLS - 1 - String(b._r).length, y, b._r, { fg: "dim" });
    } else {
      s.text(1, y, String(v), { fg: "dim" });
    }
  }

  return {
    OS,
    run(code) {
      const fn = new Function("OS", "pad", "clamp", code);
      fn(OS, padNum, clamp);
      return OS._app;
    },
    getApp() { return OS._app; },
    frame(screen, dt) { OS._evolve(dt); if (OS._app) OS._app._frame(screen, dt); },
  };
}

/* ============================================================== presets */
const PRESETS = {
  "home — dashboard": `// home dashboard: layout split, reactive clock, gauge + menu grid
const home = OS.app('home')

home.layout(l => l.row(1, 'bar').row('*', 'body'))

home.panel('bar').frame('rounded').title(' picoOS ')
  .titleRight(() => OS.clock('HH:mm'))

const body = home.panel('body').frame('rounded')
body.text('☀ 72°F').at(2, 1).fg('amber')
body.gauge().at(14, 1).label('batt').value(() => OS.battery)
    .width(8).style('blocks')

body.menu().frame('rounded').title(' apps ').at(2, 3)
  .grid(3)
  .items(['term', 'notes', 'files', 'music', 'sysmon', 'snake'])
  .marker('›').accent('cyan')
  .onPick(name => OS.launch(name))

home.statusbar('◂ ▸ select · ⏎ open · click items')
home.mount()`,

  "sysmon — gauges + table": `// system monitor: reusable gauges, live spark, sortable process table
const mon = OS.app('sysmon')
const ui  = mon.panel('main').frame('rounded').title(' sysmon ')

;['cpu','mem','tmp'].forEach((k, i) =>
  ui.gauge().at(2, 1 + i)
    .label(k).value(() => OS.metrics[k])
    .width(20).style('bar').showPct())

ui.spark().at(2, 5).label('load')
  .data(() => OS.history('load', 26))
  .range(0, 100).glyphs('▁▂▃▄▅▆▇█')

ui.table().at(2, 7)
  .columns(['pid','name','cpu','mem'])
  .rows(() => OS.processes())
  .sortBy('cpu','desc').select(0).marker('›')

mon.key('q', m => m.exit())
   .on('tick', 1000, m => m.refresh())

mon.statusbar('q quit · ↑↓ select · k kill')
mon.mount()`,

  "music — player": `// music player: state, button row, toggles, progress + live FFT
const p = OS.app('music')
const st = p.state({ get track() { return OS.library.current } })

const np = p.panel('np').frame('rounded').title(' now playing ')

np.text(() => st.track.title).at(3, 2).bold().fg('white')
np.text(() => st.track.artist + ' · ' + st.track.album).at(3, 3).fg('dim')

np.row().at(3, 5)
  .button('◂◂').onTap(() => OS.prev())
  .button(() => OS.playing ? '▮▮' : '▸').onTap(() => OS.playing = !OS.playing)
  .button('▸▸').onTap(() => OS.next())
  .spacer()
  .toggle('♥').bind(() => st.track.liked)
  .toggle('⤮ shuffle').bind(() => OS.mode.shuffle)

np.progress().at(3, 7)
  .value(() => OS.position).max(() => st.track.length)
  .knob('●').track('─').labels(true)

np.spark().at(3, 9).data(() => OS.fft(30)).range(0, 1).glyphs('▁▂▃▄▅▆▇█')
np.gauge().at(3, 11).label('vol').value(() => OS.volume).width(10).style('blocks')

p.statusbar('click transport · space = play/pause')
p.key(' ', () => OS.playing = !OS.playing)
p.mount()`,

  "snake — game loop": `// snake: layered grid renderer + fps game loop + arrow keys
const game = OS.app('snake')
const st = game.state({ score: 0, status: 'playing' })
OS.reset()

const board = game.panel('board').frame('rounded')
  .title(' snake ')
  .titleRight(() => 'score ' + pad(st.score, 3) + ' · ▸ ' + st.status)

const grid = board.grid().at(1, 1)
  .size(19, 9).cell('· ')
  .layer('body', () => OS.snake.cells, '█')
  .layer('head', () => [OS.snake.head], '○')
  .layer('food', () => [OS.food], '◆')

game.loop(8, () => {
  OS.step()
  if (OS.ate)  st.score += 10
  if (OS.dead) st.status = 'dead'
})

game.key('↑↓←→', (m, k) => OS.turn(k))
   .key('p', () => game.loop.toggle())
   .key('r', () => { OS.reset(); st.score = 0; st.status = 'playing' })

game.statusbar('↑↓←→ move · p pause · r reset')
game.mount()`,

  "calc — keypad + live eval": `// calculator: keypad grid, chip pad, reactive compute (degrees trig)
const calc = OS.app('calc')
const st = calc.state({ expr: 'sin(45) × 2', result: 1.41421356 })

const c = calc.panel('main').frame('rounded').title(' calc ')

c.text(() => st.expr).at('right', 2).fg('dim')
c.text(() => String(st.result)).at('right', 3).bold().fg('green')

c.keypad().at(2, 5)
  .grid([
    ['7','8','9','÷'],
    ['4','5','6','×'],
    ['1','2','3','-'],
    ['0','.','=','+'],
  ])
  .frame('single').cell(2, 1)
  .onKey(k => press(k))

c.pad().at(24, 6)
  .chips(['√','^','(',')','π','⌫','C'])
  .wrap(3).onTap(k => press(k))

function press(k) {
  if (k === 'C') { st.expr = ''; st.result = 0 }
  else if (k === '⌫') st.expr = st.expr.slice(0, -1)
  else if (k === '=') { /* live compute already shows it */ }
  else if (k === 'π') st.expr += 'π'
  else if (k === '√') st.expr += '√('
  else st.expr += k
}

// recompute every frame; keep last valid result on parse errors
calc.compute(() => { try { const r = OS.eval(st.expr); if (!isNaN(r)) st.result = Math.round(r * 1e8) / 1e8 } catch (e) {} })
calc.statusbar('tap keys · = shows result · C clears')
calc.mount()`,

  "settings — form": `// settings: form builder, sections, slider/select/toggle bound to OS.cfg
const cfg = OS.app('settings')

cfg.panel('main').frame('rounded').title(' settings ')
  .form(f => f
    .section('Display')
      .slider('brightness').bind(OS.cfg, 'bright').range(0, 100).step(5).fmt('%')
      .select('theme').bind(OS.cfg, 'theme').options(['amber','green','mono','night'])
      .select('font').bind(OS.cfg, 'font').options(['6x8','8x10','5x7'])
    .section('System')
      .toggle('haptics').bind(OS.cfg, 'haptics')
      .toggle('serial echo').bind(OS.cfg, 'echo')
      .select('auto-sleep').bind(OS.cfg, 'sleep').options(['off','1 min','2 min','5 min']))
  .marker('›').accent('amber')

cfg.statusbar('↑↓ move · ←→ change · ⏎ toggle')
cfg.mount()`,

  "chat — feed + input": `// chat: message feed with alignment + an input line (type + enter)
const chat = OS.app('chat')

chat.layout(l => l.row('*', 'log').row(1, 'input'))

const log = chat.panel('log').frame('rounded')
  .title(' #picocalc ')
  .titleRight(() => OS.room.online + ' ◉')

log.feed()
  .source(() => OS.room.messages)
  .bubble(m => m.mine ? 'right' : 'left')
  .name(m => m.user).accent(m => OS.colorOf(m.user))

chat.panel('input').input()
  .placeholder('type a message…')
  .marker('›').submitHint('⏎ ▸')
  .onSubmit(text => { OS.send(text); return '' })

chat.mount()`,

  "notes — editor": `// notes editor: editor widget with gutter + block cursor, 3-zone statusbar
const ed = OS.app('notes')

const e = ed.panel('edit').frame('rounded')
  .title(() => box.file + ' ')
  .titleRight(() => box.dirty ? '● edited' : '')

const box = e.editor()
  .open('notes.txt')
  .gutter(g => g.numbers())
  .emptyMark('~').cursor('block')
  .syntax('markdown')
  .on('change', () => box.dirty = true)

ed.statusbar(b => b
  .left(() => box.dirty ? 'INSERT *' : 'INSERT')
  .center(() => 'ln ' + box.row + ',' + box.col)
  .right('utf8 · type to edit'))

ed.key('esc', () => box.mode('normal'))
ed.mount()`,

  "hello — minimal": `// the smallest app: one panel, some text, a key, a tick.
const app = OS.app('hello')
const st = app.state({ n: 0 })

const p = app.panel('main').frame('rounded').title(' hello ')
p.text('picoOS DSL').at('center', 2).bold().fg('cyan')
p.text(() => 'ticks: ' + st.n).at('center', 4).fg('white')
p.text(() => OS.clock('HH:mm:ss')).at('center', 6).fg('amber')
p.text('press any letter →').at('center', 9).fg('dim')
p.text(() => st.last ? 'you pressed: ' + st.last : '').at('center', 10)

app.on('tick', 1000, () => st.n++)
app.key('a', () => st.last = 'a') // letters route here if bound
app.statusbar('a working starter — edit me on the left')
app.mount()`,
};

/* ================================================================ EMULATOR */
function escapeHtml(ch) {
  if (ch === "&") return "&amp;";
  if (ch === "<") return "&lt;";
  if (ch === ">") return "&gt;";
  return ch;
}
function renderHTML(cells) {
  let html = "";
  for (let r = 0; r < cells.length; r++) {
    const row = cells[r];
    let i = 0;
    while (i < row.length) {
      const c = row[i];
      let run = escapeHtml(c.ch);
      let j = i + 1;
      while (j < row.length && row[j].fg === c.fg && row[j].bold === c.bold && row[j].dim === c.dim) {
        run += escapeHtml(row[j].ch); j++;
      }
      const css = `color:${color(c.fg)};${c.bold ? "font-weight:700;" : ""}${c.dim ? "opacity:.55;" : ""}`;
      html += `<span style="${css}">${run}</span>`;
      i = j;
    }
    html += "\n";
  }
  return html;
}

function Emulator({ code, runToken, onError }) {
  const preRef = useRef(null);
  const wrapRef = useRef(null);
  const stateRef = useRef({});
  const [focused, setFocused] = useState(false);

  useEffect(() => {
    const screen = makeScreen(COLS, ROWS);
    let rt, app;
    try {
      rt = createRuntime();
      app = rt.run(code);
      onError(null);
    } catch (e) {
      onError(String(e.message || e));
      return;
    }
    stateRef.current = { rt, screen, app };

    let raf, last = performance.now(), paintAcc = 0, dead = false;
    const tick = (now) => {
      if (dead) return;
      const dt = Math.min(64, now - last); last = now;
      try {
        rt.frame(screen, dt);
        paintAcc += dt;
        if (paintAcc >= 33 && preRef.current) {
          paintAcc = 0;
          preRef.current.innerHTML = renderHTML(screen.cells);
        }
      } catch (e) {
        onError("runtime: " + String(e.message || e));
        dead = true; return;
      }
      raf = requestAnimationFrame(tick);
    };
    raf = requestAnimationFrame(tick);
    return () => { dead = true; cancelAnimationFrame(raf); };
  }, [code, runToken]);

  const keyMap = (e) => {
    if (e.key === "ArrowUp") return "↑";
    if (e.key === "ArrowDown") return "↓";
    if (e.key === "ArrowLeft") return "←";
    if (e.key === "ArrowRight") return "→";
    if (e.key === "Enter") return "⏎";
    if (e.key === "Escape") return "esc";
    if (e.key === "Backspace") return "⌫";
    if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "s") return "⌘s";
    if (e.key === " ") return " ";
    if (e.key.length === 1) return e.key;
    return null;
  };

  const onKeyDown = (e) => {
    const tok = keyMap(e);
    if (!tok) return;
    if (["↑", "↓", "←", "→", "⏎", " ", "⌫"].includes(tok)) e.preventDefault();
    const app = stateRef.current.app;
    if (app) { try { app._fireKey(tok); } catch (err) { onError("key: " + String(err.message || err)); } }
  };

  const onClick = (e) => {
    const pre = preRef.current; if (!pre) return;
    const rect = pre.getBoundingClientRect();
    const cw = rect.width / COLS, ch = rect.height / ROWS;
    const cx = Math.floor((e.clientX - rect.left) / cw);
    const cy = Math.floor((e.clientY - rect.top) / ch);
    const app = stateRef.current.app;
    if (app) { try { app._tap(cx, cy); } catch (err) { onError("tap: " + String(err.message || err)); } }
    wrapRef.current && wrapRef.current.focus();
  };

  return (
    <div style={S.deviceWrap}>
      <div style={S.bezel}>
        <div style={S.bezelTop}>
          <span style={S.brand}>PicoCalc</span>
          <span style={{ display: "flex", alignItems: "center", gap: 6 }}>
            <span style={{ ...S.led, background: focused ? PALETTE.green : "#1c2b22", boxShadow: focused ? `0 0 6px ${PALETTE.green}` : "none" }} />
            <span style={S.bezelLabel}>{focused ? "LIVE" : "click to focus"}</span>
          </span>
        </div>
        <div
          ref={wrapRef}
          tabIndex={0}
          onKeyDown={onKeyDown}
          onClick={onClick}
          onFocus={() => setFocused(true)}
          onBlur={() => setFocused(false)}
          style={{ ...S.lcd, outline: focused ? `1px solid ${PALETTE.dim}` : "1px solid transparent" }}
        >
          <pre ref={preRef} style={S.pre} />
          <div style={S.scan} />
        </div>
        <div style={S.bezelBottom}>
          <span style={S.dot} /><span style={S.dot} /><span style={S.dot} />
          <span style={{ marginLeft: "auto", letterSpacing: 1 }}>40 × 30 · 6×8</span>
        </div>
      </div>
    </div>
  );
}

/* ===================================================================== IDE */
export default function App() {
  const presetNames = Object.keys(PRESETS);
  const [preset, setPreset] = useState(presetNames[0]);
  const [code, setCode] = useState(PRESETS[presetNames[0]]);
  const [runToken, setRunToken] = useState(0);
  const [liveCode, setLiveCode] = useState(PRESETS[presetNames[0]]);
  const [auto, setAuto] = useState(true);
  const [err, setErr] = useState(null);
  const taRef = useRef(null);
  const gutRef = useRef(null);
  const debounce = useRef(null);

  const choosePreset = (name) => {
    setPreset(name); setCode(PRESETS[name]); setLiveCode(PRESETS[name]); setRunToken((t) => t + 1); setErr(null);
  };

  const run = useCallback(() => { setLiveCode(code); setRunToken((t) => t + 1); setErr(null); }, [code]);

  useEffect(() => {
    if (!auto) return;
    if (debounce.current) clearTimeout(debounce.current);
    debounce.current = setTimeout(() => { setLiveCode(code); setRunToken((t) => t + 1); }, 450);
    return () => clearTimeout(debounce.current);
  }, [code, auto]);

  const onTaKey = (e) => {
    if ((e.metaKey || e.ctrlKey) && e.key === "Enter") { e.preventDefault(); run(); return; }
    if (e.key === "Tab") {
      e.preventDefault();
      const t = e.target, s = t.selectionStart, en = t.selectionEnd;
      const nv = code.slice(0, s) + "  " + code.slice(en);
      setCode(nv);
      requestAnimationFrame(() => { t.selectionStart = t.selectionEnd = s + 2; });
    }
  };

  const lineCount = code.split("\n").length;

  return (
    <div style={S.app}>
      <style>{`
        *{box-sizing:border-box}
        textarea::selection{background:${PALETTE.dim}}
        .ta:focus{outline:none}
        select:focus,button:focus{outline:1px solid ${PALETTE.green}}
        ::-webkit-scrollbar{width:8px;height:8px}
        ::-webkit-scrollbar-thumb{background:#243; border-radius:4px}
      `}</style>

      <header style={S.header}>
        <div style={S.headerLeft}>
          <span style={S.logoMark}>▚</span>
          <span style={S.logoText}>picoOS</span>
          <span style={S.logoSub}>devkit</span>
        </div>
        <div style={S.headerRight}>
          <span style={S.eyebrow}>prototype the TUI DSL — edit JS, watch the LCD</span>
        </div>
      </header>

      <div style={S.body}>
        {/* -------- editor pane -------- */}
        <section style={S.editorPane}>
          <div style={S.paneBar}>
            <select value={preset} onChange={(e) => choosePreset(e.target.value)} style={S.select}>
              {presetNames.map((n) => <option key={n} value={n}>{n}</option>)}
            </select>
            <div style={{ flex: 1 }} />
            <label style={S.autoLabel}>
              <input type="checkbox" checked={auto} onChange={(e) => setAuto(e.target.checked)} style={{ accentColor: PALETTE.green }} />
              auto-run
            </label>
            <button onClick={run} style={S.runBtn}>▸ Run</button>
          </div>

          <div style={S.editorScroll}>
            <div ref={gutRef} style={S.gutter}>
              {Array.from({ length: lineCount }, (_, i) => <div key={i}>{i + 1}</div>)}
            </div>
            <textarea
              ref={taRef}
              className="ta"
              value={code}
              spellCheck={false}
              onChange={(e) => setCode(e.target.value)}
              onKeyDown={onTaKey}
              onScroll={(e) => { if (gutRef.current) gutRef.current.scrollTop = e.target.scrollTop; }}
              style={S.textarea}
            />
          </div>

          <div style={S.console}>
            {err
              ? <span style={{ color: PALETTE.red }}>✕ {err}</span>
              : <span style={{ color: PALETTE.dim }}>● running · Ctrl/⌘+Enter to run · Tab indents</span>}
          </div>
        </section>

        {/* -------- emulator pane -------- */}
        <section style={S.emuPane}>
          <Emulator code={liveCode} runToken={runToken} onError={setErr} />
          <p style={S.emuHint}>
            Click the screen to give it focus, then use arrows / Enter / letters.
            Buttons, menu items and keypads are clickable too.
          </p>
        </section>
      </div>
    </div>
  );
}

/* ===================================================================== css */
const mono = "ui-monospace, 'SF Mono', 'JetBrains Mono', 'Cascadia Code', Menlo, monospace";
const S = {
  app: { display: "flex", flexDirection: "column", height: "100vh", minHeight: 560, background: "#0a0d0b", color: "#cdd6cf", fontFamily: mono, fontSize: 13 },
  header: { display: "flex", alignItems: "center", justifyContent: "space-between", padding: "10px 16px", borderBottom: "1px solid #1b231d", background: "linear-gradient(180deg,#0e1310,#0a0d0b)" },
  headerLeft: { display: "flex", alignItems: "baseline", gap: 8 },
  logoMark: { color: PALETTE.green, fontSize: 16, transform: "translateY(1px)" },
  logoText: { color: "#e7f3ea", fontWeight: 700, letterSpacing: 0.5, fontSize: 15 },
  logoSub: { color: PALETTE.dim, fontSize: 11, letterSpacing: 2, textTransform: "uppercase" },
  headerRight: {},
  eyebrow: { color: "#5d7a66", fontSize: 11, letterSpacing: 1 },

  body: { flex: 1, display: "flex", minHeight: 0 },

  editorPane: { flex: "1 1 50%", display: "flex", flexDirection: "column", borderRight: "1px solid #1b231d", minWidth: 0 },
  paneBar: { display: "flex", alignItems: "center", gap: 10, padding: "8px 12px", borderBottom: "1px solid #161d18", background: "#0c110e" },
  select: { background: "#10160f", color: "#d6e6d9", border: "1px solid #243027", borderRadius: 6, padding: "4px 8px", fontFamily: mono, fontSize: 12 },
  autoLabel: { display: "flex", alignItems: "center", gap: 5, color: "#7c9183", fontSize: 12 },
  runBtn: { background: PALETTE.green, color: "#06140b", border: "none", borderRadius: 6, padding: "5px 14px", fontWeight: 700, fontFamily: mono, cursor: "pointer", fontSize: 12 },

  editorScroll: { flex: 1, display: "flex", overflow: "hidden", background: "#0a0f0c" },
  gutter: { padding: "12px 8px 12px 12px", textAlign: "right", color: "#33402f", userSelect: "none", overflow: "hidden", lineHeight: "20px", fontSize: 12.5, minWidth: 42 },
  textarea: { flex: 1, resize: "none", border: "none", background: "transparent", color: "#d3e6d6", fontFamily: mono, fontSize: 12.5, lineHeight: "20px", padding: "12px 14px 12px 6px", caretColor: PALETTE.green, whiteSpace: "pre", overflowWrap: "normal", overflowX: "auto" },

  console: { padding: "7px 14px", borderTop: "1px solid #161d18", background: "#0c110e", fontSize: 11.5, minHeight: 30, whiteSpace: "pre-wrap" },

  emuPane: { flex: "1 1 50%", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 12, padding: 20, background: "radial-gradient(120% 120% at 50% 0%, #0f1611 0%, #080b09 70%)", minWidth: 0 },
  emuHint: { color: "#4f6657", fontSize: 11.5, textAlign: "center", maxWidth: 360, lineHeight: 1.5, margin: 0 },

  deviceWrap: { display: "flex", justifyContent: "center" },
  bezel: { background: "linear-gradient(160deg,#20271f,#11160f)", border: "1px solid #2c3a2c", borderRadius: 18, padding: 14, boxShadow: "0 18px 50px rgba(0,0,0,.55), inset 0 1px 0 rgba(255,255,255,.04)" },
  bezelTop: { display: "flex", alignItems: "center", justifyContent: "space-between", padding: "0 4px 10px" },
  brand: { color: "#7d9081", fontSize: 12, letterSpacing: 3, fontWeight: 700, textTransform: "uppercase" },
  bezelLabel: { color: "#5d7466", fontSize: 10, letterSpacing: 1.5, textTransform: "uppercase" },
  led: { width: 8, height: 8, borderRadius: "50%", transition: "all .2s" },
  lcd: { position: "relative", background: PALETTE.bg, borderRadius: 8, padding: "10px 12px", border: "1px solid #16241a", boxShadow: "inset 0 0 36px rgba(0,0,0,.7), inset 0 0 4px rgba(125,242,166,.08)", cursor: "default" },
  pre: { margin: 0, fontFamily: mono, fontSize: 16, lineHeight: "16px", letterSpacing: 0, color: PALETTE.fg, textShadow: "0 0 4px rgba(125,242,166,.25)", whiteSpace: "pre", userSelect: "none" },
  scan: { pointerEvents: "none", position: "absolute", inset: 10, borderRadius: 4, background: "repeating-linear-gradient(0deg, rgba(0,0,0,0) 0px, rgba(0,0,0,0) 2px, rgba(0,0,0,.10) 3px)", mixBlendMode: "multiply" },
  bezelBottom: { display: "flex", alignItems: "center", gap: 6, padding: "10px 4px 0", color: "#4d6153", fontSize: 10 },
  dot: { width: 5, height: 5, borderRadius: "50%", background: "#2a382b" },
};
