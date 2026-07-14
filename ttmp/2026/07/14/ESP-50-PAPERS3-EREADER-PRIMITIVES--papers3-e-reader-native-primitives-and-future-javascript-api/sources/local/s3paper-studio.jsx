import React, { useCallback, useEffect, useRef, useState } from "react";

/* ============================================================================
   s3paper runtime — portable core
   ----------------------------------------------------------------------------
   Pipeline (designed to port 1:1 to the device):

     builders  →  plain JSON widget tree  →  layout pass  →  flat draw-op list
                                                              { rect | srect |
                                                                text | dots }
                                                          →  backend executes

   Only the backend (here: <canvas>) touches the display. On hardware you swap
   it for an EPD framebuffer writer; builders, layout, refresh policy, ghosting
   accounting and gesture dispatch stay untouched. No DOM, no CSS, no closures
   required below the builder layer — lambdas are resolved during layout into
   plain values.
   ========================================================================== */

const DW = 540;
const DH = 960;
const GRAY = ["#181914", "#585951", "#a5a49b", "#e8e6db"]; // g0 ink … g3 paper
const SIZES = { xs: 16, sm: 21, md: 27, lg: 38, xl: 54 };
const SANS = '"Helvetica Neue", Helvetica, Arial, sans-serif';
const SERIF = 'Georgia, "Times New Roman", serif';

const resv = (v) => (typeof v === "function" ? v() : v);
const clampG = (g, inv) => {
  g = Math.max(0, Math.min(3, g | 0));
  return inv ? 3 - g : g;
};
const fontFor = (p) => {
  const px = SIZES[p.size] || SIZES.md;
  return {
    px,
    css: `${p.weight === "bold" ? 700 : 400} ${px}px ${p.serif ? SERIF : SANS}`,
  };
};
const offsetOps = (ops, dx, dy) =>
  ops.map((o) => ({ ...o, x: o.x + dx, y: o.y + dy }));

function parseEvery(s) {
  const m = /^(\d+)\s*(ms|s|m)$/.exec(String(s).trim());
  if (!m) return null;
  const n = +m[1];
  return m[2] === "ms" ? n : m[2] === "s" ? n * 1000 : n * 60000;
}

/* ---------------------------------------------------------- text helpers */
function wrapLines(mctx, str, css, maxW) {
  mctx.font = css;
  const out = [];
  for (const para of String(str).split(/\n/)) {
    const words = para.split(/\s+/).filter(Boolean);
    let cur = "";
    for (const w of words) {
      const t = cur ? cur + " " + w : w;
      if (!cur || mctx.measureText(t).width <= maxW) cur = t;
      else {
        out.push(cur);
        cur = w;
      }
    }
    out.push(cur);
  }
  return out.length ? out : [""];
}
function ellipsize(mctx, str, css, maxW) {
  mctx.font = css;
  let s = String(str);
  if (mctx.measureText(s).width <= maxW) return s;
  while (s.length > 1 && mctx.measureText(s + "…").width > maxW)
    s = s.slice(0, -1);
  return s + "…";
}

/* ---------------------------------------------------------------- layout */
function layoutWidget(eng, node, box, inv) {
  const L = {
    text: layoutText,
    row: layoutRow,
    col: layoutCol,
    divider: layoutDivider,
    progress: layoutProgress,
    list: layoutList,
    book: layoutBook,
  }[node.t];
  if (!L) return { h: 0, ops: [], hits: [] };
  return L(eng, node, box, inv);
}

function layoutText(eng, node, box, inv) {
  const p = node.p;
  const f = fontFor(p);
  const val = String(resv(p.value) ?? "");
  const g = clampG(p.gray || 0, inv || p.invert);
  const pad = p.padAll || 0;
  const w = box.w - pad * 2;
  const lines = p.ellipsis
    ? [ellipsize(eng.m, val, f.css, w)]
    : wrapLines(eng.m, val, f.css, w);
  const lineH = Math.round(f.px * 1.35);
  const ops = lines.map((ln, i) => {
    let tx = box.x + pad;
    if (p.align === "center") {
      eng.m.font = f.css;
      tx = box.x + Math.max(pad, (box.w - eng.m.measureText(ln).width) / 2);
    }
    return {
      op: "text",
      x: Math.round(tx),
      y: Math.round(box.y + pad + i * lineH + f.px * 0.8),
      str: ln,
      font: f.css,
      g,
    };
  });
  return { h: pad * 2 + lines.length * lineH, ops, hits: [] };
}

function batteryOps(x, y, lvl, g) {
  return [
    { op: "srect", x, y, w: 38, h: 20, g, lw: 2 },
    { op: "rect", x: x + 40, y: y + 5, w: 5, h: 10, g },
    {
      op: "rect",
      x: x + 4,
      y: y + 4,
      w: Math.round(30 * Math.max(0, Math.min(1, lvl))),
      h: 12,
      g,
    },
  ];
}

function layoutRow(eng, node, box, inv) {
  const p = node.p;
  const pad = p.padAll || 0;
  const gap = p.gap ?? 10;
  const inv2 = p.invert ? true : inv;
  const inner = box.w - pad * 2;

  const kids = node.ch.map((k) => {
    if (k.t === "text") {
      const f = fontFor(k.p);
      eng.m.font = f.css;
      const s = String(resv(k.p.value) ?? "");
      return { k, w: eng.m.measureText(s).width, h: Math.round(f.px * 1.35), s, f };
    }
    if (k.t === "icon") return { k, w: 46, h: 20 };
    if (k.t === "spacer") return { k, w: 0, h: 0, flex: true };
    return { k, w: 0, h: 0 };
  });

  const nFlex = kids.filter((c) => c.flex).length;
  const fixed =
    kids.reduce((a, c) => a + (c.flex ? 0 : c.w), 0) +
    gap * Math.max(0, kids.length - 1);
  const flexW = nFlex ? Math.max(0, (inner - fixed) / nFlex) : 0;
  const maxH = kids.reduce((a, c) => Math.max(a, c.h), 0);
  const rowH = p.height || maxH + pad * 2;

  const ops = [];
  if (p.invert)
    ops.push({ op: "rect", x: box.x, y: box.y, w: box.w, h: rowH, g: clampG(0, inv) });

  let cx = box.x + pad;
  for (const c of kids) {
    const cw = c.flex ? flexW : c.w;
    const cy = box.y + (rowH - c.h) / 2;
    if (c.k.t === "text") {
      ops.push({
        op: "text",
        x: Math.round(cx),
        y: Math.round(cy + c.f.px * 0.8),
        str: c.s,
        font: c.f.css,
        g: clampG(c.k.p.gray || 0, inv2),
      });
    } else if (c.k.t === "icon") {
      const lvl = c.k.p.bind ? resv(c.k.p.bind) : 1;
      ops.push(...batteryOps(Math.round(cx), Math.round(cy), lvl, clampG(0, inv2)));
    }
    cx += cw + gap;
  }
  return { h: rowH, ops, hits: [] };
}

function layoutCol(eng, node, box, inv) {
  const p = node.p;
  const pad = p.padAll || 0;
  const gap = p.gap ?? 0;
  const x = box.x + pad;
  const w = box.w - pad * 2;
  let y = box.y + (p.padTop ?? pad);
  const ops = [];
  const hits = [];
  for (const k of node.ch) {
    const r = layoutWidget(eng, k, { x, y, w, h: box.y + box.h - y }, inv);
    ops.push(...r.ops);
    hits.push(...r.hits);
    y += r.h + gap;
  }
  return { h: y - box.y - (node.ch.length ? gap : 0) + pad, ops, hits };
}

function layoutDivider(eng, node, box, inv) {
  return {
    h: 2,
    ops: [{ op: "dots", x: box.x, y: box.y, w: box.w, g: clampG(2, inv) }],
    hits: [],
  };
}

function layoutProgress(eng, node, box, inv) {
  const h = node.p.height || 6;
  const v = Math.max(0, Math.min(1, resv(node.p.value) || 0));
  return {
    h,
    ops: [
      { op: "rect", x: box.x, y: box.y, w: box.w, h, g: clampG(2, inv) },
      { op: "rect", x: box.x, y: box.y, w: Math.round(box.w * v), h, g: clampG(0, inv) },
    ],
    hits: [],
  };
}

function layoutList(eng, node, box, inv) {
  const p = node.p;
  let items = p.items || [];
  if (p.sort) items = [...items].sort(p.sort);
  const ops = [];
  const hits = [];
  let y = box.y;
  for (let i = 0; i < items.length; i++) {
    const built = p.item ? p.item(items[i]) : null;
    if (!built) break;
    const r = layoutWidget(eng, built.__node, { x: box.x, y, w: box.w, h: box.y + box.h - y }, inv);
    if (y + r.h > box.y + box.h) break; // paginate("fit"): never clip an item
    ops.push(...r.ops);
    const it = items[i];
    if (p.onSelect)
      hits.push({ x: box.x, y, w: box.w, h: r.h, fn: () => p.onSelect(it) });
    y += r.h;
    if (p.separator && i < items.length - 1) {
      const s = layoutWidget(eng, p.separator, { x: box.x + 20, y, w: box.w - 40, h: 2 }, inv);
      ops.push(...s.ops);
      y += s.h;
    }
  }
  return { h: y - box.y, ops, hits };
}

function layoutBook(eng, node, box, inv) {
  const p = node.p;
  const st = (node.st = node.st || { page: 0 });
  const px = Math.round(p.pt * 3); // ~234 ppi panel: 1pt ≈ 3px
  const lineH = Math.round(px * 1.55);
  const mx = p.mx,
    my = p.my;
  const css = `400 ${px}px ${SERIF}`;
  const key = [box.w, box.h, px, mx, my].join("|");

  if (!node.cache || node.cache.key !== key) {
    const maxW = box.w - mx * 2;
    const indent = Math.round(px * 1.4);
    const lines = [];
    const paras = String(p.content).split(/\n\s*\n/);
    eng.m.font = css;
    for (const para of paras) {
      // first line wraps against the indented width, the rest against full width
      const words = para.replace(/\s+/g, " ").trim().split(" ").filter(Boolean);
      let cur = "",
        first = true;
      for (const w of words) {
        const t = cur ? cur + " " + w : w;
        const lim = first ? maxW - indent : maxW;
        if (!cur || eng.m.measureText(t).width <= lim) cur = t;
        else {
          lines.push({ t: cur, ind: first ? indent : 0 });
          first = false;
          cur = w;
        }
      }
      if (cur) lines.push({ t: cur, ind: first ? indent : 0 });
    }
    const perPage = Math.max(1, Math.floor((box.h - my * 2 - 44) / lineH));
    const pages = [];
    for (let i = 0; i < lines.length; i += perPage)
      pages.push(lines.slice(i, i + perPage));
    node.cache = { key, pages };
  }

  const pages = node.cache.pages;
  st.page = Math.max(0, Math.min(st.page, pages.length - 1));
  const g = clampG(0, inv);
  const ops = [];
  (pages[st.page] || []).forEach((ln, i) => {
    ops.push({
      op: "text",
      x: box.x + mx + ln.ind,
      y: Math.round(box.y + my + i * lineH + px * 0.8),
      str: ln.t,
      font: css,
      g,
    });
  });
  const folio = `${st.page + 1} / ${pages.length}`;
  eng.m.font = `400 ${SIZES.xs}px ${SANS}`;
  ops.push({
    op: "text",
    x: Math.round(box.x + (box.w - eng.m.measureText(folio).width) / 2),
    y: box.y + box.h - 18,
    str: folio,
    font: `400 ${SIZES.xs}px ${SANS}`,
    g: clampG(1, inv),
  });
  return { h: box.h, ops, hits: [] };
}

function layoutRegion(eng, node) {
  const wgt = node.p.widget;
  const f = fontFor(wgt.p);
  const s = String(resv(wgt.p.value) ?? "");
  eng.m.font = f.css;
  const tw = eng.m.measureText(s).width;
  const padX = 12,
    padY = 7;
  const w = Math.ceil(tw + padX * 2);
  const h = Math.round(f.px * 1.3 + padY * 2);
  const at = node.p.at || {};
  const x =
    at.left != null ? at.left : DW - (at.right != null ? at.right : 0) - w;
  const y =
    at.top != null ? at.top : DH - (at.bottom != null ? at.bottom : 0) - h;
  return [
    { op: "rect", x, y, w, h, g: 3 },
    { op: "srect", x, y, w, h, g: 2, lw: 2 },
    {
      op: "text",
      x: Math.round(x + padX),
      y: Math.round(y + padY + f.px * 0.8),
      str: s,
      font: f.css,
      g: clampG(wgt.p.gray || 0, false),
    },
  ];
}

function layoutPage(eng, n) {
  const p = n.p;
  const ops = [{ op: "rect", x: 0, y: 0, w: DW, h: DH, g: 3 }];
  const hits = [];
  let y0 = 0;
  if (p.header) {
    const r = layoutWidget(eng, p.header, { x: 0, y: 0, w: DW, h: DH }, false);
    ops.push(...r.ops);
    y0 = r.h;
  }
  let footH = 0,
    footOps = [];
  if (p.footer) {
    const r = layoutWidget(eng, p.footer, { x: 0, y: 0, w: DW, h: DH }, false);
    footH = r.h;
    footOps = offsetOps(r.ops, 0, DH - footH);
  }
  if (p.content) {
    const r = layoutWidget(
      eng,
      p.content,
      { x: 0, y: y0, w: DW, h: DH - y0 - footH },
      false
    );
    ops.push(...r.ops);
    hits.push(...r.hits);
  }
  ops.push(...footOps);
  for (const reg of p.overlays) ops.push(...layoutRegion(eng, reg));
  return { ops, hits };
}

/* --------------------------------------------------------------- backend */
function paintOps(ctx, ops, invert) {
  ctx.textBaseline = "alphabetic";
  for (const o of ops) {
    const c = GRAY[Math.max(0, Math.min(3, invert ? 3 - o.g : o.g))];
    if (o.op === "rect") {
      ctx.fillStyle = c;
      ctx.fillRect(o.x, o.y, o.w, o.h);
    } else if (o.op === "srect") {
      ctx.strokeStyle = c;
      ctx.lineWidth = o.lw || 2;
      ctx.strokeRect(o.x + 1, o.y + 1, o.w - 2, o.h - 2);
    } else if (o.op === "text") {
      ctx.fillStyle = c;
      ctx.font = o.font;
      ctx.fillText(o.str, o.x, o.y);
    } else if (o.op === "dots") {
      ctx.fillStyle = c;
      for (let x = o.x; x < o.x + o.w; x += 9) ctx.fillRect(x, o.y, 3, 2);
    }
  }
}

/* ---------------------------------------------------------------- engine */
const defaultPolicy = (ctx) => (ctx.turnsSinceFull >= 6 ? "full" : "partial");

function createEngine(canvas, hooks) {
  const ctx = canvas.getContext("2d");
  const ghost = document.createElement("canvas");
  ghost.width = DW;
  ghost.height = DH;
  const gctx = ghost.getContext("2d");
  const mCan = document.createElement("canvas");

  const eng = {
    m: mCan.getContext("2d"),
    pages: {},
    current: null,
    navStack: [],
    policy: null,
    hits: [],
    handlers: [],
    contentBuilder: null,
    turns: 0,
    flashing: false,
    disposed: false,
    lastPointerTs: 0,
    lastRenderAt: 0,
    intervals: [],
    timeouts: [],
  };

  const status = (kind) =>
    hooks.onStatus &&
    hooks.onStatus({
      kind,
      turns: eng.turns,
      ghost: Math.min(1, eng.turns * 0.06),
    });

  function doPartial(ops) {
    gctx.clearRect(0, 0, DW, DH);
    gctx.drawImage(canvas, 0, 0);
    paintOps(ctx, ops, false);
    const a = Math.min(0.045 * eng.turns, 0.22);
    if (a > 0.01) {
      ctx.save();
      ctx.globalAlpha = a;
      ctx.globalCompositeOperation = "multiply";
      ctx.drawImage(ghost, 0, 0);
      ctx.restore();
    }
    eng.turns++;
    status("partial");
  }

  function solid(g) {
    ctx.fillStyle = GRAY[g];
    ctx.fillRect(0, 0, DW, DH);
  }

  function doFull(ops) {
    eng.flashing = true;
    const T = (fn, ms) => eng.timeouts.push(setTimeout(fn, ms));
    paintOps(ctx, ops, true);
    T(() => solid(0), 110);
    T(() => solid(3), 220);
    T(() => {
      paintOps(ctx, ops, false);
      eng.turns = 0;
      eng.flashing = false;
      status("full");
    }, 330);
  }

  eng.requestRender = (kind) => {
    if (eng.disposed || eng.flashing) return;
    const pb = eng.pages[eng.current];
    if (!pb) return;
    let laid;
    try {
      laid = layoutPage(eng, pb.__node);
    } catch (e) {
      hooks.onError && hooks.onError(e);
      return;
    }
    eng.hits = laid.hits;
    eng.handlers = pb.__node.p.handlers;
    eng.contentBuilder = pb.__node.p.contentBuilder;
    eng.lastRenderAt = performance.now();
    let k = kind;
    if (k === "auto")
      k = (eng.policy || defaultPolicy)({
        turnsSinceFull: eng.turns,
        ghostingScore: Math.min(1, eng.turns * 0.06),
      });
    if (k === "full") doFull(laid.ops);
    else doPartial(laid.ops);
  };

  eng.mountPage = (name, kind) => {
    if (!eng.pages[name]) return;
    eng.current = name;
    eng.intervals.forEach(clearInterval);
    eng.intervals = [];
    for (const reg of eng.pages[name].__node.p.overlays) {
      const ms = reg.p.everyMs;
      if (!ms) continue;
      eng.intervals.push(
        setInterval(() => {
          if (eng.disposed) return;
          if (reg.p.quiet && Date.now() - eng.lastPointerTs < 2500) return;
          eng.requestRender("partial");
        }, ms)
      );
    }
    eng.requestRender(kind);
  };

  eng.dispatch = (type, x, y) => {
    eng.lastPointerTs = Date.now();
    const t0 = performance.now();
    let acted = false;
    if (type === "tap") {
      for (const h of eng.hits) {
        if (x >= h.x && x <= h.x + h.w && y >= h.y && y <= h.y + h.h) {
          h.fn();
          acted = true;
          break;
        }
      }
      if (!acted) {
        const zone = x < DW / 3 ? "tapLeft" : x > (2 * DW) / 3 ? "tapRight" : "tap";
        let hs = eng.handlers.filter((h) => h.g === zone);
        if (!hs.length && zone !== "tap")
          hs = eng.handlers.filter((h) => h.g === "tap");
        for (const h of hs) {
          h.fn(eng.contentBuilder);
          acted = true;
        }
      }
    } else if (type === "swipeDown") {
      for (const h of eng.handlers.filter((h) => h.g === "swipeDown")) {
        h.fn(eng.contentBuilder);
        acted = true;
      }
    }
    // if a handler ran but didn't itself trigger a render, apply the policy
    if (acted && eng.lastRenderAt < t0) eng.requestRender("auto");
  };

  eng.dispose = () => {
    eng.disposed = true;
    eng.intervals.forEach(clearInterval);
    eng.timeouts.forEach(clearTimeout);
  };

  return eng;
}

/* ------------------------------------------------------- builders (env) */
function makeEnv(eng) {
  const text = (v) => {
    const n = {
      t: "text",
      p: { value: v, size: "md", weight: "normal", align: "left", gray: 0, serif: false, ellipsis: false, padAll: 0, invert: false },
      ch: [],
    };
    const b = { __node: n };
    b.size = (s) => ((n.p.size = s), b);
    b.center = () => ((n.p.align = "center"), b);
    b.gray = (g) => ((n.p.gray = g), b);
    b.weight = (w) => ((n.p.weight = w), b);
    b.serif = () => ((n.p.serif = true), b);
    b.ellipsis = () => ((n.p.ellipsis = true), b);
    b.pad = (v2) => ((n.p.padAll = v2), b);
    b.invert = () => ((n.p.invert = true), b);
    return b;
  };

  const icon = (name) => {
    const n = { t: "icon", p: { name, bind: null }, ch: [] };
    const b = { __node: n, bind: (fn) => ((n.p.bind = fn), b) };
    return b;
  };

  const spacer = () => ({ __node: { t: "spacer", p: {}, ch: [] } });

  const kids = (arr) => arr.map((k) => k.__node);

  const row = (...cs) => {
    const n = { t: "row", p: { padAll: 0, gap: 10, height: 0, invert: false }, ch: kids(cs) };
    const b = { __node: n };
    b.pad = (v) => ((n.p.padAll = v), b);
    b.gap = (v) => ((n.p.gap = v), b);
    b.height = (v) => ((n.p.height = v), b);
    b.invert = () => ((n.p.invert = true), b);
    return b;
  };

  const col = (...cs) => {
    const n = { t: "col", p: { padAll: 0, padTop: null, gap: 0 }, ch: kids(cs) };
    const b = { __node: n };
    b.pad = (v) => ((n.p.padAll = v), b);
    b.padTop = (v) => ((n.p.padTop = v), b);
    b.gap = (v) => ((n.p.gap = v), b);
    return b;
  };

  const divider = () => {
    const n = { t: "divider", p: { dotted: true }, ch: [] };
    const b = { __node: n, dotted: () => b, solid: () => b };
    return b;
  };

  const progress = (v) => {
    const n = { t: "progress", p: { value: v, height: 6 }, ch: [] };
    const b = { __node: n, height: (h) => ((n.p.height = h), b) };
    return b;
  };

  const list = (items) => {
    const n = { t: "list", p: { items, item: null, separator: null, sort: null, onSelect: null }, ch: [] };
    const b = { __node: n };
    b.item = (fn) => ((n.p.item = fn), b);
    b.separator = (w) => ((n.p.separator = w.__node), b);
    b.sort = (fn) => ((n.p.sort = fn), b);
    b.onSelect = (fn) => ((n.p.onSelect = fn), b);
    b.paginate = () => b; // "fit" is the only mode — an opinion, not an option
    return b;
  };

  const book = (content) => {
    const n = { t: "book", p: { content, pt: 11, mx: 40, my: 36 }, ch: [] };
    const b = { __node: n };
    b.font = (_name, pt) => ((n.p.pt = pt || 11), b);
    b.margins = (m) => ((n.p.mx = m.x ?? n.p.mx), (n.p.my = m.y ?? n.p.my), b);
    b.hyphenate = () => b;
    b.nextPage = () => {
      if (n.cache && n.st) n.st.page = Math.min(n.st.page + 1, n.cache.pages.length - 1);
      eng.requestRender("auto");
    };
    b.prevPage = () => {
      if (n.st) n.st.page = Math.max(n.st.page - 1, 0);
      eng.requestRender("auto");
    };
    return b;
  };

  const region = (w) => {
    const n = { t: "region", p: { widget: w.__node, at: {}, everyMs: null, quiet: false }, ch: [] };
    const b = { __node: n };
    b.at = (a) => ((n.p.at = a), b);
    b.every = (s) => ((n.p.everyMs = parseEvery(s)), b);
    b.quiet = () => ((n.p.quiet = true), b);
    return b;
  };

  const page = (name) => {
    const n = {
      t: "page",
      p: { name, header: null, footer: null, content: null, contentBuilder: null, overlays: [], handlers: [] },
      ch: [],
    };
    const b = { __node: n };
    b.header = (w) => ((n.p.header = w.__node), b);
    b.footer = (w) => ((n.p.footer = w.__node), b);
    b.content = (w) => ((n.p.content = w.__node), (n.p.contentBuilder = w), b);
    b.overlay = (r) => (n.p.overlays.push(r.__node), b);
    b.on = (g, fn) => (n.p.handlers.push({ g, fn }), b);
    b.render = () => mount({}, b);
    eng.pages[name] = b;
    return b;
  };

  function mount(cfg, pb) {
    if (cfg && cfg.policy) eng.policy = cfg.policy;
    if (!pb) return;
    if (pb.__node.t !== "page") {
      const wrapped = page("__main").content(pb);
      pb = wrapped;
    }
    eng.navStack = [];
    eng.mountPage(pb.__node.p.name, "full");
  }

  const paper = () => {
    const cfg = { policy: null };
    let pg = null;
    const b = {
      rotation: () => b,
      grayscale: () => b,
      refreshPolicy: (fn) => ((cfg.policy = fn), b),
      page: (p) => ((pg = p), b),
      render: () => mount(cfg, pg),
    };
    return b;
  };

  const nav = {
    push: (target) => {
      const name = typeof target === "string" ? target : target && target.__node ? target.__node.p.name : null;
      if (!name || !eng.pages[name]) return;
      eng.navStack.push(eng.current);
      eng.mountPage(name, "full");
    },
    back: () => {
      const prev = eng.navStack.pop();
      if (prev) eng.mountPage(prev, "full");
    },
  };

  const gesture = { tap: "tap", tapLeft: "tapLeft", tapRight: "tapRight", swipeDown: "swipeDown" };
  const sys = { battery: () => 0.63 };
  const clock = {
    format: (fmt) => {
      const d = new Date();
      const p2 = (x) => String(x).padStart(2, "0");
      return fmt === "HH:mm:ss"
        ? `${p2(d.getHours())}:${p2(d.getMinutes())}:${p2(d.getSeconds())}`
        : `${p2(d.getHours())}:${p2(d.getMinutes())}`;
    },
  };

  return { paper, page, text, row, col, spacer, divider, progress, icon, list, book, region, gesture, nav, sys, clock };
}

/* ================================================================ presets */
const PRESETS = [
  {
    key: "hello",
    name: "01 · Hello, ink",
    hint: "Full refresh on mount — watch the flash.",
    code: `// The smallest s3paper program.
// paper() owns the panel; .page() accepts any widget
// and wraps it into an anonymous page.

paper()
  .page(
    col(
      text("Hello, ink.").size("xl").center(),
      text("540 x 960 - grayscale-4 - full refresh")
        .size("xs").gray(1).center(),
    ).gap(20).padTop(430)
  )
  .render();
`,
  },
  {
    key: "status",
    name: "02 · Status bar + live region",
    hint: "The seconds clock is a partial-refresh region — every 5 s, no flash. Tap to accumulate ghosting.",
    code: `// Lambdas mark widgets as dynamic. The overlay region
// re-renders itself every 5 s without a flash, and
// .quiet() defers ticks while you're interacting.

const statusBar = row(
  icon("battery").bind(() => sys.battery()),
  spacer(),
  text(() => clock.format("HH:mm")).size("sm"),
).height(48).pad(16).invert();

paper().page(
  page("home")
    .header(statusBar)
    .content(
      col(
        text("Quiet by design").size("lg"),
        text("The seconds clock below lives in a region() - the only part of the panel that updates. Partial refresh, zero flash.")
          .size("sm").gray(1),
        divider().dotted(),
        text("Tap the page a few times: each tap is a partial refresh, and ghosting accumulates until the refresh policy schedules a full flash.")
          .size("sm").gray(1),
      ).pad(30).gap(24).padTop(110)
    )
    .overlay(
      region(text(() => clock.format("HH:mm:ss")).size("sm"))
        .at({ bottom: 18, right: 18 })
        .every("5s")
        .quiet()
    )
    .on(gesture.tap, () => {})
).render();
`,
  },
  {
    key: "library",
    name: "03 · Library (list + nav)",
    hint: "Tap a book to open its detail page. Tap anywhere on the detail page to go back.",
    code: `// list() paginates to fit - scrolling doesn't exist
// on e-ink, so the API doesn't offer it. Each item is
// composed from primitives via a lambda.

const shelf = [
  { title: "The Left Hand of Darkness", author: "Ursula K. Le Guin", pct: 0.62, opened: 5 },
  { title: "Snow Country",              author: "Yasunari Kawabata",  pct: 0.18, opened: 9 },
  { title: "The Overstory",             author: "Richard Powers",     pct: 0.87, opened: 2 },
  { title: "Invisible Cities",          author: "Italo Calvino",      pct: 0.31, opened: 7 },
  { title: "Pachinko",                  author: "Min Jin Lee",        pct: 0.05, opened: 1 },
];

paper().page(
  page("library")
    .header(row(text("Library").size("lg").weight("bold")).pad(24))
    .content(
      list(shelf)
        .item(b =>
          col(
            text(b.title).weight("bold").ellipsis(),
            text(b.author).size("sm").gray(1),
            progress(b.pct).height(5),
          ).pad(22).gap(12)
        )
        .separator(divider().dotted())
        .sort((a, z) => a.opened - z.opened)   // most recent first
        .onSelect(b =>
          nav.push(
            page("detail")
              .content(
                col(
                  text(b.title).size("lg").weight("bold"),
                  text("by " + b.author).size("sm").gray(1),
                  divider().dotted(),
                  progress(b.pct).height(8),
                  text(Math.round(b.pct * 100) + "% read").size("xs").gray(1),
                  text("Tap anywhere to go back.").size("xs").gray(2),
                ).pad(30).gap(22).padTop(140)
              )
              .on(gesture.tap, () => nav.back())
          )
        )
        .paginate("fit")
    )
    .footer(
      row(
        text(() => clock.format("HH:mm")).size("xs").gray(1),
        spacer(),
        text("5 books").size("xs").gray(1),
      ).height(44).pad(18)
    )
).render();
`,
  },
  {
    key: "reader",
    name: "04 · Reader (refresh policy)",
    hint: "Tap the right / left third of the page to turn. Every 7th turn the policy injects a full flash.",
    code: `// book() owns layout + pagination. Page turns are
// partial by default; the refreshPolicy lambda decides
// when ghosting has earned a full flash.

const CHAPTER = [
  "The keeper climbed the hundred and six steps twice a day, once at dusk to light the lamp and once at dawn to trim it, and in between he read. The tower held more books than tools, which said something about the trade, or about him.",
  "Reading by lamplight above the sea was its own weather. On calm nights the sentences went down easily, page after page, and the only sound was the slow clockwork of the lens turning its beam across the water.",
  "In storms it was different. The tower hummed like a struck bell and the print seemed to shiver on the page, so he read the same paragraph three times and gave up and watched the black glass of the window instead.",
  "He had a theory that a page should behave like the sea between waves: perfectly still while you looked at it, changing only in the instant you asked it to change. Flicker was for lesser instruments.",
  "The supply boat brought paper, oil, and one crate of novels a season. He rationed them badly, always. By March he was rereading, which he claimed was the truest kind of reading anyway.",
  "Years later, when they automated the light, he kept the habit: a chapter at dusk, a chapter at dawn. Some machinery, he said, you keep running because it keeps you running.",
].join("\\n\\n");

const novel = book(CHAPTER)
  .font("Bookerly", 11)
  .margins({ x: 46, y: 44 });

paper()
  .refreshPolicy(ctx =>
    ctx.turnsSinceFull > 6 || ctx.ghostingScore > 0.4
      ? "full"
      : "partial"
  )
  .page(
    page("reader")
      .content(novel)
      .on(gesture.tapRight, b => b.nextPage())
      .on(gesture.tapLeft,  b => b.prevPage())
  )
  .render();
`,
  },
];

/* ============================================================== component */
const CSS = `
.s3s{--bg:#191a17;--panel:#1f201c;--edge:#30312b;--txt:#c9c8bd;--dim:#84837a;
  --paper:#e8e6db;--ink:#181914;
  height:100%;min-height:560px;display:flex;flex-direction:column;
  background:var(--bg);color:var(--txt);
  font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;}
.s3s *{box-sizing:border-box}
.s3s-hdr{display:flex;align-items:center;gap:14px;padding:10px 16px;
  border-bottom:1px solid var(--edge);flex-wrap:wrap}
.s3s-brand{font-size:13px;letter-spacing:.14em;color:var(--paper);white-space:nowrap}
.s3s-brand b{font-weight:700}
.s3s-pipe{font-size:10px;letter-spacing:.08em;color:var(--dim);white-space:nowrap}
.s3s-hdr select{margin-left:auto;background:var(--panel);color:var(--txt);
  border:1px solid var(--edge);border-radius:6px;padding:7px 10px;font:inherit;
  font-size:12px;max-width:46vw}
.s3s-btn{background:var(--panel);color:var(--txt);border:1px solid var(--edge);
  border-radius:6px;padding:7px 14px;font:inherit;font-size:12px;cursor:pointer;
  letter-spacing:.06em}
.s3s-btn:hover{border-color:var(--dim)}
.s3s-btn:focus-visible{outline:2px solid var(--paper);outline-offset:1px}
.s3s-btn.run{background:var(--paper);color:var(--ink);border-color:var(--paper);font-weight:700}
.s3s-main{flex:1;display:flex;min-height:0}
.s3s-edit{flex:1;min-width:0;display:flex;flex-direction:column;
  border-right:1px solid var(--edge)}
.s3s-edit textarea{flex:1;width:100%;resize:none;border:0;outline:none;
  background:#151613;color:#d6d5ca;padding:16px;font:inherit;font-size:12.5px;
  line-height:1.6;tab-size:2;white-space:pre;overflow:auto}
.s3s-err{padding:8px 16px;font-size:11.5px;color:#d89a86;background:#241c18;
  border-top:1px solid #3a2a23}
.s3s-hint{padding:8px 16px;font-size:11px;color:var(--dim);
  border-top:1px solid var(--edge);line-height:1.5}
.s3s-dev{width:400px;flex-shrink:0;display:flex;flex-direction:column;
  align-items:center;gap:12px;padding:22px 18px;overflow:auto}
.s3s-bezel{background:#232420;border:1px solid #35362f;border-radius:20px;
  padding:14px 14px 26px;box-shadow:inset 0 1px 0 rgba(255,255,255,.05),
  0 14px 40px rgba(0,0,0,.45);width:100%;max-width:330px}
.s3s-bezel canvas{display:block;width:100%;height:auto;background:var(--paper);
  border-radius:3px;touch-action:none;cursor:pointer}
.s3s-read{display:flex;gap:16px;font-size:10px;letter-spacing:.12em;
  color:var(--dim);text-transform:uppercase;flex-wrap:wrap;justify-content:center}
.s3s-read b{color:var(--txt);font-weight:400}
.s3s-read .full{color:var(--paper)}
.s3s-devbtns{display:flex;gap:10px}
.s3s-tabs{display:none}
@media (max-width:840px){
  .s3s-main{flex-direction:column}
  .s3s-tabs{display:flex;border-bottom:1px solid var(--edge)}
  .s3s-tab{flex:1;padding:10px;font:inherit;font-size:12px;letter-spacing:.1em;
    background:none;border:0;color:var(--dim);cursor:pointer;
    border-bottom:2px solid transparent;text-transform:uppercase}
  .s3s-tab.on{color:var(--paper);border-bottom-color:var(--paper)}
  .s3s-edit{border-right:0;display:none}
  .s3s-dev{width:100%;display:none;padding:18px 14px}
  .s3s.tab-code .s3s-edit{display:flex}
  .s3s.tab-device .s3s-dev{display:flex}
  .s3s-bezel{max-width:340px}
  .s3s-hdr select{max-width:none;flex:1}
}
@media (prefers-reduced-motion:reduce){.s3s *{transition:none!important}}
`;

export default function S3PaperStudio() {
  const canvasRef = useRef(null);
  const engineRef = useRef(null);
  const pointerRef = useRef(null);
  const [presetKey, setPresetKey] = useState(PRESETS[0].key);
  const [code, setCode] = useState(PRESETS[0].code);
  const [error, setError] = useState(null);
  const [status, setStatus] = useState({ kind: "—", turns: 0, ghost: 0 });
  const [tab, setTab] = useState("device");

  const preset = PRESETS.find((p) => p.key === presetKey);

  const run = useCallback((src) => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    if (engineRef.current) engineRef.current.dispose();
    setError(null);
    setStatus({ kind: "—", turns: 0, ghost: 0 });
    const eng = createEngine(canvas, {
      onStatus: (s) => setStatus(s),
      onError: (e) => setError(String((e && e.message) || e)),
    });
    engineRef.current = eng;
    const env = makeEnv(eng);
    try {
      const fn = new Function(...Object.keys(env), '"use strict";\n' + src);
      fn(...Object.values(env));
      if (!eng.current)
        setError("Nothing rendered — finish with paper().page(...).render()");
    } catch (e) {
      setError(String((e && e.message) || e));
    }
  }, []);

  useEffect(() => {
    run(PRESETS[0].code);
    return () => engineRef.current && engineRef.current.dispose();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const onPreset = (key) => {
    const p = PRESETS.find((x) => x.key === key);
    setPresetKey(key);
    setCode(p.code);
    run(p.code);
  };

  const deviceXY = (e) => {
    const r = canvasRef.current.getBoundingClientRect();
    return {
      x: ((e.clientX - r.left) * DW) / r.width,
      y: ((e.clientY - r.top) * DH) / r.height,
    };
  };
  const onDown = (e) => {
    pointerRef.current = deviceXY(e);
    if (engineRef.current) engineRef.current.lastPointerTs = Date.now();
  };
  const onUp = (e) => {
    const start = pointerRef.current;
    pointerRef.current = null;
    if (!start || !engineRef.current) return;
    const end = deviceXY(e);
    if (end.y - start.y > 120) engineRef.current.dispatch("swipeDown", end.x, end.y);
    else engineRef.current.dispatch("tap", end.x, end.y);
  };

  const onKey = (e) => {
    if (e.key === "Tab") {
      e.preventDefault();
      const t = e.target;
      const s = t.selectionStart;
      const next = code.slice(0, s) + "  " + code.slice(t.selectionEnd);
      setCode(next);
      requestAnimationFrame(() => t.setSelectionRange(s + 2, s + 2));
    } else if ((e.metaKey || e.ctrlKey) && e.key === "Enter") {
      e.preventDefault();
      run(code);
    }
  };

  return (
    <div className={`s3s tab-${tab}`}>
      <style>{CSS}</style>

      <header className="s3s-hdr">
        <span className="s3s-brand">
          <b>s3paper</b> studio
        </span>
        <span className="s3s-pipe">build → layout → draw ops → panel</span>
        <select
          aria-label="Preset"
          value={presetKey}
          onChange={(e) => onPreset(e.target.value)}
        >
          {PRESETS.map((p) => (
            <option key={p.key} value={p.key}>
              {p.name}
            </option>
          ))}
        </select>
        <button className="s3s-btn run" onClick={() => run(code)}>
          Run ⌘↵
        </button>
      </header>

      <div className="s3s-tabs">
        <button
          className={`s3s-tab ${tab === "code" ? "on" : ""}`}
          onClick={() => setTab("code")}
        >
          Code
        </button>
        <button
          className={`s3s-tab ${tab === "device" ? "on" : ""}`}
          onClick={() => setTab("device")}
        >
          Device
        </button>
      </div>

      <div className="s3s-main">
        <section className="s3s-edit">
          <textarea
            spellCheck={false}
            value={code}
            onChange={(e) => setCode(e.target.value)}
            onKeyDown={onKey}
            aria-label="s3paper source"
          />
          {error ? (
            <div className="s3s-err">✕ {error}</div>
          ) : (
            <div className="s3s-hint">{preset ? preset.hint : ""}</div>
          )}
        </section>

        <aside className="s3s-dev">
          <div className="s3s-bezel">
            <canvas
              ref={canvasRef}
              width={DW}
              height={DH}
              onPointerDown={onDown}
              onPointerUp={onUp}
            />
          </div>
          <div className="s3s-read">
            <span>
              last <b className={status.kind === "full" ? "full" : ""}>{status.kind}</b>
            </span>
            <span>
              since full <b>{status.turns}</b>
            </span>
            <span>
              ghost <b>{Math.round(status.ghost * 100)}%</b>
            </span>
            <span>
              panel <b>540×960 · g4</b>
            </span>
          </div>
          <div className="s3s-devbtns">
            <button
              className="s3s-btn"
              onClick={() =>
                engineRef.current && engineRef.current.requestRender("full")
              }
            >
              Full refresh
            </button>
          </div>
        </aside>
      </div>
    </div>
  );
}
