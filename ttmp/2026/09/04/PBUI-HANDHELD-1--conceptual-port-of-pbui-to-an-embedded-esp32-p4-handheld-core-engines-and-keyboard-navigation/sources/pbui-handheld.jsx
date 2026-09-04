import React, { useState, useEffect, useRef, useMemo } from "react";

/* ============================================================
   PBUI / HANDHELD — prototype
   A presentation-based UI on a simulated 320×320, 53×32-char
   LCD, driven entirely by the keyboard. No wheel: ↑/↓ move the
   presentation caret, ←/→ flip cards in the deck.

   Pointing engines:  caret (↑↓) · hints (f) · type-cycle (;x)
                      · label search (/) · accept mode (typed
                      hints + completion + recency default)
   Organization:      workspaces ▸ decks of full-screen cards ▸
                      drill-in stack (⏎ pushes, ⌫ pops) ·
                      iconic overview (o)
   Everything else:   minibuffer (:) · verb menu (m) · verb keys
                      (R/P/E) · tray (y,t) · peek (hold i) ·
                      transport (space , . < >) · repeat (r)
   ============================================================ */

/* ---------------- palette — the original PBUI paper scheme ---------------- */
const LCD = {
  bg: "#ffffff", ink: "#23262b", dim: "#7b8087", faint: "#b0b0a8",
  line: "#d9d9d4", sel: "#fdeec6",
  add: "#2e7d51", del: "#b8452c", addBg: "#e7f4ec", delBg: "#fbe9e4",
  sage: "#7cae9b", blue: "#7aa6c9", rose: "#d59a86", mustard: "#e0b95c",
  lavender: "#a99fc9", mint: "#8fc7b0", red: "#c2503a", green: "#3f9d6b",
  bezel: "#26282c", bezel2: "#33363c", desk: "#141519", silk: "#9aa1ab",
};
/* every presentation type wears its desktop tone (PTONE) as a left bar */
const TONE = {
  file: LCD.mint, hunk: LCD.rose, task: LCD.mustard, mem: LCD.lavender,
  ctxseg: LCD.mustard, step: LCD.lavender, toolcall: LCD.blue,
  card: LCD.ink, app: LCD.sage,
};
/* semantic change vocabulary keeps its desktop tones too */
const SEMTONE = {
  "extract-function": LCD.blue, "signature-change": LCD.mustard,
  "add-guard": LCD.sage, config: LCD.dim, scaffold: LCD.rose,
};
const COLS = 53, CONTENT_ROWS = 28; // rows 2..29
const GLYPH = { file: "ƒ", hunk: "◇", task: "☐", mem: "μ", ctxseg: "⌸", step: "§", toolcall: "⚙", ws: "⌂", card: "▣", app: "▦" };
const tokensOf = (s) => Math.max(8, Math.round(String(s || "").length / 3.6));
const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
const kfmt = (n) => (n >= 1000 ? (n / 1000).toFixed(1) + "k" : String(Math.round(n)));
/* Simulated repo files contain module declarations. Assemble them at
   runtime so the host bundler never mistakes a fake module inside the
   scenario for a real dependency. */
const _KW = "im" + "port";
const IMP = (what, mod) => _KW + " " + what + " from " + JSON.stringify(mod) + ";";

/* ============================================================
   THE RUN — a small hardcoded IR: files, tasks, memory, steps
   ============================================================ */
const FILES0 = {
  "src/auth/token.ts": [
    IMP("{ decode, sign }", "./jwt"), "",
    "export function verifyToken(tok) {",
    "  const claims = decode(tok);",
    "  if (!claims) return null;",
    "  if (claims.exp < Date.now() / 1000) return null;",
    "  return claims;",
    "}", "",
    "export function issueToken(user) {",
    "  return sign({ sub: user.id, exp: now() + 900 });",
    "}",
  ],
  "src/auth/session.ts": [
    IMP("{ verifyToken }", "./token"), "",
    "export function loadSession(req) {",
    "  const tok = req.cookies.session;",
    "  const claims = verifyToken(tok);",
    "  if (!claims) return null;",
    "  if (claims.exp - now() < 60) refresh(claims);",
    "  return { user: claims.sub, exp: claims.exp };",
    "}",
  ],
  "src/db/users.sql": [
    "CREATE TABLE users (",
    "  id TEXT PRIMARY KEY,",
    "  email TEXT NOT NULL",
    ");",
  ],
};
const TASKS0 = [
  { id: "t1", title: "add refresh grace period", status: "todo" },
  { id: "t2", title: "rotate signing keys", status: "todo" },
];
const MEM0 = [{ id: "m0", text: "tokens are 15-min TTL", pinned: true, src: "seed" }];

/* steps: each event becomes one tick of the transport */
const STEPS = [
  { id: "s1", title: "read the auth module", events: [
    { t: "read", path: "src/auth/token.ts" },
    { t: "read", path: "src/auth/session.ts" },
    { t: "think", text: "TTL check duplicated inline; no grace path" },
  ]},
  { id: "s2", title: "plan grace period", events: [
    { t: "think", text: "plan: extract validateSession, widen verifyToken, add guard" },
    { t: "task", op: "add", id: "t3", title: "extract validateSession" },
    { t: "task", op: "status", id: "t1", status: "doing" },
  ]},
  { id: "s3", title: "extract validateSession", events: [
    { t: "task", op: "status", id: "t3", status: "doing" },
    { t: "edit", id: "e1", path: "src/auth/session.ts", sym: "validateSession", sem: "extract-function",
      del: ["const claims = verifyToken(tok);", "if (!claims) return null;", "if (claims.exp - now() < 60) refresh(claims);"],
      add: ["const claims = validateSession(tok);", "if (!claims) return null;", "…", "function validateSession(tok) {", "  const c = verifyToken(tok);", "  if (!c) return null;", "  if (c.exp - now() < 60) refresh(c);", "  return c;", "}"] },
    { t: "mem", op: "add", id: "m1", text: "session.ts owns all TTL logic" },
    { t: "task", op: "status", id: "t3", status: "done" },
  ]},
  { id: "s4", title: "extend users schema", events: [
    { t: "edit", id: "e2", path: "src/db/users.sql", sym: "users", sem: "config",
      del: [], add: ["refresh_at INTEGER,", "grace_until INTEGER"] },
    { t: "tool", id: "tc1", name: "bash: sqlite3 migrate.sql", ok: true },
  ]},
  { id: "s5", title: "scaffold refresh endpoint", events: [
    { t: "create", id: "e3", path: "src/auth/refresh.ts", sym: "refresh", sem: "scaffold",
      del: [], add: [IMP("{ issueToken }", "./token"), "", "export function refresh(claims) {", "  return issueToken({ id: claims.sub });", "}"] },
  ]},
  { id: "s6", title: "widen verifyToken signature", events: [
    { t: "edit", id: "e4", path: "src/auth/token.ts", sym: "verifyToken", sem: "signature-change",
      del: ["export function verifyToken(tok) {"],
      add: ["export function verifyToken(tok, opts = {}) {", "  const grace = opts.grace ?? 0;"] },
    { t: "tool", id: "tc2", name: "bash: pnpm test", ok: false },
    { t: "think", text: "tests expect the old arity — session call site fixed next" },
  ]},
  { id: "s7", title: "add grace guard, fix tests", events: [
    { t: "edit", id: "e5", path: "src/auth/token.ts", sym: "verifyToken", sem: "add-guard",
      del: ["if (claims.exp < Date.now() / 1000) return null;"],
      add: ["const cutoff = Date.now() / 1000 - grace;", "if (claims.exp < cutoff) return null;"] },
    { t: "tool", id: "tc3", name: "bash: pnpm test", ok: true },
    { t: "task", op: "status", id: "t1", status: "done", note: "30s grace shipped" },
  ]},
  { id: "s8", title: "compact the window", events: [
    { t: "compact" },
    { t: "mem", op: "add", id: "m2", text: "grace period is 30s, make config-driven later", pinned: true },
  ]},
];

const SEM_BLURB = {
  "extract-function": "body lifted into a new named function",
  "signature-change": "parameters changed — callers affected",
  "add-guard": "precondition / early return added",
  config: "configuration / schema", scaffold: "new file skeleton",
};

/* flatten to a global event timeline */
const TL = (() => {
  const out = []; let gi = 0;
  STEPS.forEach((st, i) => {
    out.push({ gi: gi++, kind: "step", step: st, n: i + 1 });
    st.events.forEach((ev) => out.push({ gi: gi++, kind: "ev", step: st, n: i + 1, ev }));
  });
  return out;
})();
const MAXGI = TL.length - 1;
const stepEnd = {}; const stepStart = {};
TL.forEach((n) => { if (stepStart[n.step.id] === undefined) stepStart[n.step.id] = n.gi; stepEnd[n.step.id] = n.gi; });

/* ---- fold the timeline up to `cursor`, applying overrides ---- */
function fold(cursor, ov) {
  const skip = new Set(ov.skip), memForget = new Set(ov.memForget), memPin = new Set(ov.memPin), memUnpin = new Set(ov.memUnpin || []);
  const ctxEvict = new Set(ov.ctxEvict), ctxPin = new Set(ov.ctxPin);
  const edits = [], tools = [], thinks = [];
  const tasks = TASKS0.map((t) => ({ ...t }));
  const memory = MEM0.map((m) => ({ ...m }));
  let ctx = [
    { id: "c-sys", kind: "system", label: "system prompt + conventions", tok: 900, pinned: true },
    { id: "c-tls", kind: "system", label: "tool schemas (7 tools)", tok: 520, pinned: true },
  ];
  memory.forEach((m) => m.pinned && ctx.push({ id: "c-" + m.id, kind: "memory", label: "memory · " + m.text.slice(0, 30), tok: tokensOf(m.text) + 12, pinned: true, mem: m.id }));
  let executed = 0, curStep = null;
  for (const node of TL) {
    if (node.gi > cursor) break;
    if (node.kind === "step") { executed = node.n; curStep = node.step; continue; }
    const ev = node.ev, sid = node.step.id, seg = (kind, label, tok, extra) =>
      ctx.push({ id: "cs" + node.gi, kind, label, tok, step: sid, pinned: false, ...(extra || {}) });
    if (ev.t === "read") { seg("file", "read " + ev.path.split("/").pop(), tokensOf(FILES0[ev.path].join("\n"))); }
    else if (ev.t === "think") { thinks.push({ text: ev.text, step: sid }); seg("message", "think · " + ev.text.slice(0, 30), tokensOf(ev.text)); }
    else if (ev.t === "tool") { tools.push({ id: ev.id, name: ev.name, ok: ev.ok, step: sid, stepN: node.n }); seg("tool", ev.name + " → " + (ev.ok ? "ok" : "ERR"), 90); }
    else if (ev.t === "edit" || ev.t === "create") {
      const rev = skip.has(ev.id);
      edits.push({ id: ev.id, path: ev.path, sym: ev.sym, sem: ev.sem, del: ev.del, add: ev.add, step: sid, stepN: node.n, created: ev.t === "create", reverted: rev, a: rev ? 0 : ev.add.length, r: rev ? 0 : ev.del.length });
      seg("diff", (ev.t === "create" ? "new " : "diff ") + ev.path.split("/").pop(), 40 + ev.add.length * 14);
    }
    else if (ev.t === "task") {
      if (ev.op === "add") tasks.push({ id: ev.id, title: ev.title, status: "todo", born: sid });
      else { const tk = tasks.find((x) => x.id === ev.id); if (tk) { tk.status = ev.status; if (ev.note) tk.note = ev.note; } }
    }
    else if (ev.t === "mem") {
      const m = { id: ev.id, text: ev.text, pinned: !!ev.pinned, src: sid };
      memory.push(m);
      if (m.pinned) ctx.push({ id: "c-" + m.id, kind: "memory", label: "memory · " + m.text.slice(0, 30), tok: tokensOf(m.text) + 12, pinned: true, mem: m.id, step: sid });
    }
    else if (ev.t === "compact") {
      const drop = ctx.filter((s) => !s.pinned && s.kind !== "system" && !ctxPin.has(s.id));
      const freed = drop.reduce((a, b) => a + b.tok, 0);
      ctx = ctx.filter((s) => s.pinned || s.kind === "system" || ctxPin.has(s.id));
      ctx.push({ id: "cs" + node.gi, kind: "summary", label: "compacted " + drop.length + " segs (−" + kfmt(freed) + ")", tok: Math.max(220, freed * 0.14), step: sid });
    }
  }
  Object.entries(ov.taskSet).forEach(([id, status]) => { const tk = tasks.find((x) => x.id === id); if (tk) tk.status = status; });
  memory.forEach((m) => {
    if (memForget.has(m.id)) m.forgotten = true;
    m.pinned = (m.pinned || memPin.has(m.id)) && !memUnpin.has(m.id);
  });
  /* an override-pin materializes a window segment; an unpin (or forget)
     removes the memory's segment from the window */
  ctx = ctx.filter((s) => !(s.mem && (memUnpin.has(s.mem) || memForget.has(s.mem))));
  memory.filter((m) => m.pinned && !m.forgotten && !ctx.some((c) => c.mem === m.id))
    .forEach((m) => ctx.push({ id: "c-" + m.id, kind: "memory", label: "memory · " + m.text.slice(0, 30), tok: tokensOf(m.text) + 12, pinned: true, mem: m.id, step: m.src !== "seed" ? m.src : null }));
  ctx = ctx.filter((s) => !ctxEvict.has(s.id));
  ctx.forEach((s) => { if (ctxPin.has(s.id)) s.pinned = true; });
  const churn = {};
  edits.forEach((e) => { const c = churn[e.path] || (churn[e.path] = { a: 0, r: 0, sem: "", created: false }); c.a += e.a; c.r += e.r; c.sem = e.sem; c.created = c.created || e.created; });
  return { edits, tools, thinks, tasks, memory, ctx, churn, executed, curStep, ctxTok: ctx.reduce((a, b) => a + b.tok, 0), budget: 8000 };
}

/* ============================================================
   OBJECT LAYER — labels, catalogs, describe
   ============================================================ */
function labelFor(pt, v, S) {
  if (pt === "file") return String(v).split("/").pop();
  if (pt === "hunk") { const e = S.edits.find((x) => x.id === v); return e ? e.sym + " · " + e.sem : "(not yet)"; }
  if (pt === "task") { const t = S.tasks.find((x) => x.id === v); return t ? t.title : "(not yet)"; }
  if (pt === "mem") { const m = S.memory.find((x) => x.id === v); return m ? m.text.slice(0, 26) : "(not yet)"; }
  if (pt === "ctxseg") { const s = S.ctx.find((x) => x.id === v); return s ? s.label.slice(0, 26) : "(evicted)"; }
  if (pt === "step") { const i = STEPS.findIndex((s) => s.id === v); return "§" + (i + 1) + " " + STEPS[i].title; }
  if (pt === "toolcall") { const t = S.tools.find((x) => x.id === v); return t ? t.name : "(not yet)"; }
  if (pt === "card") { const f = findCard(SP, v); return f ? "[" + APP_TITLE[f.card.app] + "] · " + SP[f.wi].name : "(closed)"; }
  if (pt === "app") return APP_TITLE[v] ? APP_TITLE[v].toLowerCase() : String(v);
  return String(v);
}
function catalog(types, S) {
  const out = [];
  const put = (pt, v) => out.push({ pt, v, label: labelFor(pt, v, S) });
  const want = (t) => types.includes("any") || types.includes(t);
  if (want("hunk")) S.edits.forEach((e) => put("hunk", e.id));
  if (want("file")) Object.keys(FILES0).concat(S.edits.filter((e) => e.created).map((e) => e.path)).filter((p, i, a) => a.indexOf(p) === i).forEach((p) => put("file", p));
  if (want("task")) S.tasks.forEach((t) => put("task", t.id));
  if (want("mem")) S.memory.forEach((m) => put("mem", m.id));
  if (want("ctxseg")) S.ctx.forEach((s) => put("ctxseg", s.id));
  if (want("step")) STEPS.slice(0, Math.max(1, S.executed)).forEach((s) => put("step", s.id));
  if (want("toolcall")) S.tools.forEach((t) => put("toolcall", t.id));
  if (want("card")) SP.forEach((sp) => sp.cards.forEach((c) => put("card", c.id)));
  if (want("app")) APPS.forEach((a) => put("app", a));
  return out;
}

/* line helpers — a line may carry a bg ("add"/"del"/"sel") for diff rows;
   a seg style may be a named key, a raw hex color, or a style object */
const L = (segs, pres, bg) => ({ segs, pres, bg });
const T = (t, s) => ({ t, s });
const FILL = { fill: true };
const statusGlyph = (s) => (s === "done" ? "☑" : s === "doing" ? "◐" : s === "dropped" ? "⊘" : "☐");

function describe(pt, v, S, st) {
  const ls = [];
  const h = (t) => ls.push(L([T(t, "hdr")]));
  const d = (t) => ls.push(L([T(t, "dim")]));
  if (pt === "file") {
    const text = FILES0[v] || (S.edits.find((e) => e.path === v && e.created) || { add: [] }).add;
    const c = S.churn[v];
    h(GLYPH.file + " " + v);
    ls.push(L([T((text ? text.length : 0) + " lines · ", "dim"), T(c ? "+" + c.a : "", "add"), T(c ? " −" + c.r : "", "del"), T(c ? " this run" : "untouched", "dim"), T(c && c.created ? " · created by run" : "", "green")]));
    const hs = S.edits.filter((e) => e.path === v);
    if (hs.length) { ls.push(L([T(" ", 0)])); h("changes:"); hs.forEach((e) => ls.push(L([T("  " + GLYPH.hunk + " " + e.sym + " · ", e.reverted ? "faint" : 0), T(e.sem, e.reverted ? "faint" : { color: SEMTONE[e.sem] || LCD.dim }), T("  +" + e.a + " −" + e.r + (e.reverted ? "  reverted" : ""), e.reverted ? "faint" : "dim")], { pt: "hunk", v: e.id }))); }
    ls.push(L([T(" ", 0)])); h("head of file:");
    (text || []).slice(0, 9).forEach((t) => d("  " + t));
  } else if (pt === "hunk") {
    const e = S.edits.find((x) => x.id === v);
    if (!e) { d("not reached yet — scrub forward (. or >)"); return ls; }
    ls.push(L([T(GLYPH.hunk + " " + e.sym + " · ", "hdr"), T(e.sem, { color: SEMTONE[e.sem] || LCD.dim, fontWeight: 700 })]));
    d((SEM_BLURB[e.sem] || "") + (e.reverted ? "  ·  REVERTED" : ""));
    ls.push(L([T("in "), T(GLYPH.file + " " + e.path.split("/").pop(), { color: LCD.ink })], { pt: "file", v: e.path }));
    ls.push(L([T("at ")].concat([T(labelFor("step", e.step, S))]), { pt: "step", v: e.step }));
    ls.push(L([T(" ", 0)]));
    e.del.forEach((t) => ls.push(L([T("− " + t, "del")], null, "del")));
    e.add.forEach((t) => ls.push(L([T("+ " + t, "add")], null, "add")));
  } else if (pt === "task") {
    const t = S.tasks.find((x) => x.id === v);
    h(statusGlyph(t.status) + " " + t.title); d("status: " + t.status + (t.note ? " · " + t.note : ""));
    if (t.born) ls.push(L([T("added at ")].concat([T(labelFor("step", t.born, S))]), { pt: "step", v: t.born }));
  } else if (pt === "mem") {
    const m = S.memory.find((x) => x.id === v);
    h(GLYPH.mem + " " + m.text); d((m.pinned ? "pinned in window" : "not pinned") + (m.forgotten ? " · forgotten" : ""));
    if (m.src && m.src !== "seed") ls.push(L([T("learned at ")].concat([T(labelFor("step", m.src, S))]), { pt: "step", v: m.src }));
    else d("seeded before the run");
  } else if (pt === "ctxseg") {
    const s = S.ctx.find((x) => x.id === v);
    if (!s) { d("evicted / not present at this cursor"); return ls; }
    h(GLYPH.ctxseg + " " + s.label); d(s.kind + " · " + kfmt(s.tok) + " tok · " + ((s.tok / S.budget) * 100).toFixed(1) + "% of window" + (s.pinned ? " · pinned" : ""));
    if (s.step) ls.push(L([T("entered at ")].concat([T(labelFor("step", s.step, S))]), { pt: "step", v: s.step }));
  } else if (pt === "step") {
    const i = STEPS.findIndex((s) => s.id === v); const sp = STEPS[i];
    h("§" + (i + 1) + " " + sp.title); d(sp.events.length + " events" + (i + 1 > S.executed ? " · not reached at this cursor" : ""));
    S.edits.filter((e) => e.step === v).forEach((e) => ls.push(L([T("  "), T(GLYPH.hunk + " " + e.sym + " · "), T(e.sem, { color: SEMTONE[e.sem] || LCD.dim })], { pt: "hunk", v: e.id })));
    S.tools.filter((t) => t.step === v).forEach((t) => ls.push(L([T("  "), T(GLYPH.toolcall + " " + t.name + " → "), T(t.ok ? "ok" : "ERR", t.ok ? "green" : "red")], { pt: "toolcall", v: t.id })));
    S.thinks.filter((t) => t.step === v).forEach((t) => d("  ✎ " + t.text));
  } else if (pt === "toolcall") {
    const t = S.tools.find((x) => x.id === v);
    ls.push(L([T(GLYPH.toolcall + " " + t.name + " ", "hdr"), T(t.ok ? "· exit 0" : "· FAILED", t.ok ? "green" : "red")]));
    ls.push(L([T("at ")].concat([T(labelFor("step", t.step, S))]), { pt: "step", v: t.step }));
  } else if (pt === "card") {
    const f = findCard(SP, v);
    if (!f) { d("this tile has been closed"); return ls; }
    h(GLYPH.card + " [" + APP_TITLE[f.card.app] + "] tile");
    d("workspace " + SP[f.wi].name + " · position " + (f.ci + 1) + "/" + SP[f.wi].cards.length);
    d("vitals: " + vitals(f.card.app, S, st));
    ls.push(L([T(" ", 0)]));
    d("a tile is an object: switch · close · yank all");
    d("apply to it — the REPL always offers them");
  }
  return ls;
}

/* ============================================================
   COMMANDS — typed signatures drive completion/accept/defaults
   ============================================================ */
const ALL = ["hunk", "file", "task", "mem", "ctxseg", "step", "toolcall", "card"];
const CMDS = {
  open: { types: ALL, doc: "inspect an object (push a card)" },
  revert: { types: ["hunk"], doc: "undo a change" },
  restore: { types: ["hunk"], doc: "un-revert a change" },
  pin: { types: ["ctxseg", "mem"], doc: "pin into the window" },
  unpin: { types: ["ctxseg", "mem"], doc: "unpin" },
  evict: { types: ["ctxseg"], doc: "drop from the window" },
  forget: { types: ["mem"], doc: "forget a memory" },
  done: { types: ["task"], doc: "mark task done" },
  start: { types: ["task"], doc: "mark task doing" },
  goto: { types: ["step"], doc: "scrub the run to a step" },
  switch: { types: ["card"], doc: "jump to a tile" },
  close: { types: ["card"], doc: "close a tile (default: this one)" },
  newtile: { types: ["app"], doc: "open a new tile after this one" },
  yank: { types: ALL, doc: "put on the tray" },
  drop: { types: ALL, doc: "take an object off the tray", needsTray: true },
  clear: { types: [], doc: "empty the tray", needsTray: true },
  help: { types: [], doc: "show the key map" },
};
/* the REPL scans the screen: a command is offered when something visible
   can receive it. The current tile is itself an object and is always
   "visible", so tile verbs (and newtile's <app> slot) are always offered.
   Tray verbs appear only while the tray holds something. */
function availCmds(visibleTypes, trayLen) {
  const vis = new Set(visibleTypes);
  vis.add("card"); vis.add("app");
  return Object.keys(CMDS).filter((n) => {
    const c = CMDS[n];
    if (c.needsTray && !trayLen) return false;
    return !c.types.length || c.types.some((x) => vis.has(x));
  });
}
const arr = (a, x, on) => (on ? (a.includes(x) ? a : [...a, x]) : a.filter((y) => y !== x));

function runCmd(st, S, name, obj) {
  let s2 = { ...st, lastCmd: name };
  SP = s2.spaces || SP;
  const log = (text, pres) => { s2.log = [...s2.log, { kind: "cmd", text: name + (obj ? " " + labelFor(obj.pt, obj.v, S) : "") }, { kind: "out", text, pres }]; };
  const toast = (t) => { s2.toast = t; };
  if (obj) s2.hist = { ...s2.hist, [obj.pt]: obj.v };
  if (name === "help") { s2.mode = "help"; return s2; }
  if (name === "clear") { const n = s2.tray.length; s2.tray = []; s2.trayOpen = false; s2.toast = n ? "tray cleared (" + n + " dropped)" : "tray already empty"; return s2; }
  if (!obj) return s2;
  const ov = { ...s2.ov };
  if (name === "open") { s2.stack = [...s2.stack, obj]; toast("opened — ⌫ pops back"); }
  else if (name === "drop") {
    const had = s2.tray.some((x) => x.pt === obj.pt && x.v === obj.v);
    if (!had) toast("not on the tray — y puts it there");
    else {
      s2.tray = s2.tray.filter((x) => !(x.pt === obj.pt && x.v === obj.v));
      s2.trayOpen = true;
      toast("off the tray" + (s2.tray.length ? " — $n renumbered" : " — tray empty"));
    }
  }
  else if (name === "revert") { ov.skip = arr(ov.skip, obj.v, true); log("reverted — files recomputed", obj); toast("reverted"); }
  else if (name === "restore") { ov.skip = arr(ov.skip, obj.v, false); log("restored", obj); toast("restored"); }
  else if (name === "pin") {
    if (obj.pt === "mem") { ov.memPin = arr(ov.memPin, obj.v, true); ov.memForget = arr(ov.memForget, obj.v, false); ov.memUnpin = arr(ov.memUnpin || [], obj.v, false); }
    else ov.ctxPin = arr(ov.ctxPin, obj.v, true);
    log("pinned — survives compaction", obj); toast("pinned");
  }
  else if (name === "unpin") { if (obj.pt === "mem") { ov.memUnpin = arr(ov.memUnpin || [], obj.v, true); ov.memPin = arr(ov.memPin, obj.v, false); } else ov.ctxPin = arr(ov.ctxPin, obj.v, false); log("unpinned", obj); toast("unpinned"); }
  else if (name === "evict") { ov.ctxEvict = arr(ov.ctxEvict, obj.v, true); log("evicted from the window", obj); toast("evicted"); }
  else if (name === "forget") { ov.memForget = arr(ov.memForget, obj.v, true); log("forgotten", obj); toast("forgotten"); }
  else if (name === "done" || name === "start") { ov.taskSet = { ...ov.taskSet, [obj.v]: name === "done" ? "done" : "doing" }; log(name === "done" ? "marked done" : "started", obj); toast(name); }
  else if (name === "goto") { s2.cursor = stepEnd[obj.v]; s2.playing = false; log("rewound the run", obj); toast("cursor → " + labelFor("step", obj.v, S)); }
  else if (name === "yank") { if (!s2.tray.find((x) => x.pt === obj.pt && x.v === obj.v)) s2.tray = [...s2.tray, obj]; s2.trayOpen = true; toast("on the tray — $" + s2.tray.length); }
  else if (name === "switch") {
    const f = findCard(s2.spaces, obj.v);
    if (!f) toast("that tile is closed");
    else { s2.ws = f.wi; s2.card = f.ci; s2.stack = []; toast("→ " + labelFor("card", obj.v, S)); }
  }
  else if (name === "close") {
    const f = findCard(s2.spaces, obj.v);
    if (!f) toast("already closed");
    else if (s2.spaces[f.wi].cards.length <= 1) toast("won't close the last tile in a workspace");
    else {
      const label = labelFor("card", obj.v, S);
      s2.spaces = s2.spaces.map((sp, wi) => (wi === f.wi ? { ...sp, cards: sp.cards.filter((c) => c.id !== obj.v) } : sp));
      SP = s2.spaces;
      if (s2.ws === f.wi) {
        if (f.ci < s2.card) s2.card = s2.card - 1;
        else if (f.ci === s2.card) { s2.card = clamp(s2.card, 0, s2.spaces[f.wi].cards.length - 1); s2.stack = []; }
      }
      s2.log = [...s2.log, { kind: "cmd", text: "close " + label }, { kind: "out", text: "tile closed" }];
      toast("closed " + label);
    }
  }
  else if (name === "newtile") {
    const id = "k" + s2.nextId;
    s2.nextId = s2.nextId + 1;
    s2.spaces = s2.spaces.map((sp, wi) => (wi === s2.ws ? { ...sp, cards: [...sp.cards.slice(0, s2.card + 1), { id, app: obj.v }, ...sp.cards.slice(s2.card + 1)] } : sp));
    SP = s2.spaces;
    s2.card = s2.card + 1; s2.stack = [];
    s2.log = [...s2.log, { kind: "cmd", text: "newtile " + obj.v }, { kind: "out", text: "opened in " + s2.spaces[s2.ws].name, pres: { pt: "card", v: id } }];
    toast("new [" + APP_TITLE[obj.v] + "] tile");
  }
  s2.ov = ov;
  return s2;
}
const defaultVerb = (pt) => (pt === "task" ? "cycle" : pt === "step" ? "goto" : pt === "card" ? "switch" : "open");
function menuFor(pt, v, S, st) {
  const it = (k, label, run) => ({ k, label, run });
  const items = [it("i", "inspect", (s) => runCmd(s, S, "open", { pt, v })), it("y", "yank to tray", (s) => runCmd(s, S, "yank", { pt, v }))];
  if (pt === "hunk") {
    const e = S.edits.find((x) => x.id === v);
    items.push(it("r", e && e.reverted ? "restore this change" : "revert this change", (s) => runCmd(s, S, e && e.reverted ? "restore" : "revert", { pt, v })));
    if (e) items.push(it("s", "rewind to its step", (s) => runCmd(s, S, "goto", { pt: "step", v: e.step })));
  }
  if (pt === "mem") {
    const m = S.memory.find((x) => x.id === v);
    items.push(it("p", m && m.pinned ? "unpin" : "pin into the window", (s) => runCmd(s, S, m && m.pinned ? "unpin" : "pin", { pt, v })));
    items.push(it("x", "forget", (s) => runCmd(s, S, "forget", { pt, v })));
  }
  if (pt === "ctxseg") {
    const sg = S.ctx.find((x) => x.id === v);
    items.push(it("p", sg && sg.pinned ? "unpin" : "pin (survive compaction)", (s) => runCmd(s, S, sg && sg.pinned ? "unpin" : "pin", { pt, v })));
    items.push(it("e", "evict now", (s) => runCmd(s, S, "evict", { pt, v })));
    if (sg && sg.step) items.push(it("s", "rewind to where it entered", (s) => runCmd(s, S, "goto", { pt: "step", v: sg.step })));
  }
  if (pt === "task") { items.push(it("d", "mark done", (s) => runCmd(s, S, "done", { pt, v }))); items.push(it("g", "start", (s) => runCmd(s, S, "start", { pt, v }))); }
  if (pt === "step") items.push(it("g", "rewind the run here", (s) => runCmd(s, S, "goto", { pt, v })));
  if (pt === "card") {
    items.unshift(it("g", "switch to it", (s) => runCmd(s, S, "switch", { pt, v })));
    items.push(it("x", "close this tile", (s) => runCmd(s, S, "close", { pt, v })));
    items.push(it("n", "open a new tile… (accept an app)", (s) => ({ ...s, mode: "accept", accept: { cmd: "newtile", types: ["app"], buf: "", sel: 0, dflt: null, fromCmd: true } })));
  }
  if (pt === "toolcall") { const t = S.tools.find((x) => x.id === v); if (t) items.push(it("s", "rewind to its step", (s) => runCmd(s, S, "goto", { pt: "step", v: t.step }))); }
  return items;
}

/* ============================================================
   APPS — each builds lines for one card
   ============================================================ */
const APP_TITLE = { listener: "LISTENER", files: "FILES", edits: "EDITS", tasks: "TASKS", window: "WINDOW", memory: "MEMORY" };
const APPS = Object.keys(APP_TITLE);
/* tiles are objects: cards get stable ids so they can be created, closed,
   yanked, and named in the REPL like anything else */
const SPACES0 = [
  { name: "triage", cards: [{ id: "k1", app: "listener" }, { id: "k2", app: "files" }, { id: "k3", app: "edits" }, { id: "k4", app: "tasks" }] },
  { name: "context", cards: [{ id: "k5", app: "window" }, { id: "k6", app: "memory" }] },
];
/* labelFor/catalog/describe need the live layout; the component mirrors
   its state here each update (prototype-grade plumbing) */
let SP = SPACES0;
const findCard = (spaces, id) => {
  for (let wi = 0; wi < spaces.length; wi++) {
    const ci = spaces[wi].cards.findIndex((c) => c.id === id);
    if (ci >= 0) return { wi, ci, card: spaces[wi].cards[ci] };
  }
  return null;
};

function appLines(app, S, st) {
  const ls = [];
  const hdr = (t) => ls.push(L([T(t, "hdr")]));
  if (app === "listener") {
    ls.push(L([T("PBUI/HB 0.1 · presentation shell", "dim")]));
    ls.push(L([T("objects flow through this transcript & stay live", "dim")]));
    ls.push(L([T(" ")]));
    st.log.forEach((e) => {
      if (e.kind === "cmd") ls.push(L([T("» ", { color: LCD.blue, fontWeight: 700 }), T(e.text)]));
      else if (e.pres) {
        ls.push(L([T("  "), T(GLYPH[e.pres.pt] + " " + labelFor(e.pres.pt, e.pres.v, S))], e.pres));
        ls.push(L([T("    · " + e.text, "dim")]));
      }
      else ls.push(L([T("  · " + e.text, "dim")]));
    });
    ls.push(L([T("» ", { color: LCD.blue, fontWeight: 700 }), T("(: opens the command line)", "faint")]));
  }
  if (app === "files") {
    hdr("FILES · the repo as this run left it");
    const paths = Object.keys(FILES0).concat(S.edits.filter((e) => e.created).map((e) => e.path)).filter((p, i, a) => a.indexOf(p) === i);
    paths.forEach((p) => {
      const c = S.churn[p];
      ls.push(L([T(GLYPH.file + " " + p.padEnd(24).slice(0, 24)),
        T(c ? ("+" + c.a).padEnd(4) : "—".padEnd(4), "add"), T(c ? ("−" + c.r).padEnd(5) : "".padEnd(5), "del"),
        T(c ? c.sem : "", SEMTONE[c && c.sem] || "dim"), FILL, T(c && c.created ? "new" : "", "green")], { pt: "file", v: p }));
      S.edits.filter((e) => e.path === p).forEach((e) =>
        ls.push(L([T("   " + GLYPH.hunk + " " + e.sym.padEnd(16).slice(0, 16) + " ", e.reverted ? "faint" : 0),
          T(e.sem.padEnd(17).slice(0, 17), e.reverted ? "faint" : SEMTONE[e.sem] || "dim"),
          T(e.reverted ? "reverted" : "+" + e.a + " −" + e.r, e.reverted ? "faint" : "dim"), FILL, T("§" + e.stepN, { color: LCD.lavender })], { pt: "hunk", v: e.id })));
    });
    ls.push(L([T(" ")]));
    ls.push(L([T("⏎ inspect · R revert/restore · m verbs · f hints", "faint")]));
  }
  if (app === "edits") {
    hdr("EDITS · every change, list above / diff below");
    if (!S.edits.length) ls.push(L([T("no changes at this cursor — scrub forward (.)", "dim")]));
    S.edits.forEach((e) => ls.push(L([T(GLYPH.hunk + " " + e.sym.padEnd(16).slice(0, 16) + " ", e.reverted ? "faint" : 0),
      T(e.sem.padEnd(17).slice(0, 17), e.reverted ? "faint" : SEMTONE[e.sem] || "dim"),
      T(e.reverted ? "reverted " : ("+" + e.a).padEnd(4), e.reverted ? "faint" : "add"), T(e.reverted ? "" : ("−" + e.r).padEnd(4), "del"),
      T(e.path.split("/").pop(), { color: LCD.mint }), FILL, T("§" + e.stepN, { color: LCD.lavender })], { pt: "hunk", v: e.id })));
    ls.push(L([T("─".repeat(COLS), "faint")]));
    ls.__detail = true; // detail of caret hunk appended by caller
  }
  if (app === "tasks") {
    hdr("TASKS");
    S.tasks.forEach((t) => ls.push(L([
      T(statusGlyph(t.status) + " ", t.status === "done" ? "green" : t.status === "doing" ? { color: LCD.mustard } : 0),
      T(t.title.padEnd(32).slice(0, 32), t.status === "done" ? "dim" : 0),
      T(t.status, t.status === "doing" ? { color: LCD.mustard } : "dim"), FILL, T(t.note ? "· " + t.note : "", "faint")], { pt: "task", v: t.id })));
    ls.push(L([T(" ")]));
    ls.push(L([T("⏎ cycles todo → doing → done", "faint")]));
  }
  if (app === "window") {
    const pct = S.ctxTok / S.budget, bar = Math.round(pct * 36);
    const gc = pct > 0.9 ? LCD.rose : pct > 0.7 ? LCD.mustard : LCD.mint;
    hdr("CONTEXT WINDOW · " + kfmt(S.ctxTok) + " / " + kfmt(S.budget) + " tok");
    ls.push(L([T("▮".repeat(clamp(bar, 0, 36)), { color: gc }), T("▯".repeat(clamp(36 - bar, 0, 36)), "faint"), T(" " + Math.round(pct * 100) + "%", "dim")]));
    ls.push(L([T(" ")]));
    S.ctx.forEach((s) => ls.push(L([T(GLYPH.ctxseg + " " + s.label.padEnd(32).slice(0, 32)), T(kfmt(s.tok).padStart(5), "dim"), T(s.pinned ? "  •pin" : "", { color: LCD.mustard, fontWeight: 700 }), FILL, T(s.kind, "faint")], { pt: "ctxseg", v: s.id })));
    ls.push(L([T(" ")]));
    ls.push(L([T("P pin · E evict · pins survive §8's compaction", "faint")]));
  }
  if (app === "memory") {
    hdr("MEMORY");
    S.memory.forEach((m) => ls.push(L([T(GLYPH.mem + " " + m.text.padEnd(38).slice(0, 38), m.forgotten ? "faint" : 0), T(m.forgotten ? "forgotten" : m.pinned ? "•pinned" : "", m.forgotten ? "faint" : { color: LCD.mustard, fontWeight: 700 }), FILL, T(m.src === "seed" ? "seed" : "§" + (STEPS.findIndex((s) => s.id === m.src) + 1), { color: LCD.lavender })], { pt: "mem", v: m.id })));
    ls.push(L([T(" ")]));
    ls.push(L([T("P pin puts it into the window as a segment", "faint")]));
  }
  return ls;
}
function vitals(app, S, st) {
  if (app === "listener") return st.log.filter((e) => e.kind === "cmd").length + " cmds";
  if (app === "files") { const a = S.edits.reduce((x, e) => x + e.a, 0), r = S.edits.reduce((x, e) => x + e.r, 0); return "+" + a + " −" + r; }
  if (app === "edits") return S.edits.length + " chg";
  if (app === "tasks") return S.tasks.filter((t) => t.status === "doing").length + " doing";
  if (app === "window") return Math.round((S.ctxTok / S.budget) * 100) + "%";
  if (app === "memory") return "μ" + S.memory.filter((m) => !m.forgotten).length + "·" + S.memory.filter((m) => m.pinned && !m.forgotten).length + "pin";
  return "";
}

/* ============================================================
   HELP
   ============================================================ */
const HELP = [
  ["MOVE", ""],
  ["↑ ↓ (j k)", "presentation caret"],
  ["← → ([ ])", "prev / next card in the deck"],
  ["1..9 · ⇥", "jump to card · next workspace"],
  ["f", "hint labels — type one to jump"],
  ["; then f/h/t/m/c/s", "next file/hunk/task/mem/seg/step"],
  ["/", "search labels · ⏎ jumps"],
  ["ACT", ""],
  ["⏎", "default verb (open / cycle / goto)"],
  ["m", "verb menu for the caret"],
  ["R · P · E", "revert · pin · evict (typed verbs)"],
  ["hold i", "peek (inspector overlay, no nav)"],
  ["TRAY", ""],
  ["y · x", "yank the caret on · drop it off"],
  ["t", "show / hide the strip ($n renumber on drop)"],
  ["drop ␣ · clear", "drop lights only tray members · empty it"],
  ["$1 $2 …", "tray entries as command args, from any card"],
  ["r", "repeat last command on the caret"],
  ["REPL  (:)", ""],
  ["⇥", "completion scans the screen: only verbs"],
  ["", "something visible can receive are offered"],
  ["␣", "next argument — verb completes, every"],
  ["", "matching object lights up (digit picks)"],
  ["⏎ on empty arg", "takes the default: caret, or this tile"],
  ["$n · it", "pronouns in the arg slot: pin ␣ $1 ⏎ works"],
  ["TILES ARE OBJECTS", ""],
  ["newtile <app>", "open a tile after this one"],
  ["close <card>", "close a tile — close ␣⏎ = this one"],
  ["switch <card>", "jump anywhere (⏎ on a ▣ chip too)"],
  ["SHELL", ""],
  ["o", "overview (workspaces × cards)"],
  ["⌫ · esc", "pop drill-in · up one level"],
  ["space · , . · < >", "play · scrub event · scrub step"],
  ["? ", "this card"],
];

/* ============================================================
   MAIN COMPONENT
   ============================================================ */
export default function PBUIHandheld() {
  const [st, setSt] = useState(() => ({
    ws: 0, card: 1, spaces: SPACES0, nextId: 7, stack: [], carets: {}, scrolls: {},
    mode: "nav", hintBuf: "", hints: [], scopeWait: false,
    cmdBuf: "", cmdSel: 0, accept: null, menu: null, searchBuf: "",
    ovSel: 0, peek: false, tray: [], trayOpen: false,
    cursor: MAXGI, playing: false,
    ov: { skip: [], taskSet: {}, memForget: [], memPin: [], memUnpin: [], ctxEvict: [], ctxPin: [] },
    hist: {}, lastCmd: null, toast: null,
    log: [],
  }));
  SP = st.spaces;
  const S = useMemo(() => fold(st.cursor, st.ov), [st.cursor, st.ov]);
  const ref = useRef({}); ref.current = { st, S };

  /* ---------- view assembly (pure, from st+S) ---------- */
  const view = useMemo(() => {
    const cardIdx = clamp(st.card, 0, st.spaces[st.ws].cards.length - 1);
    const curCard = st.spaces[st.ws].cards[cardIdx];
    const app = curCard.app;
    let lines, title;
    if (st.mode === "help") {
      lines = [L([T("KEY MAP — any key returns", "hdr")]), L([T(" ")])];
      HELP.forEach(([k, v]) => lines.push(v === "" ? L([T("· " + k, "hdr")]) : L([T("  " + k.padEnd(18)), T(v, "dim")])));
      title = "HELP";
    } else if (st.mode === "overview") {
      lines = [L([T("OVERVIEW · " + st.spaces.length + " workspaces", "hdr")]), L([T(" ")])];
      st.spaces.forEach((sp, wi) => {
        lines.push(L([T(GLYPH.ws + " " + sp.name, { color: LCD.sage, fontWeight: 700 })]));
        sp.cards.forEach((c) => lines.push(L([T("   " + GLYPH.card + " " + APP_TITLE[c.app].padEnd(10)), T("│ " + vitals(c.app, S, st), "dim"), FILL, T(c.id === curCard.id ? "· you are here" : "", { color: LCD.mustard })], { pt: "card", v: c.id })));
        lines.push(L([T(" ")]));
      });
      lines.push(L([T("⏎ dive · m verbs · ↑↓ choose · o / esc close", "faint")]));
      title = "OVERVIEW";
    } else if (st.stack.length) {
      const top = st.stack[st.stack.length - 1];
      lines = describe(top.pt, top.v, S, st);
      title = "inspect";
    } else {
      lines = appLines(app, S, st);
      title = APP_TITLE[app];
    }
    /* edits app: append live detail of the caret hunk */
    const presIdx = []; lines.forEach((l, i) => { if (l && l.pres) presIdx.push(i); });
    const vkey = st.mode === "overview" ? "ov" : st.mode === "help" ? "help" : st.ws + ":" + st.card + ":" + st.stack.length;
    let caret = clamp(st.carets[vkey] ?? 0, 0, Math.max(0, presIdx.length - 1));
    if (!st.stack.length && st.mode !== "overview" && st.mode !== "help" && app === "edits" && presIdx.length) {
      const e = S.edits[caret];
      if (e) {
        lines.push(L([T(GLYPH.hunk + " " + e.sym + " · ", "hdr"), T(e.sem, { color: SEMTONE[e.sem] || LCD.dim, fontWeight: 700 }), T("   " + e.path, { color: LCD.mint })]));
        lines.push(L([T((SEM_BLURB[e.sem] || "") + (e.reverted ? "  · REVERTED (R restores)" : ""), "dim")]));
        e.del.slice(0, 5).forEach((t) => lines.push(L([T("− " + t, "del")], null, "del")));
        e.add.slice(0, 10).forEach((t) => lines.push(L([T("+ " + t, "add")], null, "add")));
      }
    }
    return { lines, presIdx, caret, vkey, title, app };
  }, [st, S]);

  const caretPres = view.presIdx.length ? view.lines[view.presIdx[view.caret]].pres : null;

  /* ---------- key handling ---------- */
  useEffect(() => {
    const down = (e) => {
      const { st, S } = ref.current;
      const k = e.key;
      const eaten = ["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", " ", "Tab", "Backspace", "/", "?", "'", "Enter"].includes(k);
      if (eaten) e.preventDefault();
      setSt((s0) => {
        let s = { ...s0 };
        SP = s.spaces;
        s.card = clamp(s.card, 0, s.spaces[s.ws].cards.length - 1);
        const curCard = s.spaces[s.ws].cards[s.card];
        const app = curCard.app;
        const vkey = s.mode === "overview" ? "ov" : s.mode === "help" ? "help" : s.ws + ":" + s.card + ":" + s.stack.length;
        /* recompute pres list cheaply for handler */
        let lines;
        if (s.mode === "overview") { lines = []; s.spaces.forEach((sp) => sp.cards.forEach((c) => lines.push({ pres: { pt: "card", v: c.id } }))); }
        else if (s.mode === "help") lines = [];
        else if (s.stack.length) lines = describe(s.stack[s.stack.length - 1].pt, s.stack[s.stack.length - 1].v, S, s).filter((l) => l.pres).map((l) => ({ pres: l.pres }));
        else lines = appLines(app, S, s).filter((l) => l && l.pres).map((l) => ({ pres: l.pres }));
        const nPres = lines.length;
        let caret = clamp(s.carets[vkey] ?? 0, 0, Math.max(0, nPres - 1));
        const setCaret = (i) => { caret = clamp(i, 0, Math.max(0, nPres - 1)); s.carets = { ...s.carets, [vkey]: caret }; };
        const cur = nPres ? lines[caret].pres : null;
        const touch = (o) => { if (o) s.hist = { ...s.hist, [o.pt]: o.v }; };
        const onTray = (o) => !!o && s.tray.some((x) => x.pt === o.pt && x.v === o.v);
        const trayCands = () => s.tray.map((o) => ({ pt: o.pt, v: o.v, label: labelFor(o.pt, o.v, S) }));
        const enterAccept = (cmd, fromCmd) => {
          const types = CMDS[cmd].types;
          let dflt = null;
          if (cmd === "drop") dflt = cur && onTray(cur) ? { ...cur } : s.tray.length ? { pt: s.tray[s.tray.length - 1].pt, v: s.tray[s.tray.length - 1].v } : null;
          else if (cur && types.includes(cur.pt)) dflt = { ...cur }; // the caret is "it"
          else if (types.includes("card") && (cmd === "close" || cmd === "open")) dflt = { pt: "card", v: curCard.id }; // the current tile offers itself
          else for (const t of types) {
            if (s.hist[t] !== undefined && catalog([t], S).find((c) => c.v === s.hist[t])) { dflt = { pt: t, v: s.hist[t] }; break; }
          }
          s.mode = "accept"; s.accept = { cmd, types, buf: "", sel: 0, dflt, fromCmd: !!fromCmd };
        };
        const acceptCands = () => {
          const a = s.accept; if (!a) return [];
          const q = a.buf.toLowerCase();
          /* pronouns resolve in the argument slot: $n reaches into the
             tray, "it" nominates the caret */
          if (/^\$\d+$/.test(q)) {
            const t = s.tray[parseInt(q.slice(1)) - 1];
            return t && a.types.includes(t.pt) ? [{ pt: t.pt, v: t.v, label: labelFor(t.pt, t.v, S) + " · " + q }] : [];
          }
          const base = a.cmd === "drop" ? trayCands() : catalog(a.types, S);
          let m = base.filter((c) => c.label.toLowerCase().includes(q));
          if (a.cmd !== "drop") m.sort((x, y) => (y.v === s.hist[y.pt] ? 1 : 0) - (x.v === s.hist[x.pt] ? 1 : 0));
          if (q === "it" && cur && a.types.includes(cur.pt) && (a.cmd !== "drop" || onTray(cur)))
            m = [{ pt: cur.pt, v: cur.v, label: labelFor(cur.pt, cur.v, S) + " · it" }, ...m];
          return m;
        };

        /* ---- help: any key closes ---- */
        if (s.mode === "help") { s.mode = "nav"; return s; }
        /* ---- ESC ladder ---- */
        if (k === "Escape") {
          if (s.mode !== "nav") { s.mode = "nav"; s.accept = null; s.menu = null; s.hintBuf = ""; s.searchBuf = ""; s.cmdBuf = ""; s.scopeWait = false; return s; }
          if (s.peek) { s.peek = false; return s; }
          if (s.stack.length) { s.stack = s.stack.slice(0, -1); return s; }
          return s;
        }

        /* ---- CMD minibuffer ---- */
        if (s.mode === "cmd") {
          /* completion is scanned off the screen: only commands something
             visible (or the ever-present current tile) can receive */
          const avail = availCmds(lines.map((l) => l.pres.pt), s.tray.length);
          const prefix = s.cmdBuf.split(" ")[0];
          if (k === "Enter") {
            const buf = s.cmdBuf.trim(); s.mode = "nav"; s.cmdBuf = "";
            if (!buf) return s;
            const [name0, ...rest] = buf.split(/\s+/); const argStr = rest.join(" ");
            const uniq = !CMDS[name0] && avail.filter((n) => n.startsWith(name0));
            const name = CMDS[name0] ? name0 : uniq && uniq.length === 1 ? uniq[0] : null;
            if (!name) { s.toast = "no such command: " + name0 + "  (? for help)"; return s; }
            const cmd = CMDS[name];
            if (!cmd.types.length) return runCmd(s, S, name, null);
            let obj = null;
            if (argStr === "it" && cur && cmd.types.includes(cur.pt)) obj = cur;
            else if (/^\$\d+$/.test(argStr)) { const t = s.tray[parseInt(argStr.slice(1)) - 1]; if (t && cmd.types.includes(t.pt)) obj = t; }
            else if (argStr) { const src = name === "drop" ? trayCands() : catalog(cmd.types, S); const m = src.filter((c) => c.label.toLowerCase().includes(argStr.toLowerCase())); if (m.length) obj = { pt: m[0].pt, v: m[0].v }; }
            if (obj) { touch(obj); return runCmd(s, S, name, obj); }
            enterAccept(name, true); return s;
          }
          if (k === "Backspace") { s.cmdBuf = s.cmdBuf.slice(0, -1); return s; }
          if (k === "Tab") {
            const names = avail.filter((n) => n.startsWith(prefix));
            if (!s.cmdBuf.includes(" ") && names.length) { s.cmdSel = (s.cmdSel + 1) % names.length; s.cmdBuf = names[s.cmdSel % names.length]; }
            return s;
          }
          if (k === " ") {
            /* space = "next argument": complete the verb, then light up
               every visible object that can receive it */
            const names = avail.filter((n) => n.startsWith(prefix));
            const name = CMDS[prefix] ? prefix : names.length === 1 ? names[0] : null;
            if (!name) { s.toast = names.length ? "ambiguous: " + names.join(" · ") : "unknown command: " + prefix; return s; }
            if (!CMDS[name].types.length) { s.mode = "nav"; s.cmdBuf = ""; return runCmd(s, S, name, null); }
            s.cmdBuf = ""; enterAccept(name, true); return s;
          }
          if (k.length === 1) { s.cmdBuf += k; s.cmdSel = -1; return s; }
          return s;
        }

        /* ---- ACCEPT mode ---- */
        if (s.mode === "accept") {
          const a = s.accept;
          /* digits quick-pick lit targets only while the arg buffer is
             empty; once you start typing (a filter, $n, or it) digits
             are literal text */
          if (k >= "1" && k <= "9" && a.buf === "") {
            const matches = lines.map((l) => l.pres).filter((p) => a.types.includes(p.pt) && (a.cmd !== "drop" || onTray(p))).slice(0, 9);
            const pick = matches[parseInt(k) - 1];
            if (pick) { s.mode = "nav"; s.accept = null; touch(pick); return runCmd(s, S, a.cmd, pick); }
            return s;
          }
          if (k === "Tab") { a.sel = a.sel + 1; s.accept = { ...a }; return s; }
          if (k === "Backspace") { s.accept = { ...a, buf: a.buf.slice(0, -1), sel: 0 }; return s; }
          if (k === "Enter") {
            const cands = acceptCands();
            const dfltOk = a.dflt && (a.cmd === "drop" ? onTray(a.dflt) : catalog(a.types, S).find((c) => c.pt === a.dflt.pt && c.v === a.dflt.v));
            const pick = a.buf ? cands[a.sel % Math.max(1, cands.length)] : (dfltOk ? a.dflt : cands[a.sel % Math.max(1, cands.length)]);
            s.mode = "nav"; s.accept = null;
            if (pick) { const obj = { pt: pick.pt, v: pick.v }; touch(obj); return runCmd(s, S, a.cmd, obj); }
            s.toast = "nothing matched"; return s;
          }
          if (k.length === 1) { s.accept = { ...a, buf: a.buf + k, sel: 0 }; return s; }
          return s;
        }

        /* ---- SEARCH ---- */
        if (s.mode === "search") {
          if (k === "Enter") {
            const q = s.searchBuf.toLowerCase(); s.mode = "nav"; s.searchBuf = "";
            const i = lines.findIndex((l) => labelFor(l.pres.pt, l.pres.v, S).toLowerCase().includes(q));
            if (i >= 0) setCaret(i); else s.toast = "no label matches here";
            return s;
          }
          if (k === "Backspace") { s.searchBuf = s.searchBuf.slice(0, -1); return s; }
          if (k.length === 1) { s.searchBuf += k; return s; }
          return s;
        }

        /* ---- HINT ---- */
        if (s.mode === "hint") {
          if (k.length === 1 && /[a-z]/.test(k)) {
            const buf = s.hintBuf + k;
            const hit = s.hints.findIndex((h) => h === buf);
            if (hit >= 0) { s.mode = "nav"; s.hintBuf = ""; setCaret(hit); touch(lines[hit] && lines[hit].pres); }
            else if (s.hints.some((h) => h.startsWith(buf))) s.hintBuf = buf;
            else { s.mode = "nav"; s.hintBuf = ""; s.toast = "no such hint"; }
            return s;
          }
          return s;
        }

        /* ---- MENU ---- */
        if (s.mode === "menu" && s.menu) {
          const items = s.menu.items;
          if (k === "ArrowDown" || k === "j") { s.menu = { ...s.menu, sel: (s.menu.sel + 1) % items.length }; return s; }
          if (k === "ArrowUp" || k === "k") { s.menu = { ...s.menu, sel: (s.menu.sel - 1 + items.length) % items.length }; return s; }
          if (k === "Enter") { const it = items[s.menu.sel]; s.mode = "nav"; s.menu = null; return it.run(s); }
          const it = items.find((x) => x.k === k);
          if (it) { s.mode = "nav"; s.menu = null; return it.run(s); }
          return s;
        }

        /* ---- OVERVIEW ---- */
        if (s.mode === "overview") {
          if (k === "ArrowDown" || k === "j") { setCaret(caret + 1); return s; }
          if (k === "ArrowUp" || k === "k") { setCaret(caret - 1); return s; }
          if (k === "o") { s.mode = "nav"; return s; }
          if (k === "m" && cur) { s.mode = "menu"; s.menu = { items: menuFor(cur.pt, cur.v, S, s), sel: 0, label: labelFor(cur.pt, cur.v, S), pt: cur.pt }; return s; }
          if (k === "Enter" && cur) { const f = findCard(s.spaces, cur.v); if (f) { s.ws = f.wi; s.card = f.ci; s.stack = []; s.mode = "nav"; } return s; }
          return s;
        }

        /* ---- type-scope (; x) ---- */
        if (s.scopeWait) {
          s.scopeWait = false;
          const map = { f: "file", h: "hunk", t: "task", m: "mem", c: "ctxseg", s: "step", o: "toolcall" };
          const pt = map[k];
          if (pt) {
            for (let d = 1; d <= nPres; d++) { const i = (caret + d) % nPres; if (lines[i].pres.pt === pt) { setCaret(i); return s; } }
            s.toast = "no <" + pt + "> on this card";
          }
          return s;
        }

        /* ---- NAV ---- */
        if (k === "ArrowDown" || k === "j") { setCaret(caret + 1); return s; }
        if (k === "ArrowUp" || k === "k") { setCaret(caret - 1); return s; }
        if (k === "ArrowRight" || k === "]") { s.card = (s.card + 1) % s.spaces[s.ws].cards.length; s.stack = []; return s; }
        if (k === "ArrowLeft" || k === "[") { s.card = (s.card - 1 + s.spaces[s.ws].cards.length) % s.spaces[s.ws].cards.length; s.stack = []; return s; }
        if (k === "Tab") { s.ws = (s.ws + 1) % s.spaces.length; s.card = 0; s.stack = []; return s; }
        if (k >= "1" && k <= "9") { const i = parseInt(k) - 1; if (s.spaces[s.ws].cards[i]) { s.card = i; s.stack = []; } return s; }
        if (k === "Backspace") { if (s.stack.length) s.stack = s.stack.slice(0, -1); return s; }
        if (k === "Enter" && cur) {
          touch(cur);
          if (cur.pt === "task") {
            const t = S.tasks.find((x) => x.id === cur.v);
            const next = t.status === "todo" ? "doing" : t.status === "doing" ? "done" : "todo";
            s.ov = { ...s.ov, taskSet: { ...s.ov.taskSet, [cur.v]: next } }; s.toast = t.title + " → " + next; return s;
          }
          if (cur.pt === "step") return runCmd(s, S, "goto", cur);
          if (cur.pt === "card") return runCmd(s, S, "switch", cur);
          return runCmd(s, S, "open", cur);
        }
        if (k === "f") {
          const alpha = "asdfghjklqwertyuiopzxcvbnm";
          s.hints = lines.map((_, i) => (nPres <= 26 ? alpha[i] : alpha[Math.floor(i / 26)] + alpha[i % 26]));
          s.mode = "hint"; s.hintBuf = ""; return s;
        }
        if (k === ";") { s.scopeWait = true; return s; }
        if (k === "/") { s.mode = "search"; s.searchBuf = ""; return s; }
        if (k === ":") { s.mode = "cmd"; s.cmdBuf = ""; s.cmdSel = -1; return s; }
        if (k === "m" && cur) { s.mode = "menu"; s.menu = { items: menuFor(cur.pt, cur.v, S, s), sel: 0, label: labelFor(cur.pt, cur.v, S), pt: cur.pt }; return s; }
        if (k === "o") { s.mode = "overview"; return s; }
        if (k === "?") { s.mode = "help"; return s; }
        if (k === "y" && cur) { touch(cur); return runCmd(s, S, "yank", cur); }
        if (k === "x" && cur) {
          if (onTray(cur)) { touch(cur); return runCmd(s, S, "drop", cur); }
          s.toast = "not on the tray — y puts it there"; return s;
        }
        if (k === "t") { s.trayOpen = !s.trayOpen; return s; }
        if (k === "i" && !e.repeat) { s.peek = true; return s; }
        if (k === " ") { s.playing = !s.playing; if (s.playing && s.cursor >= MAXGI) s.cursor = 0; return s; }
        if (k === ",") { s.cursor = clamp(s.cursor - 1, 0, MAXGI); s.playing = false; return s; }
        if (k === ".") { s.cursor = clamp(s.cursor + 1, 0, MAXGI); s.playing = false; return s; }
        if (k === "<") { const ends = STEPS.map((x) => stepEnd[x.id]).filter((g) => g < s.cursor); s.cursor = ends.length ? ends[ends.length - 1] : 0; s.playing = false; return s; }
        if (k === ">") { const ends = STEPS.map((x) => stepEnd[x.id]).filter((g) => g > s.cursor); s.cursor = ends.length ? ends[0] : MAXGI; s.playing = false; return s; }
        if (k === "R" && cur && cur.pt === "hunk") { const ed = S.edits.find((x) => x.id === cur.v); touch(cur); return runCmd(s, S, ed && ed.reverted ? "restore" : "revert", cur); }
        if (k === "P" && cur && (cur.pt === "mem" || cur.pt === "ctxseg")) {
          const pinned = cur.pt === "mem" ? (S.memory.find((x) => x.id === cur.v) || {}).pinned : (S.ctx.find((x) => x.id === cur.v) || {}).pinned;
          touch(cur); return runCmd(s, S, pinned ? "unpin" : "pin", cur);
        }
        if (k === "E" && cur && cur.pt === "ctxseg") { touch(cur); return runCmd(s, S, "evict", cur); }
        if (k === "r" && s.lastCmd && cur) {
          const cmd = CMDS[s.lastCmd];
          if (cmd && cmd.types.includes(cur.pt)) { touch(cur); return runCmd(s, S, s.lastCmd, cur); }
          s.toast = "last cmd (" + s.lastCmd + ") doesn't take <" + (cur ? cur.pt : "?") + ">"; return s;
        }
        return s;
      });
    };
    const up = (e) => { if (e.key === "i") setSt((s) => (s.peek ? { ...s, peek: false } : s)); };
    window.addEventListener("keydown", down);
    window.addEventListener("keyup", up);
    return () => { window.removeEventListener("keydown", down); window.removeEventListener("keyup", up); };
  }, []);

  /* transport playback */
  useEffect(() => {
    if (!st.playing) return;
    const iv = setInterval(() => setSt((s) => (s.cursor >= MAXGI ? { ...s, playing: false } : { ...s, cursor: s.cursor + 1 })), 550);
    return () => clearInterval(iv);
  }, [st.playing]);
  /* toast expiry */
  useEffect(() => {
    if (!st.toast) return;
    const t = setTimeout(() => setSt((s) => ({ ...s, toast: null })), 2100);
    return () => clearTimeout(t);
  }, [st.toast]);

  /* ---------- render ---------- */
  const caretLine = view.presIdx.length ? view.presIdx[view.caret] : 0;
  const stored = st.scrolls[view.vkey] ?? 0;
  const scroll = clamp(clamp(stored, caretLine - (CONTENT_ROWS - 1), caretLine), 0, Math.max(0, view.lines.length - CONTENT_ROWS));
  const visible = view.lines.slice(scroll, scroll + CONTENT_ROWS);

  const segStyle = (s) => {
    if (s && typeof s === "object") return s;
    if (typeof s === "string" && s[0] === "#") return { color: s };
    return {
      hdr: { fontWeight: 700, letterSpacing: "0.04em" },
      dim: { color: LCD.dim }, faint: { color: LCD.faint },
      add: { color: LCD.add, fontWeight: 600 }, del: { color: LCD.del },
      green: { color: LCD.green, fontWeight: 600 }, red: { color: LCD.red, fontWeight: 600 },
      inv: { background: LCD.ink, color: LCD.bg, padding: "0 2px" },
    }[s] || {};
  };
  const LINE_BG = { add: LCD.addBg, del: LCD.delBg, sel: LCD.sel };
  const onTrayR = (p) => !!p && st.tray.some((x) => x.pt === p.pt && x.v === p.v);

  const acceptCands = st.accept ? (() => {
    const a = st.accept, q = a.buf.toLowerCase();
    if (/^\$\d+$/.test(q)) {
      const t = st.tray[parseInt(q.slice(1)) - 1];
      return t && a.types.includes(t.pt) ? [{ pt: t.pt, v: t.v, label: labelFor(t.pt, t.v, S) + " · " + q }] : [];
    }
    const base = a.cmd === "drop"
      ? st.tray.map((o) => ({ pt: o.pt, v: o.v, label: labelFor(o.pt, o.v, S) }))
      : catalog(a.types, S);
    let m = base.filter((c) => c.label.toLowerCase().includes(q));
    if (a.cmd !== "drop") m.sort((x, y) => (y.v === st.hist[y.pt] ? 1 : 0) - (x.v === st.hist[x.pt] ? 1 : 0));
    if (q === "it" && caretPres && a.types.includes(caretPres.pt) && (a.cmd !== "drop" || onTrayR(caretPres)))
      m = [{ pt: caretPres.pt, v: caretPres.v, label: labelFor(caretPres.pt, caretPres.v, S) + " · it" }, ...m];
    return m;
  })() : [];

  const renderLine = (l, i) => {
    const abs = scroll + i;
    const isCaret = view.presIdx.length && abs === caretLine && st.mode !== "overview" ? true : (st.mode === "overview" && l.pres && view.presIdx[view.caret] === abs);
    const isPres = !!l.pres;
    let acceptable = st.mode === "accept" && isPres && st.accept.types.includes(l.pres.pt) && (st.accept.cmd !== "drop" || onTrayR(l.pres));
    /* as you type in the arg slot, the lit set narrows with you:
       a filter dims non-matches, $n lights exactly that tray entry,
       "it" lights the caret */
    if (acceptable && st.accept.buf) {
      const q = st.accept.buf.toLowerCase();
      if (/^\$\d+$/.test(q)) { const t = st.tray[parseInt(q.slice(1)) - 1]; acceptable = !!t && t.pt === l.pres.pt && t.v === l.pres.v; }
      else if (q === "it") acceptable = l.pres === caretPres;
      else acceptable = labelFor(l.pres.pt, l.pres.v, S).toLowerCase().includes(q);
    }
    let hintChip = null;
    if (st.mode === "hint" && isPres) {
      const pi = view.presIdx.indexOf(abs);
      if (pi >= 0 && st.hints[pi] && st.hints[pi].startsWith(st.hintBuf)) hintChip = st.hints[pi];
    }
    if (acceptable && st.accept.buf === "") {
      const matches = view.presIdx.map((x) => view.lines[x].pres).filter((p) => st.accept.types.includes(p.pt) && (st.accept.cmd !== "drop" || onTrayR(p))).slice(0, 9);
      const n = matches.findIndex((p) => p === l.pres);
      if (n >= 0) hintChip = String(n + 1);
    }
    /* every presentation wears its type tone as a left bar — the
       desktop's PTONE border, one cell narrower */
    const bar = isPres ? (TONE[l.pres.pt] || LCD.ink) : "transparent";
    return (
      <div key={i} style={{
        height: 19, lineHeight: "19px", whiteSpace: "pre", overflow: "hidden", display: "flex",
        borderLeft: "3px solid " + bar, paddingLeft: 3, paddingRight: 6,
        background: isCaret || acceptable ? LCD.sel : (l.bg && LINE_BG[l.bg]) || "transparent",
        outline: acceptable ? "2px solid " + LCD.red : isCaret ? "1px dotted " + LCD.ink : "none",
        outlineOffset: -2,
        animation: acceptable ? "pbuipulse 0.9s infinite" : "none",
        position: "relative", zIndex: acceptable ? 1 : 0,
      }}>
        {hintChip && <span style={{ background: acceptable ? LCD.red : LCD.ink, color: LCD.bg, fontWeight: 700, padding: "0 3px", marginRight: 4 }}>{hintChip}</span>}
        {l.segs.map((sg, j) => sg.fill
          ? <span key={j} style={{ flex: 1 }} />
          : <span key={j} style={segStyle(sg.s)}>{sg.t}</span>)}
      </div>
    );
  };

  /* doc line — the desktop footer: ink bar, mustard mode word */
  const doc = (() => {
    if (st.toast) return { m: "✓", t: st.toast };
    if (st.mode === "cmd") {
      const avail = availCmds(view.presIdx.map((i) => view.lines[i].pres.pt), st.tray.length);
      const names = avail.filter((n) => n.startsWith(st.cmdBuf.split(" ")[0])).slice(0, 6);
      const sig = names.map((n) => n + (CMDS[n].types.length ? "·" + (GLYPH[CMDS[n].types[0]] || "▸") : "")).join(" ");
      return { m: "CMD", t: "» " + st.cmdBuf + "▁  ␣=arg  " + (st.cmdBuf.includes(" ") ? "" : sig) };
    }
    if (st.mode === "accept") {
      const a = st.accept, c = acceptCands[a.sel % Math.max(1, acceptCands.length)];
      const curCardId = st.spaces[st.ws].cards[clamp(st.card, 0, st.spaces[st.ws].cards.length - 1)].id;
      const take = a.buf ? (c ? c.label : "∅") : a.dflt ? labelFor(a.dflt.pt, a.dflt.v, S) + (a.cmd === "drop" ? " · $" + (st.tray.findIndex((x) => x.pt === a.dflt.pt && x.v === a.dflt.v) + 1) : a.dflt.pt === "card" && a.dflt.v === curCardId ? " · this tile" : " · it") : c ? c.label : "∅";
      return { m: "ACCEPT", t: "» " + a.cmd + " <" + (a.cmd === "drop" ? "tray" : a.types.join("|")) + "> " + a.buf + "▁ [⏎ " + take.slice(0, 18) + "] " + acceptCands.length + " cand · " + (a.buf === "" ? "digits pick lit" : "narrowing lit") + " · $n it ok · ⇥ cycle" };
    }
    if (st.mode === "hint") return { m: "HINT", t: "type a label · esc cancels   " + st.hintBuf };
    if (st.mode === "search") return { m: "SEARCH", t: "/" + st.searchBuf + "▁ · ⏎ jumps the caret" };
    if (st.mode === "menu") return { m: "MENU", t: "letter or ↑↓ + ⏎ · esc" };
    if (st.mode === "overview") return { m: "OVERVIEW", t: "⏎ dive · m verbs · ↑↓ choose · o close" };
    if (st.mode === "help") return { m: "HELP", t: "any key returns" };
    if (st.scopeWait) return { m: "SCOPE", t: "; + f file · h hunk · t task · m mem · c seg · s step" };
    if (caretPres) {
      const verbs = ({ hunk: "R revert", mem: "P pin", ctxseg: "P pin E evict", task: "⏎ cycle" }[caretPres.pt] || "") + (onTrayR(caretPres) ? " x drop" : " y yank");
      return { m: st.playing ? "PLAYING" : "READY", t: "<" + caretPres.pt + "> " + labelFor(caretPres.pt, caretPres.v, S).slice(0, 18) + " · ⏎ " + defaultVerb(caretPres.pt) + " " + verbs + " · m menu · : cmd · ? help" };
    }
    return { m: st.playing ? "PLAYING" : "READY", t: "↑↓ caret · ←→ cards · : cmd · o overview · ? help" };
  })();

  const stepBar = STEPS.map((s, i) => (i + 1 <= S.executed ? "▮" : "▯")).join("");
  const breadcrumb = st.stack.map((o) => labelFor(o.pt, o.v, S).slice(0, 14)).join(" ▸ ");
  const peekLines = st.peek && caretPres ? describe(caretPres.pt, caretPres.v, S, st).slice(0, 8) : null;

  return (
    <div style={{ minHeight: "100vh", background: LCD.desk, display: "flex", flexDirection: "column", alignItems: "center", padding: "28px 12px", fontFamily: "ui-monospace, 'Cascadia Mono', Menlo, Consolas, monospace" }}>
      <style>{`@keyframes pbuipulse { 50% { outline-color: ${LCD.mustard}; } }
        @media (prefers-reduced-motion: reduce) { * { animation: none !important; } }`}</style>

      {/* device */}
      <div style={{ background: LCD.bezel, borderRadius: 22, padding: "18px 18px 14px", boxShadow: "0 18px 50px rgba(0,0,0,0.55), inset 0 1px 0 " + LCD.bezel2 }}>
        <div style={{ color: LCD.silk, fontSize: 10, letterSpacing: "0.3em", textAlign: "center", marginBottom: 8 }}>P B U I / H B · 320 × 320 · NO POINTER</div>

        {/* screen */}
        <div tabIndex={0} style={{ width: "53ch", fontSize: 13, background: LCD.bg, color: LCD.ink, borderRadius: 3, border: "3px solid #1a1c20", boxShadow: "inset 0 1px 6px rgba(0,0,0,0.14)", outline: "none", position: "relative", overflow: "hidden" }}>
          {/* row 0: status — the desktop's ink top bar */}
          <div style={{ height: 19, lineHeight: "19px", display: "flex", whiteSpace: "pre", background: LCD.ink, color: LCD.bg, padding: "0 6px" }}>
            <b style={{ letterSpacing: "0.18em" }}>PBUI</b>
            <span style={{ color: LCD.mustard, marginLeft: 8, letterSpacing: "0.08em" }}>GRACE-PERIOD</span>
            <span style={{ flex: 1 }} />
            <span>{st.playing ? <span style={{ color: LCD.mustard }}>▶ </span> : ""}§{S.executed}/{STEPS.length} {stepBar} ev{st.cursor}/{MAXGI}</span>
            <span style={{ color: S.ctxTok / S.budget > 0.9 ? LCD.rose : S.ctxTok / S.budget > 0.7 ? LCD.mustard : LCD.mint }}>  ctx {kfmt(S.ctxTok)}</span>
          </div>
          {/* row 1: title */}
          <div style={{ height: 19, lineHeight: "19px", display: "flex", whiteSpace: "pre", color: LCD.dim, borderBottom: "1px solid " + LCD.line, padding: "0 6px" }}>
            <span><span style={{ color: LCD.sage, fontWeight: 700 }}>{GLYPH.ws}{st.spaces[st.ws].name}</span> ▸ <b style={{ color: LCD.ink }}>{view.title}</b>{breadcrumb ? " ▸ " + breadcrumb : ""}</span>
            <span style={{ flex: 1 }} />
            {st.mode !== "overview" && st.mode !== "help" && <span>card {clamp(st.card, 0, st.spaces[st.ws].cards.length - 1) + 1}/{st.spaces[st.ws].cards.length}</span>}
          </div>
          {/* content */}
          <div style={{ height: 19 * CONTENT_ROWS, overflow: "hidden", position: "relative" }}>
            {visible.map(renderLine)}
            {/* menu overlay */}
            {st.mode === "menu" && st.menu && (
              <div style={{ position: "absolute", right: 6, top: 12, background: LCD.bg, border: "2px solid " + LCD.ink, boxShadow: "4px 4px 0 " + LCD.ink, minWidth: "28ch", zIndex: 5 }}>
                <div style={{ background: LCD.ink, color: LCD.bg, padding: "0 6px", height: 19, lineHeight: "19px", fontWeight: 700 }}>
                  {"<" + st.menu.pt + "> "}<span style={{ color: TONE[st.menu.pt] || LCD.mustard }}>{st.menu.label.slice(0, 20)}</span>
                </div>
                {st.menu.items.map((it, i) => (
                  <div key={i} style={{ padding: "0 6px", height: 19, lineHeight: "19px", background: i === st.menu.sel ? LCD.sel : "transparent", color: LCD.ink, borderTop: i ? "1px dotted " + LCD.line : "none" }}>
                    <b>{it.k}</b>  {it.label}
                  </div>
                ))}
              </div>
            )}
            {/* peek overlay */}
            {peekLines && (
              <div style={{ position: "absolute", left: 6, right: 6, bottom: 4, background: LCD.bg, border: "2px solid " + LCD.ink, boxShadow: "4px 4px 0 " + LCD.ink, zIndex: 4, padding: "2px 4px" }}>
                <div style={{ height: 19, lineHeight: "19px", color: LCD.dim }}>┌╌ peek — release i to close ╌┐</div>
                {peekLines.map((l, i) => (
                  <div key={i} style={{ height: 19, lineHeight: "19px", whiteSpace: "pre", overflow: "hidden", display: "flex" }}>
                    {l.segs.map((sg, j) => sg.fill ? <span key={j} style={{ flex: 1 }} /> : <span key={j} style={segStyle(sg.s)}>{sg.t}</span>)}
                  </div>
                ))}
              </div>
            )}
          </div>
          {/* row 30: tray — chips wear the desktop Pres chip look */}
          <div style={{ height: 19, lineHeight: "17px", whiteSpace: "nowrap", overflow: "hidden", borderTop: "1px solid " + LCD.line, padding: "0 6px", display: "flex", alignItems: "center", gap: 6 }}>
            {st.trayOpen ? (st.tray.length ? (
              <>
                <span style={{ color: LCD.dim, fontWeight: 700, fontSize: 11 }}>TRAY</span>
                {st.tray.map((o, i) => (
                  <span key={i} style={{ display: "inline-flex", alignItems: "center" }}>
                    <span style={{ color: LCD.dim, fontSize: 11 }}>{"$" + (i + 1) + " "}</span>
                    <span style={{ background: LCD.bg, border: "1px solid " + LCD.ink, borderLeft: "4px solid " + (TONE[o.pt] || LCD.ink), padding: "0 4px", fontSize: 11 }}>
                      {GLYPH[o.pt] + " " + labelFor(o.pt, o.v, S).slice(0, 12)}
                    </span>
                  </span>
                ))}
              </>
            ) : <span style={{ color: LCD.dim }}>TRAY · empty — y yanks the caret here</span>) : <span> </span>}
          </div>
          {/* row 31: doc line — the desktop footer bar */}
          <div style={{ height: 19, lineHeight: "19px", whiteSpace: "pre", overflow: "hidden", background: LCD.ink, color: LCD.bg, padding: "0 6px", display: "flex" }}>
            <b style={{ color: LCD.mustard, marginRight: 8, letterSpacing: "0.06em" }}>{doc.m}</b>
            <span style={{ overflow: "hidden", textOverflow: "ellipsis" }}>{doc.t}</span>
          </div>
        </div>

        {/* silkscreen key legend */}
        <div style={{ color: LCD.silk, fontSize: 10.5, marginTop: 12, width: "53ch", lineHeight: 1.7 }}>
          <div style={{ display: "flex", gap: 14, flexWrap: "wrap" }}>
            <span><b style={{ color: "#c8cdd4" }}>↑↓</b> caret</span>
            <span><b style={{ color: "#c8cdd4" }}>←→</b> cards</span>
            <span><b style={{ color: "#c8cdd4" }}>⏎</b> act</span>
            <span><b style={{ color: "#c8cdd4" }}>f</b> hints</span>
            <span><b style={{ color: "#c8cdd4" }}>:</b> cmd</span>
            <span><b style={{ color: "#c8cdd4" }}>m</b> menu</span>
            <span><b style={{ color: "#c8cdd4" }}>o</b> overview</span>
            <span><b style={{ color: "#c8cdd4" }}>i</b> hold=peek</span>
            <span><b style={{ color: "#c8cdd4" }}>y x t</b> tray</span>
            <span><b style={{ color: "#c8cdd4" }}>space , . &lt; &gt;</b> transport</span>
            <span><b style={{ color: "#c8cdd4" }}>?</b> help</span>
          </div>
          <div style={{ marginTop: 4, color: "#6d737d" }}>
            try: <b>:</b> rev<b>␣</b> — the verb completes and every target lights up (digit picks, ⏎ takes the default) · <b>:</b> close<b>␣⏎</b> closes <i>this</i> tile · <b>:</b> newtile<b>␣</b>ed<b>⏎</b> opens a second EDITS · <b>&lt;&lt;</b> then <b>space</b> replays the run
          </div>
        </div>
      </div>
    </div>
  );
}
