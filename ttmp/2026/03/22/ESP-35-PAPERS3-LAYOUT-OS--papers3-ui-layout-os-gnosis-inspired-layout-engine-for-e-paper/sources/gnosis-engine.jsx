import { useState, useRef, useEffect, useCallback } from "react";

// ── Bitmap font (simplified 5x7 pixel font) ──
const GLYPH_W = 6;
const GLYPH_H = 8;
const FONT_DATA = {};
const chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789:.-/%(){}[] >←+#";
// 5x7 bitmap font data - each char is array of 7 rows, each row is 5-bit number
const BITMAPS = {
  A:[ 0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001],
  B:[ 0b11110,0b10001,0b10001,0b11110,0b10001,0b10001,0b11110],
  C:[ 0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110],
  D:[ 0b11110,0b10001,0b10001,0b10001,0b10001,0b10001,0b11110],
  E:[ 0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111],
  F:[ 0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b10000],
  G:[ 0b01110,0b10001,0b10000,0b10111,0b10001,0b10001,0b01110],
  H:[ 0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001],
  I:[ 0b01110,0b00100,0b00100,0b00100,0b00100,0b00100,0b01110],
  J:[ 0b00111,0b00010,0b00010,0b00010,0b00010,0b10010,0b01100],
  K:[ 0b10001,0b10010,0b10100,0b11000,0b10100,0b10010,0b10001],
  L:[ 0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111],
  M:[ 0b10001,0b11011,0b10101,0b10101,0b10001,0b10001,0b10001],
  N:[ 0b10001,0b10001,0b11001,0b10101,0b10011,0b10001,0b10001],
  O:[ 0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110],
  P:[ 0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000],
  Q:[ 0b01110,0b10001,0b10001,0b10001,0b10101,0b10010,0b01101],
  R:[ 0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001],
  S:[ 0b01110,0b10001,0b10000,0b01110,0b00001,0b10001,0b01110],
  T:[ 0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100],
  U:[ 0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110],
  V:[ 0b10001,0b10001,0b10001,0b10001,0b10001,0b01010,0b00100],
  W:[ 0b10001,0b10001,0b10001,0b10101,0b10101,0b11011,0b10001],
  X:[ 0b10001,0b10001,0b01010,0b00100,0b01010,0b10001,0b10001],
  Y:[ 0b10001,0b10001,0b01010,0b00100,0b00100,0b00100,0b00100],
  Z:[ 0b11111,0b00001,0b00010,0b00100,0b01000,0b10000,0b11111],
  " ":[0,0,0,0,0,0,0],
  "0":[0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110],
  "1":[0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110],
  "2":[0b01110,0b10001,0b00001,0b00010,0b00100,0b01000,0b11111],
  "3":[0b01110,0b10001,0b00001,0b00110,0b00001,0b10001,0b01110],
  "4":[0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010],
  "5":[0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110],
  "6":[0b01110,0b10001,0b10000,0b11110,0b10001,0b10001,0b01110],
  "7":[0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000],
  "8":[0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110],
  "9":[0b01110,0b10001,0b10001,0b01111,0b00001,0b10001,0b01110],
  ":":[0b00000,0b00100,0b00100,0b00000,0b00100,0b00100,0b00000],
  ".":[0b00000,0b00000,0b00000,0b00000,0b00000,0b01100,0b01100],
  "-":[0b00000,0b00000,0b00000,0b11111,0b00000,0b00000,0b00000],
  "/":[0b00001,0b00010,0b00010,0b00100,0b01000,0b01000,0b10000],
  "%":[0b11001,0b11001,0b00010,0b00100,0b01000,0b10011,0b10011],
  "(":[0b00010,0b00100,0b01000,0b01000,0b01000,0b00100,0b00010],
  ")":[0b01000,0b00100,0b00010,0b00010,0b00010,0b00100,0b01000],
  ">":[0b01000,0b00100,0b00010,0b00001,0b00010,0b00100,0b01000],
  "+":[0b00000,0b00100,0b00100,0b11111,0b00100,0b00100,0b00000],
  "#":[0b01010,0b01010,0b11111,0b01010,0b11111,0b01010,0b01010],
  "_":[0b00000,0b00000,0b00000,0b00000,0b00000,0b00000,0b11111],
  "=":[0b00000,0b00000,0b11111,0b00000,0b11111,0b00000,0b00000],
  "!":[0b00100,0b00100,0b00100,0b00100,0b00100,0b00000,0b00100],
  ",":[0b00000,0b00000,0b00000,0b00000,0b00000,0b00100,0b01000],
  "'":[0b00100,0b00100,0b01000,0b00000,0b00000,0b00000,0b00000],
  '"':[0b01010,0b01010,0b10100,0b00000,0b00000,0b00000,0b00000],
};
// Add lowercase as copies of uppercase
"abcdefghijklmnopqrstuvwxyz".split("").forEach((c,i) => {
  BITMAPS[c] = BITMAPS[String.fromCharCode(65+i)];
});

// ── E-ink palette ──
const PAL = { bg: "#d8d4cc", fg: "#2a2a28", mid: "#9e9a92", light: "#c4c0b8", ghost: "#b8b4ac" };
const DEBUG_COLORS = ["#e6333380","#3366e680","#33aa3380","#cc880080","#aa33cc80","#33aabb80","#cc335580","#669e3380"];

// ── Layout engine ──
function layoutNode(node, x, y, w, h, results = [], depth = 0) {
  if (!node || typeof node !== "object") return results;
  const rect = { x, y, w, h, type: node.type || "unknown", id: node.id, depth, waveform: node.waveform || "part" };
  results.push(rect);
  node._rect = rect;

  const kids = node.items || node.children || [];
  if (node.type === "vbox" || node.layout === "vbox") {
    let fixedH = 0, flexCount = 0;
    kids.forEach(k => { if (k.h) fixedH += k.h; else flexCount++; });
    const flexH = flexCount > 0 ? Math.floor((h - fixedH) / flexCount) : 0;
    let cy = y;
    kids.forEach(k => {
      const ch = k.h || flexH;
      layoutNode(k, x, cy, w, ch, results, depth + 1);
      cy += ch;
    });
  } else if (node.type === "hbox" || node.layout === "hbox") {
    if (node.split) {
      const lw = node.split;
      const rw = w - lw - 1;
      if (kids[0]) layoutNode(kids[0], x, y, lw, h, results, depth + 1);
      if (kids[1]) layoutNode(kids[1], x + lw + 1, y, rw, h, results, depth + 1);
    } else {
      let fixedW = 0, flexCount = 0;
      kids.forEach(k => { if (k.w) fixedW += k.w; else if (k.spacer) flexCount++; else fixedW += (k.label || k.content || "").length * GLYPH_W + 4; });
      const flexW = flexCount > 0 ? Math.floor((w - fixedW) / flexCount) : 0;
      let cx = x;
      kids.forEach(k => {
        let cw;
        if (k.spacer) cw = flexW;
        else if (k.w) cw = k.w;
        else cw = (k.label || k.content || "").length * GLYPH_W + 4;
        layoutNode(k, cx, y, cw, h, results, depth + 1);
        cx += cw;
      });
    }
  } else if (node.type === "grid") {
    const cols = node.cols || 7;
    const cellW = node.cell_w || Math.floor(w / cols);
    const cellH = node.cell_h || 20;
    const count = node.count || cols * 5;
    for (let i = 0; i < count; i++) {
      const col = i % cols;
      const row = Math.floor(i / cols);
      const gx = x + col * cellW;
      const gy = y + row * cellH;
      results.push({ x: gx, y: gy, w: cellW, h: cellH, type: "grid_cell", depth: depth + 1, index: i });
    }
  } else if (node.type === "fixed") {
    kids.forEach(k => {
      const kx = x + (k.x || 0);
      const ky = y + (k.y || 0);
      const kw = k.w || w - (k.x || 0);
      const kh = k.h || h - (k.y || 0);
      layoutNode(k, kx, ky, kw, kh, results, depth + 1);
    });
  }
  return results;
}

// ── Canvas renderer ──
function renderToCanvas(ctx, nodes, dsl, debug) {
  const W = ctx.canvas.width;
  const H = ctx.canvas.height;

  // e-ink background
  ctx.fillStyle = PAL.bg;
  ctx.fillRect(0, 0, W, H);

  // grain
  const imgData = ctx.getImageData(0, 0, W, H);
  for (let i = 0; i < imgData.data.length; i += 4) {
    const n = (Math.random() - 0.5) * 8;
    imgData.data[i] += n;
    imgData.data[i + 1] += n;
    imgData.data[i + 2] += n;
  }
  ctx.putImageData(imgData, 0, 0);

  nodes.forEach((n, idx) => {
    // Debug bounding boxes
    if (debug.showBounds) {
      ctx.strokeStyle = DEBUG_COLORS[n.depth % DEBUG_COLORS.length];
      ctx.lineWidth = 1;
      ctx.strokeRect(n.x + 0.5, n.y + 0.5, n.w - 1, n.h - 1);
    }
    if (debug.showDepth) {
      ctx.fillStyle = DEBUG_COLORS[n.depth % DEBUG_COLORS.length];
      ctx.globalAlpha = 0.08;
      ctx.fillRect(n.x, n.y, n.w, n.h);
      ctx.globalAlpha = 1;
    }
  });

  // Render actual widgets by walking the DSL
  renderDSLNode(ctx, dsl, 0, 0, W, H, debug);

  // Debug: dirty rects
  if (debug.showDirty && debug.dirtyRects) {
    debug.dirtyRects.forEach(r => {
      ctx.strokeStyle = "#ff000099";
      ctx.lineWidth = 2;
      ctx.setLineDash([4, 4]);
      ctx.strokeRect(r.x, r.y, r.w, r.h);
      ctx.setLineDash([]);
      // waveform label
      ctx.fillStyle = "#ff0000cc";
      drawBitmapText(ctx, r.waveform || "PART", r.x + 2, r.y - 10, 1, "#ff0000cc");
    });
  }

  // Crosshair overlay
  if (debug.showCrosshair) {
    ctx.strokeStyle = PAL.mid + "30";
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(W / 2, 0); ctx.lineTo(W / 2, H);
    ctx.moveTo(0, H / 2); ctx.lineTo(W, H / 2);
    ctx.stroke();
  }
}

function drawBitmapText(ctx, text, x, y, size = 1, color = PAL.fg) {
  if (!text) return;
  const str = String(text);
  for (let i = 0; i < str.length; i++) {
    const ch = str[i];
    const bm = BITMAPS[ch] || BITMAPS[" "] || [0,0,0,0,0,0,0];
    const gx = x + i * GLYPH_W * size;
    for (let row = 0; row < 7; row++) {
      for (let col = 0; col < 5; col++) {
        if (bm[row] & (1 << (4 - col))) {
          ctx.fillStyle = color;
          ctx.fillRect(gx + col * size, y + row * size, size, size);
        }
      }
    }
  }
}

function drawBar(ctx, x, y, w, h, value, max, trackColor, fillColor) {
  ctx.fillStyle = trackColor || PAL.light;
  ctx.fillRect(x, y, w, h);
  const fillW = Math.floor(w * Math.min(value, max) / max);
  ctx.fillStyle = fillColor || PAL.fg;
  ctx.fillRect(x, y, fillW, h);
}

function drawCircleBresenham(ctx, cx, cy, r, color = PAL.fg) {
  ctx.strokeStyle = color;
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.stroke();
}

function drawCross(ctx, cx, cy, len, color = PAL.fg) {
  ctx.strokeStyle = color;
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(cx - len, cy); ctx.lineTo(cx + len, cy);
  ctx.moveTo(cx, cy - len); ctx.lineTo(cx, cy + len);
  ctx.stroke();
}

function renderDSLNode(ctx, node, x, y, w, h, debug) {
  if (!node) return;
  const kids = node.items || node.children || [];

  // Draw separators / borders
  if (node.border_b) {
    ctx.fillStyle = PAL.mid;
    ctx.fillRect(x, y + h - 1, w, 1);
  }
  if (node.border_t) {
    ctx.fillStyle = PAL.mid;
    ctx.fillRect(x, y, w, 1);
  }

  // Type-specific rendering
  if (node.type === "label" || node.label) {
    const text = node.content || node.label || "";
    const sz = node.size || 1;
    const col = node.invert ? PAL.bg : (node.color === "fg" ? PAL.fg : node.color === "ghost" ? PAL.ghost : node.color === "mid" ? PAL.mid : PAL.fg);
    if (node.invert) {
      ctx.fillStyle = PAL.fg;
      ctx.fillRect(x, y, (text.length * GLYPH_W * sz) + 6, GLYPH_H * sz + 4);
      drawBitmapText(ctx, text, x + 3, y + 2, sz, PAL.bg);
    } else {
      drawBitmapText(ctx, text, x + (node.px || 0), y + (node.py || 0), sz, col);
    }
  }

  if (node.type === "sep") {
    ctx.fillStyle = PAL.mid;
    ctx.fillRect(x, y, w, 1);
  }

  if (node.type === "bar") {
    drawBar(ctx, x, y, node.w || w, node.h || 3, node.value || 0, node.max || 100, PAL.light, PAL.fg);
  }

  if (node.type === "circle") {
    drawCircleBresenham(ctx, x + (node.cx || 0), y + (node.cy || 0), node.r || 20, PAL.mid);
  }

  if (node.type === "cross") {
    drawCross(ctx, x + (node.cx || 0), y + (node.cy || 0), node.len || 8, PAL.fg);
  }

  if (node.type === "gauge") {
    const val = node.value || Math.floor(Math.random() * node.max);
    const padStr = String(val).padStart(3, "0");
    drawBitmapText(ctx, node.label || "", x, y, 1, PAL.mid);
    drawBitmapText(ctx, padStr, x + 18, y, 1, PAL.fg);
    const barX = x + 50;
    const barW = w - 58;
    drawBar(ctx, barX, y + 3, barW, 3, val, node.max || 360, PAL.light, PAL.fg);
  }

  if (node.type === "dot") {
    ctx.fillStyle = PAL.fg;
    ctx.fillRect(x + 2, y + h/2 - 2, 4, 4);
  }

  if (node.type === "badge") {
    const text = node.content || "";
    ctx.fillStyle = PAL.fg;
    const bw = text.length * GLYPH_W + 8;
    ctx.fillRect(x, y + 2, bw, GLYPH_H + 4);
    drawBitmapText(ctx, text, x + 4, y + 4, 1, PAL.bg);
  }

  if (node.type === "icon") {
    const sz = 8;
    const ix = x + w/2 - sz/2;
    const iy = y + h/2 - sz/2;
    ctx.strokeStyle = PAL.mid;
    ctx.lineWidth = 1;
    if (node.shape === "square") ctx.strokeRect(ix, iy, sz, sz);
    else if (node.shape === "circle") { ctx.beginPath(); ctx.arc(ix + sz/2, iy + sz/2, sz/2, 0, Math.PI*2); ctx.stroke(); }
    else if (node.shape === "diamond") {
      ctx.beginPath(); ctx.moveTo(ix+sz/2,iy); ctx.lineTo(ix+sz,iy+sz/2); ctx.lineTo(ix+sz/2,iy+sz); ctx.lineTo(ix,iy+sz/2); ctx.closePath(); ctx.stroke();
    } else if (node.shape === "triangle") {
      ctx.beginPath(); ctx.moveTo(ix+sz/2,iy); ctx.lineTo(ix+sz,iy+sz); ctx.lineTo(ix,iy+sz); ctx.closePath(); ctx.stroke();
    }
  }

  if (node.type === "grid") {
    const cols = node.cols || 7;
    const cellW = node.cell_w || Math.floor(w / cols);
    const cellH = node.cell_h || 20;
    const count = node.count || cols * 5;
    for (let i = 0; i < count; i++) {
      const col = i % cols;
      const row = Math.floor(i / cols);
      const gx = x + col * cellW;
      const gy = y + row * cellH;
      ctx.strokeStyle = PAL.light;
      ctx.lineWidth = 0.5;
      ctx.strokeRect(gx + 0.5, gy + 0.5, cellW - 1, cellH - 1);
      const dayNum = (i % 31) + 1;
      if (node.today === i) {
        ctx.fillStyle = PAL.fg;
        ctx.fillRect(gx+1, gy+1, cellW-2, cellH-2);
        drawBitmapText(ctx, String(dayNum), gx + cellW - 18, gy + 3, 1, PAL.bg);
      } else {
        drawBitmapText(ctx, String(dayNum), gx + cellW - 18, gy + 3, 1, PAL.fg);
      }
      if (node.event_days && node.event_days.includes(dayNum)) {
        ctx.fillStyle = PAL.fg;
        ctx.fillRect(gx + cellW/2 - 1, gy + cellH - 4, 3, 3);
      }
    }
  }

  if (node.type === "list") {
    const rowH = node.row_h || 14;
    const maxItems = node.max || 6;
    const items = node.data || [];
    for (let i = 0; i < Math.min(items.length, maxItems); i++) {
      const ry = y + i * rowH;
      if (node.selected === i) {
        ctx.fillStyle = PAL.light;
        ctx.fillRect(x, ry, w, rowH);
      }
      const item = items[i];
      if (typeof item === "string") {
        drawBitmapText(ctx, item, x + 2, ry + 2, 1, PAL.fg);
      } else if (item.cols) {
        let cx2 = x + 2;
        item.cols.forEach(c => {
          drawBitmapText(ctx, c.text || "", cx2, ry + 2, 1, c.color === "mid" ? PAL.mid : c.color === "ghost" ? PAL.ghost : PAL.fg);
          cx2 += (c.w || 60);
        });
      }
    }
  }

  if (node.type === "text_block") {
    const lines = (node.content || "").split("\n");
    const lineH = node.line_h || 14;
    lines.forEach((line, i) => {
      if (y + i * lineH < y + h) {
        drawBitmapText(ctx, line, x, y + i * lineH, 1, PAL.fg);
      }
    });
  }

  if (node.type === "cursor") {
    ctx.fillStyle = PAL.fg;
    ctx.fillRect(x, y, 5, GLYPH_H);
  }

  // Recurse for layout containers
  if (node.type === "vbox" || node.layout === "vbox") {
    let fixedH = 0, flexCount = 0;
    kids.forEach(k => { if (k.h) fixedH += k.h; else flexCount++; });
    const flexH = flexCount > 0 ? Math.floor((h - fixedH) / flexCount) : 0;
    let cy = y;
    kids.forEach(k => {
      const ch = k.h || flexH;
      renderDSLNode(ctx, k, x, cy, w, ch, debug);
      cy += ch;
    });
  } else if (node.type === "hbox" || node.layout === "hbox") {
    if (node.split) {
      const lw = node.split;
      const rw = w - lw - 1;
      if (kids[0]) renderDSLNode(ctx, kids[0], x, y, lw, h, debug);
      // divider
      ctx.fillStyle = PAL.mid;
      ctx.fillRect(x + lw, y, 1, h);
      if (kids[1]) renderDSLNode(ctx, kids[1], x + lw + 1, y, rw, h, debug);
    } else {
      let fixedW = 0, flexCount = 0;
      kids.forEach(k => { if (k.w) fixedW += k.w; else if (k.spacer) flexCount++; else fixedW += ((k.label || k.content || "").length * GLYPH_W) + 8; });
      const flexW = flexCount > 0 ? Math.floor((w - fixedW) / flexCount) : 0;
      let cx2 = x;
      kids.forEach(k => {
        let cw;
        if (k.spacer) cw = flexW;
        else if (k.w) cw = k.w;
        else cw = ((k.label || k.content || "").length * GLYPH_W) + 8;
        renderDSLNode(ctx, k, cx2, y, cw, h, debug);
        cx2 += cw;
      });
    }
  } else if (node.type === "fixed") {
    kids.forEach(k => {
      const kx = x + (k.x || 0);
      const ky = y + (k.y || 0);
      const kw = k.w || w - (k.x || 0);
      const kh = k.h || h - (k.y || 0);
      renderDSLNode(ctx, k, kx, ky, kw, kh, debug);
    });
  }
}

// ── PRESETS ──
const PRESETS = {
  "main: Dashboard": {
    type: "vbox",
    items: [
      { type: "hbox", h: 16, border_b: true, items: [
        { type: "label", label: "GNOSIS//3.1", size: 1 },
        { spacer: true },
        { type: "label", label: "SIG:97%", size: 1, color: "mid" },
        { type: "label", label: "PWR:EINK", size: 1, color: "mid", w: 64 },
        { type: "dot", w: 12 }
      ]},
      { type: "hbox", split: 200, items: [
        { type: "fixed", items: [
          { type: "label", label: "CHRONO", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "label", label: "14:37", x: 8, y: 22, size: 4 },
          { type: "label", label: "2026.03.22", x: 8, y: 58, size: 1, color: "mid" },
          { type: "label", label: "SEC 42", x: 8, y: 72, size: 1, color: "ghost" },
          { type: "sep", x: 0, y: 96, w: 200 },
          { type: "label", label: "ORIENTATION", x: 8, y: 104, size: 1, color: "ghost" },
          { type: "circle", cx: 100, cy: 184, r: 60 },
          { type: "circle", cx: 100, cy: 184, r: 45 },
          { type: "circle", cx: 100, cy: 184, r: 28 },
          { type: "cross", cx: 100, cy: 184, len: 10 }
        ]},
        { type: "fixed", items: [
          { type: "label", label: "TELEMETRY", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "gauge", x: 8, y: 22, w: 180, label: "R", value: 15, max: 360 },
          { type: "gauge", x: 8, y: 36, w: 180, label: "P", value: 34, max: 360 },
          { type: "gauge", x: 8, y: 50, w: 180, label: "Y", value: 127, max: 360 },
          { type: "gauge", x: 8, y: 64, w: 180, label: "T", value: 291, max: 360 },
          { type: "gauge", x: 8, y: 78, w: 180, label: "V", value: 3, max: 360 },
          { type: "gauge", x: 8, y: 92, w: 180, label: "A", value: 188, max: 360 },
          { type: "sep", x: 0, y: 112, w: 200 },
          { type: "label", label: "SYS.LOG", x: 8, y: 120, size: 1, color: "ghost" },
          { type: "list", x: 8, y: 136, w: 180, h: 110, row_h: 16, max: 7, data: [
            { cols: [{ text: "00:00:01", w: 62, color: "ghost" }, { text: "kernel init" }] },
            { cols: [{ text: "00:00:02", w: 62, color: "ghost" }, { text: "epd driver ok" }] },
            { cols: [{ text: "00:00:02", w: 62, color: "ghost" }, { text: "mesh scan..." }] },
            { cols: [{ text: "00:00:03", w: 62, color: "ghost" }, { text: "3 nodes found" }] },
            { cols: [{ text: "00:00:04", w: 62, color: "ghost" }, { text: "telemetry open" }] },
            { cols: [{ text: "00:00:05", w: 62, color: "ghost" }, { text: "orient lock" }] },
            { cols: [{ text: "00:00:06", w: 62, color: "ghost" }, { text: "ready" }] }
          ]},
          { type: "cursor", x: 128, y: 232 }
        ]}
      ]},
      { type: "hbox", h: 16, border_t: true, items: [
        { type: "icon", shape: "square", w: 24 },
        { type: "icon", shape: "circle", w: 24 },
        { type: "icon", shape: "diamond", w: 24 },
        { type: "icon", shape: "triangle", w: 24 },
        { spacer: true },
        { type: "badge", content: "AUTO", w: 42 },
        { spacer: true },
        { type: "label", label: "PIXEL MONOSPACED", size: 1, color: "ghost" }
      ]}
    ]
  },

  "calendar: Temporal Map": {
    type: "vbox",
    items: [
      { type: "hbox", h: 16, border_b: true, items: [
        { type: "label", label: "GNOSIS//3.1" },
        { spacer: true },
        { type: "label", label: "SIG:97%", color: "mid" },
        { type: "label", label: "PWR:EINK", color: "mid", w: 64 }
      ]},
      { type: "hbox", split: 260, items: [
        { type: "fixed", items: [
          { type: "label", label: "TEMPORAL MAP", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "label", label: "MARCH 2026", x: 8, y: 22, size: 2 },
          { type: "grid", x: 8, y: 50, cols: 7, cell_w: 35, cell_h: 26, count: 35, today: 21,
            event_days: [3, 7, 12, 15, 21, 25, 28] }
        ]},
        { type: "fixed", items: [
          { type: "label", label: "AGENDA", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "label", label: "2026.03.22", x: 8, y: 22, size: 1 },
          { type: "list", x: 8, y: 40, w: 120, h: 120, row_h: 28, max: 4, data: [
            { cols: [{ text: "09:00", w: 42, color: "mid" }, { text: "Team sync" }] },
            { cols: [{ text: "11:30", w: 42, color: "mid" }, { text: "Design rev" }] },
            { cols: [{ text: "14:00", w: 42, color: "mid" }, { text: "EPD testing" }] },
            { cols: [{ text: "16:30", w: 42, color: "mid" }, { text: "Code review" }] }
          ]},
          { type: "sep", x: 0, y: 174, w: 140 },
          { type: "label", label: "UPCOMING", x: 8, y: 182, size: 1, color: "ghost" },
          { type: "list", x: 8, y: 198, w: 120, h: 60, row_h: 16, max: 3, data: [
            { cols: [{ text: "03.24", w: 42, color: "ghost" }, { text: "Sprint end" }] },
            { cols: [{ text: "03.27", w: 42, color: "ghost" }, { text: "Demo day" }] },
            { cols: [{ text: "04.01", w: 42, color: "ghost" }, { text: "Release v4" }] }
          ]}
        ]}
      ]},
      { type: "hbox", h: 16, border_t: true, items: [
        { type: "icon", shape: "square", w: 24 },
        { type: "icon", shape: "circle", w: 24 },
        { type: "icon", shape: "diamond", w: 24 },
        { type: "icon", shape: "triangle", w: 24 },
        { spacer: true },
        { type: "badge", content: "AUTO", w: 42 }
      ]}
    ]
  },

  "mail: Inbox": {
    type: "vbox",
    items: [
      { type: "hbox", h: 16, border_b: true, items: [
        { type: "label", label: "GNOSIS//3.1" },
        { spacer: true },
        { type: "label", label: "3 UNREAD", color: "mid" },
        { type: "dot", w: 12 }
      ]},
      { type: "hbox", split: 165, items: [
        { type: "fixed", items: [
          { type: "label", label: "INBOX", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "label", label: "3 UNREAD", x: 8, y: 20, size: 1 },
          { type: "list", x: 4, y: 40, w: 155, h: 210, row_h: 34, max: 6, selected: 0, data: [
            { cols: [{ text: "K.Tanaka", w: 80 }, { text: "10:42", w: 40, color: "ghost" }] },
            { cols: [{ text: "Re: EPD specs", w: 130 }] },
            { cols: [{ text: "M.Chen", w: 80 }, { text: "09:15", w: 40, color: "ghost" }] },
            { cols: [{ text: "Firmware v2.1", w: 130 }] },
            { cols: [{ text: "J.Park", w: 80 }, { text: "YST", w: 40, color: "ghost" }] },
            { cols: [{ text: "Board layout", w: 130 }] }
          ]}
        ]},
        { type: "fixed", items: [
          { type: "label", label: "MESSAGE", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "label", label: "Re: EPD specs", x: 8, y: 22, size: 2 },
          { type: "label", label: "FROM K.Tanaka", x: 8, y: 48, size: 1, color: "mid" },
          { type: "label", label: "DATE 2026.03.22 10:42", x: 8, y: 62, size: 1, color: "mid" },
          { type: "sep", x: 0, y: 78, w: 230 },
          { type: "text_block", x: 8, y: 86, w: 210, h: 130, line_h: 14,
            content: "The new EPD controller\nsamples arrived. SSD1680\nshows 50ms partial with\nno visible ghosting at\n1-bit depth.\n\nDU4 waveform needs more\ntesting on the 4.2 inch\npanel. Will send results\nby end of week." },
          { type: "sep", x: 0, y: 224, w: 230 },
          { type: "label", label: "REPLY  FWD  FLAG", x: 8, y: 232, size: 1, color: "mid" }
        ]}
      ]},
      { type: "hbox", h: 16, border_t: true, items: [
        { type: "icon", shape: "square", w: 24 },
        { type: "icon", shape: "circle", w: 24 },
        { type: "icon", shape: "diamond", w: 24 },
        { type: "icon", shape: "triangle", w: 24 },
        { spacer: true },
        { type: "badge", content: "AUTO", w: 42 }
      ]}
    ]
  },

  "reader: Library": {
    type: "vbox",
    items: [
      { type: "hbox", h: 16, border_b: true, items: [
        { type: "label", label: "GNOSIS//3.1" },
        { spacer: true },
        { type: "label", label: "PWR:EINK", color: "mid", w: 64 }
      ]},
      { type: "hbox", split: 145, items: [
        { type: "fixed", items: [
          { type: "label", label: "LIBRARY", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "label", label: "7 VOL", x: 8, y: 20, size: 1, color: "mid" },
          { type: "list", x: 4, y: 38, w: 135, h: 210, row_h: 30, max: 7, selected: 2, data: [
            { cols: [{ text: "Neuromancer" }] },
            { cols: [{ text: "Gibson  72%", color: "ghost" }] },
            { cols: [{ text: "Snow Crash" }] },
            { cols: [{ text: "Stephenson  45%", color: "ghost" }] },
            { cols: [{ text: "Dune" }] },
            { cols: [{ text: "Herbert  100%", color: "ghost" }] },
            { cols: [{ text: "Solaris" }] }
          ]}
        ]},
        { type: "fixed", items: [
          { type: "label", label: "READER", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "label", label: "SNOW CRASH", x: 8, y: 22, size: 2 },
          { type: "label", label: "Neal Stephenson", x: 8, y: 44, size: 1, color: "mid" },
          { type: "label", label: "CH 12", x: 190, y: 44, size: 1, color: "ghost" },
          { type: "sep", x: 0, y: 58, w: 250 },
          { type: "text_block", x: 8, y: 66, w: 230, h: 160, line_h: 16,
            content: "The Deliverator belongs\nto an elite order, a\nhallowed sub-category.\nHe is a pizza delivery\ndriver.\n\nThere are only a few combos\nin the Metaverse that are\nworse than this. CosaNostra\nPizza does not deliver in\nthirty minutes or less." },
          { type: "sep", x: 0, y: 232, w: 250 },
          { type: "label", label: "P.187/440", x: 8, y: 238, size: 1, color: "ghost" },
          { type: "bar", x: 72, y: 242, w: 100, h: 2, value: 45, max: 100 },
          { type: "label", label: "BM NT", x: 190, y: 238, size: 1, color: "mid" }
        ]}
      ]},
      { type: "hbox", h: 16, border_t: true, items: [
        { type: "icon", shape: "square", w: 24 },
        { type: "icon", shape: "circle", w: 24 },
        { type: "icon", shape: "diamond", w: 24 },
        { type: "icon", shape: "triangle", w: 24 },
        { spacer: true },
        { type: "badge", content: "AUTO", w: 42 }
      ]}
    ]
  },

  "minimal: Empty Screen": {
    type: "vbox",
    items: [
      { type: "hbox", h: 16, border_b: true, items: [
        { type: "label", label: "GNOSIS//3.1" },
        { spacer: true },
        { type: "dot", w: 12 }
      ]},
      { type: "fixed", items: [
        { type: "circle", cx: 200, cy: 130, r: 80 },
        { type: "circle", cx: 200, cy: 130, r: 55 },
        { type: "cross", cx: 200, cy: 130, len: 16 },
        { type: "label", label: "STANDBY", x: 170, y: 220, size: 1, color: "ghost" }
      ]},
      { type: "hbox", h: 16, border_t: true, items: [
        { type: "icon", shape: "square", w: 24 },
        { type: "icon", shape: "circle", w: 24 },
        { type: "icon", shape: "diamond", w: 24 },
        { type: "icon", shape: "triangle", w: 24 },
        { spacer: true },
        { type: "badge", content: "AUTO", w: 42 }
      ]}
    ]
  },

  "debug: Widget Gallery": {
    type: "vbox",
    items: [
      { type: "hbox", h: 16, border_b: true, items: [
        { type: "label", label: "WIDGET GALLERY" },
        { spacer: true },
        { type: "label", label: "DEBUG", color: "ghost" }
      ]},
      { type: "fixed", items: [
        { type: "label", label: "SIZE 1 TEXT", x: 8, y: 8, size: 1 },
        { type: "label", label: "SIZE 2", x: 8, y: 22, size: 2 },
        { type: "label", label: "BIG", x: 8, y: 44, size: 4 },
        { type: "sep", x: 8, y: 80, w: 180 },
        { type: "gauge", x: 8, y: 88, w: 180, label: "A", value: 42, max: 100 },
        { type: "gauge", x: 8, y: 104, w: 180, label: "B", value: 88, max: 100 },
        { type: "gauge", x: 8, y: 120, w: 180, label: "C", value: 7, max: 100 },
        { type: "sep", x: 8, y: 140, w: 180 },
        { type: "bar", x: 8, y: 148, w: 180, h: 4, value: 65, max: 100 },
        { type: "bar", x: 8, y: 158, w: 180, h: 4, value: 30, max: 100 },
        { type: "sep", x: 8, y: 170, w: 180 },
        { type: "circle", cx: 60, cy: 210, r: 30 },
        { type: "cross", cx: 60, cy: 210, len: 12 },
        { type: "circle", cx: 150, cy: 210, r: 20 },
        { type: "circle", cx: 150, cy: 210, r: 30 },
        { type: "circle", cx: 150, cy: 210, r: 40 },
        { type: "label", label: "ICONS:", x: 210, y: 88, size: 1, color: "ghost" },
        { type: "badge", content: "BADGE", x: 210, y: 106 },
        { type: "label", label: "cursor>", x: 210, y: 128, size: 1, color: "ghost" },
        { type: "cursor", x: 260, y: 128 },
        { type: "list", x: 210, y: 150, w: 160, h: 100, row_h: 18, max: 5, selected: 1, data: [
          "List item 0",
          "Selected item",
          "List item 2",
          "List item 3",
          "List item 4"
        ]}
      ]},
      { type: "hbox", h: 16, border_t: true, items: [
        { type: "icon", shape: "square", w: 24 },
        { type: "icon", shape: "circle", w: 24 },
        { type: "icon", shape: "diamond", w: 24 },
        { type: "icon", shape: "triangle", w: 24 },
        { spacer: true },
        { type: "badge", content: "DBG", w: 36 }
      ]}
    ]
  },

  "telemetry: Full Gauges": {
    type: "vbox",
    items: [
      { type: "hbox", h: 16, border_b: true, items: [
        { type: "label", label: "GNOSIS//3.1" },
        { spacer: true },
        { type: "label", label: "TELEMETRY FULL", color: "mid" }
      ]},
      { type: "hbox", split: 200, items: [
        { type: "fixed", items: [
          { type: "label", label: "PRIMARY", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "gauge", x: 8, y: 24, w: 180, label: "R", value: 15, max: 360 },
          { type: "gauge", x: 8, y: 42, w: 180, label: "P", value: 34, max: 360 },
          { type: "gauge", x: 8, y: 60, w: 180, label: "Y", value: 127, max: 360 },
          { type: "gauge", x: 8, y: 78, w: 180, label: "T", value: 291, max: 360 },
          { type: "gauge", x: 8, y: 96, w: 180, label: "V", value: 3, max: 360 },
          { type: "gauge", x: 8, y: 114, w: 180, label: "A", value: 188, max: 360 },
          { type: "sep", x: 0, y: 138, w: 200 },
          { type: "label", label: "SECONDARY", x: 8, y: 146, size: 1, color: "ghost" },
          { type: "gauge", x: 8, y: 164, w: 180, label: "X", value: 44, max: 100 },
          { type: "gauge", x: 8, y: 182, w: 180, label: "Z", value: 77, max: 100 },
          { type: "gauge", x: 8, y: 200, w: 180, label: "W", value: 12, max: 100 },
          { type: "gauge", x: 8, y: 218, w: 180, label: "Q", value: 95, max: 100 }
        ]},
        { type: "fixed", items: [
          { type: "label", label: "POSITION", x: 8, y: 6, size: 1, color: "ghost" },
          { type: "circle", cx: 100, cy: 100, r: 70 },
          { type: "circle", cx: 100, cy: 100, r: 50 },
          { type: "circle", cx: 100, cy: 100, r: 28 },
          { type: "cross", cx: 100, cy: 100, len: 12 },
          { type: "label", label: "000", x: 90, y: 22, size: 1, color: "mid" },
          { type: "label", label: "090", x: 172, y: 96, size: 1, color: "mid" },
          { type: "label", label: "180", x: 90, y: 174, size: 1, color: "mid" },
          { type: "label", label: "270", x: 10, y: 96, size: 1, color: "mid" },
          { type: "sep", x: 0, y: 194, w: 200 },
          { type: "label", label: "STATUS", x: 8, y: 202, size: 1, color: "ghost" },
          { type: "label", label: "MESH: ACTIVE", x: 8, y: 218, size: 1 },
          { type: "label", label: "NODES: 3/8", x: 8, y: 232, size: 1 }
        ]}
      ]},
      { type: "hbox", h: 16, border_t: true, items: [
        { type: "icon", shape: "square", w: 24 },
        { type: "icon", shape: "circle", w: 24 },
        { type: "icon", shape: "diamond", w: 24 },
        { type: "icon", shape: "triangle", w: 24 },
        { spacer: true },
        { type: "badge", content: "AUTO", w: 42 }
      ]}
    ]
  },

  "boot: Startup Sequence": {
    type: "vbox",
    items: [
      { type: "hbox", h: 16, border_b: true, items: [
        { type: "label", label: "GNOSIS//BOOT" },
        { spacer: true }
      ]},
      { type: "fixed", items: [
        { type: "circle", cx: 200, cy: 110, r: 70 },
        { type: "circle", cx: 200, cy: 110, r: 50 },
        { type: "circle", cx: 200, cy: 110, r: 28 },
        { type: "circle", cx: 200, cy: 110, r: 8 },
        { type: "cross", cx: 200, cy: 110, len: 80 },
        { type: "label", label: "GNOSIS", x: 162, y: 192, size: 2 },
        { type: "label", label: "E-INK OPERATING SYSTEM", x: 108, y: 214, size: 1, color: "mid" },
        { type: "bar", x: 100, y: 234, w: 200, h: 3, value: 72, max: 100 },
        { type: "label", label: "72%", x: 190, y: 242, size: 1, color: "ghost" }
      ]},
      { type: "hbox", h: 16, border_t: true, items: [
        { type: "label", label: "INITIATED:2022", color: "ghost" },
        { spacer: true },
        { type: "label", label: "CLASSIFICATION:PIXEL MONO", color: "ghost" }
      ]}
    ]
  }
};

const DIRTY_RECT_EXAMPLES = [
  { x: 10, y: 20, w: 180, h: 60, waveform: "FAST" },
  { x: 210, y: 120, w: 150, h: 40, waveform: "PART" },
];

// ── MAIN APP ──
export default function GnosisEngine() {
  const canvasRef = useRef(null);
  const [preset, setPreset] = useState("main: Dashboard");
  const [jsonText, setJsonText] = useState(JSON.stringify(PRESETS["main: Dashboard"], null, 2));
  const [parseError, setParseError] = useState(null);
  const [debug, setDebug] = useState({
    showBounds: false,
    showDepth: false,
    showCrosshair: true,
    showDirty: false,
    dirtyRects: DIRTY_RECT_EXAMPLES,
  });
  const [hovered, setHovered] = useState(null);
  const [layoutNodes, setLayoutNodes] = useState([]);
  const [canvasScale] = useState(2);

  const CW = 400;
  const CH = 280;

  const render = useCallback(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    ctx.save();
    ctx.scale(canvasScale, canvasScale);

    let dsl;
    try {
      dsl = JSON.parse(jsonText);
      setParseError(null);
    } catch (e) {
      setParseError(e.message);
      ctx.restore();
      return;
    }

    const nodes = layoutNode(dsl, 0, 0, CW, CH, [], 0);
    setLayoutNodes(nodes);
    renderToCanvas(ctx, nodes, dsl, debug);
    ctx.restore();
  }, [jsonText, debug, canvasScale]);

  useEffect(() => { render(); }, [render]);

  const handlePresetChange = (name) => {
    setPreset(name);
    setJsonText(JSON.stringify(PRESETS[name], null, 2));
  };

  const toggleDebug = (key) => {
    setDebug(d => ({ ...d, [key]: !d[key] }));
  };

  const handleCanvasMove = (e) => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    const mx = (e.clientX - rect.left) / (rect.width / CW);
    const my = (e.clientY - rect.top) / (rect.height / CH);
    // Find deepest node under cursor
    let best = null;
    layoutNodes.forEach(n => {
      if (mx >= n.x && mx <= n.x + n.w && my >= n.y && my <= n.y + n.h) {
        if (!best || n.depth > best.depth) best = n;
      }
    });
    setHovered(best);
  };

  const editorBg = "#1e1d1b";
  const editorFg = "#c4c0b4";
  const editorBorder = "#3a3834";

  return (
    <div style={{
      display: "flex", height: "100vh", width: "100vw",
      background: "#131210", color: editorFg,
      fontFamily: "'Share Tech Mono', 'Courier New', monospace",
      overflow: "hidden"
    }}>
      {/* LEFT: Editor */}
      <div style={{
        width: 420, minWidth: 320, display: "flex", flexDirection: "column",
        borderRight: `1px solid ${editorBorder}`,
        background: editorBg,
      }}>
        {/* Toolbar */}
        <div style={{
          padding: "10px 12px", borderBottom: `1px solid ${editorBorder}`,
          display: "flex", flexDirection: "column", gap: 8, flexShrink: 0
        }}>
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
            <span style={{ fontSize: 11, letterSpacing: 3, color: "#7a7770", textTransform: "uppercase" }}>
              GNOSIS // DSL Engine
            </span>
            <span style={{ fontSize: 9, letterSpacing: 2, color: "#4a4840" }}>v3.1</span>
          </div>

          <select
            value={preset}
            onChange={e => handlePresetChange(e.target.value)}
            style={{
              background: "#2a2924", color: editorFg, border: `1px solid ${editorBorder}`,
              padding: "6px 8px", fontSize: 12, fontFamily: "inherit",
              letterSpacing: 1, cursor: "pointer", outline: "none",
              appearance: "none", WebkitAppearance: "none",
              backgroundImage: `url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='8' height='5'%3E%3Cpath d='M0 0l4 5 4-5z' fill='%237a7770'/%3E%3C/svg%3E")`,
              backgroundRepeat: "no-repeat",
              backgroundPosition: "right 8px center",
              paddingRight: 24,
            }}
          >
            {Object.keys(PRESETS).map(k => (
              <option key={k} value={k}>{k}</option>
            ))}
          </select>
        </div>

        {/* JSON editor */}
        <div style={{ flex: 1, overflow: "hidden", position: "relative" }}>
          <textarea
            value={jsonText}
            onChange={e => setJsonText(e.target.value)}
            spellCheck={false}
            style={{
              width: "100%", height: "100%",
              background: "transparent", color: "#a09c90",
              border: "none", outline: "none", resize: "none",
              padding: "12px", fontSize: 11, lineHeight: 1.6,
              fontFamily: "inherit", letterSpacing: 0.5,
              tabSize: 2,
            }}
          />
          {parseError && (
            <div style={{
              position: "absolute", bottom: 0, left: 0, right: 0,
              background: "#3a1515", color: "#e04040",
              padding: "8px 12px", fontSize: 10, letterSpacing: 1,
              borderTop: "1px solid #5a2020"
            }}>
              PARSE ERROR: {parseError}
            </div>
          )}
        </div>

        {/* Node info */}
        <div style={{
          padding: "8px 12px", borderTop: `1px solid ${editorBorder}`,
          fontSize: 10, letterSpacing: 1, color: "#5a5850",
          flexShrink: 0, minHeight: 48, lineHeight: 1.8
        }}>
          {hovered ? (
            <>
              <span style={{ color: "#8a8680" }}>NODE </span>
              <span style={{ color: editorFg }}>{hovered.type}{hovered.id ? `:${hovered.id}` : ""}</span>
              <span style={{ color: "#5a5850" }}> | </span>
              <span style={{ color: "#8a8680" }}>RECT </span>
              <span style={{ color: editorFg }}>{hovered.x},{hovered.y} {hovered.w}x{hovered.h}</span>
              <span style={{ color: "#5a5850" }}> | </span>
              <span style={{ color: "#8a8680" }}>D:</span>
              <span style={{ color: editorFg }}>{hovered.depth}</span>
              <span style={{ color: "#5a5850" }}> | </span>
              <span style={{ color: "#8a8680" }}>WF:</span>
              <span style={{ color: editorFg }}>{hovered.waveform}</span>
            </>
          ) : (
            <span>HOVER CANVAS FOR NODE INFO</span>
          )}
        </div>
      </div>

      {/* RIGHT: Canvas + Debug */}
      <div style={{ flex: 1, display: "flex", flexDirection: "column", background: "#0e0d0b" }}>
        {/* Debug toolbar */}
        <div style={{
          padding: "8px 16px",
          borderBottom: `1px solid ${editorBorder}`,
          display: "flex", gap: 6, alignItems: "center", flexWrap: "wrap",
          flexShrink: 0,
        }}>
          <span style={{ fontSize: 9, letterSpacing: 2, color: "#4a4840", marginRight: 8 }}>DEBUG</span>
          {[
            { key: "showBounds", label: "BOUNDS" },
            { key: "showDepth", label: "DEPTH" },
            { key: "showCrosshair", label: "CROSS" },
            { key: "showDirty", label: "DIRTY" },
          ].map(({ key, label }) => (
            <button
              key={key}
              onClick={() => toggleDebug(key)}
              style={{
                background: debug[key] ? PAL.fg : "#2a2924",
                color: debug[key] ? PAL.bg : "#7a7770",
                border: `1px solid ${debug[key] ? PAL.fg : editorBorder}`,
                padding: "3px 10px",
                fontSize: 10, fontFamily: "inherit", letterSpacing: 2,
                cursor: "pointer", transition: "all 0.15s",
              }}
            >
              {label}
            </button>
          ))}
          <div style={{ flex: 1 }} />
          <span style={{ fontSize: 9, letterSpacing: 1, color: "#4a4840" }}>
            {CW}x{CH} @ 1BPP = {Math.ceil(CW * CH / 8).toLocaleString()}B
          </span>
        </div>

        {/* Canvas area */}
        <div style={{
          flex: 1, display: "flex", justifyContent: "center", alignItems: "center",
          padding: 24, position: "relative",
          backgroundImage: `radial-gradient(circle at 1px 1px, #1a1916 1px, transparent 0)`,
          backgroundSize: "16px 16px",
        }}>
          {/* EPD bezel */}
          <div style={{
            background: "#2a2824",
            padding: "16px 16px 20px 16px",
            boxShadow: "0 4px 40px rgba(0,0,0,0.6), inset 0 1px 0 rgba(255,255,255,0.03)",
            position: "relative",
          }}>
            <div style={{
              position: "absolute", top: 6, left: "50%", transform: "translateX(-50%)",
              fontSize: 7, letterSpacing: 3, color: "#3a3834",
            }}>
              GNOSIS EPD-400
            </div>
            <canvas
              ref={canvasRef}
              width={CW * canvasScale}
              height={CH * canvasScale}
              onMouseMove={handleCanvasMove}
              onMouseLeave={() => setHovered(null)}
              style={{
                width: CW * canvasScale,
                height: CH * canvasScale,
                imageRendering: "pixelated",
                cursor: "crosshair",
                display: "block",
              }}
            />
          </div>

          {/* Stats overlay */}
          <div style={{
            position: "absolute", bottom: 16, left: 16,
            fontSize: 9, letterSpacing: 1, color: "#3a3834", lineHeight: 2,
          }}>
            NODES: {layoutNodes.length}
            <br />
            TREE DEPTH: {layoutNodes.reduce((m, n) => Math.max(m, n.depth), 0)}
            <br />
            LAYOUT: O(N) = O({layoutNodes.length})
            <br />
            FB: {Math.ceil(CW * CH / 8).toLocaleString()} BYTES
          </div>

          <div style={{
            position: "absolute", bottom: 16, right: 16,
            fontSize: 9, letterSpacing: 1, color: "#3a3834", lineHeight: 2,
            textAlign: "right",
          }}>
            TARGET: CORTEX-M4
            <br />
            RAM: 20KB
            <br />
            EPD: {CW}x{CH} 1-BIT
            <br />
            FONT: 8x12 BITMAP
          </div>
        </div>
      </div>
    </div>
  );
}
