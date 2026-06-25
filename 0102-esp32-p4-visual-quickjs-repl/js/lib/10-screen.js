// picoOS fixed-cell screen buffer. No browser, Node, module, or std/os APIs.
var Pico = (function (root) {
  var Pico = root.Pico || {};

  var FRAMES = {
    rounded: "╭╮╰╯─│",
    single: "┌┐└┘─│",
    double: "╔╗╚╝═║",
    bold: "┏┓┗┛━┃"
  };

  function makeCell(ch, style) {
    style = style || {};
    return {
      ch: ch == null || ch === "" ? " " : String(ch).charAt(0),
      fg: style.fg || "fg",
      bold: !!style.bold,
      dim: !!style.dim
    };
  }

  function makeScreen(cols, rows) {
    cols = Math.max(1, Math.floor(Number(cols || 40)));
    rows = Math.max(1, Math.floor(Number(rows || 30)));
    var cells = [];

    function clear() {
      cells = [];
      for (var y = 0; y < rows; y++) {
        var row = [];
        for (var x = 0; x < cols; x++) row.push(makeCell(" "));
        cells.push(row);
      }
    }

    function inBounds(x, y) {
      return x >= 0 && y >= 0 && x < cols && y < rows;
    }

    function set(x, y, ch, style) {
      x = Math.round(Number(x));
      y = Math.round(Number(y));
      if (!inBounds(x, y)) return;
      cells[y][x] = makeCell(ch, style);
    }

    function get(x, y) {
      x = Math.round(Number(x));
      y = Math.round(Number(y));
      if (!inBounds(x, y)) return makeCell(" ");
      return cells[y][x];
    }

    function text(x, y, value, style) {
      var s = String(value == null ? "" : value);
      for (var i = 0; i < s.length; i++) set(Number(x) + i, y, s.charAt(i), style);
    }

    function hline(x, y, width, ch, style) {
      for (var i = 0; i < width; i++) set(Number(x) + i, y, ch, style);
    }

    function vline(x, y, height, ch, style) {
      for (var i = 0; i < height; i++) set(x, Number(y) + i, ch, style);
    }

    function box(x, y, width, height, frame, style) {
      width = Math.floor(Number(width));
      height = Math.floor(Number(height));
      if (width <= 0 || height <= 0) return;
      var f = FRAMES[frame] || FRAMES.single;
      var tl = f.charAt(0), tr = f.charAt(1), bl = f.charAt(2), br = f.charAt(3);
      var hz = f.charAt(4), vt = f.charAt(5);
      if (width === 1 && height === 1) { set(x, y, tl, style); return; }
      if (height === 1) { hline(x, y, width, hz, style); set(x, y, tl, style); set(Number(x) + width - 1, y, tr, style); return; }
      if (width === 1) { vline(x, y, height, vt, style); set(x, y, tl, style); set(x, Number(y) + height - 1, bl, style); return; }
      set(x, y, tl, style);
      set(Number(x) + width - 1, y, tr, style);
      set(x, Number(y) + height - 1, bl, style);
      set(Number(x) + width - 1, Number(y) + height - 1, br, style);
      hline(Number(x) + 1, y, width - 2, hz, style);
      hline(Number(x) + 1, Number(y) + height - 1, width - 2, hz, style);
      vline(x, Number(y) + 1, height - 2, vt, style);
      vline(Number(x) + width - 1, Number(y) + 1, height - 2, vt, style);
    }

    function lineText(y, trimRight) {
      var s = "";
      for (var x = 0; x < cols; x++) s += cells[y][x].ch;
      if (trimRight !== false) s = s.replace(/[ ]+$/g, "");
      return s;
    }

    function toLines(trimRight) {
      var out = [];
      for (var y = 0; y < rows; y++) out.push(lineText(y, trimRight));
      return out;
    }

    function toText(trimRight) {
      return toLines(trimRight).join("\n");
    }

    clear();
    return {
      cols: cols,
      rows: rows,
      get cells() { return cells; },
      clear: clear,
      set: set,
      get: get,
      text: text,
      hline: hline,
      vline: vline,
      box: box,
      lineText: lineText,
      toLines: toLines,
      toText: toText
    };
  }

  Pico.FRAMES = FRAMES;
  Pico.makeScreen = makeScreen;
  return Pico;
})(globalThis);
