// picoOS fluent TUI runtime for portable QuickJS examples.
var Pico = (function (root) {
  var Pico = root.Pico || {};

  function style(fg, bold, dim) { return { fg: fg || "fg", bold: !!bold, dim: !!dim }; }
  function asText(v) { var r = Pico.resolve(v); return String(r == null ? "" : r); }

  function createRuntime(options) {
    options = options || {};
    var screen = options.screen || Pico.makeScreen(options.cols || 40, options.rows || 30);
    var OS = options.OS || Pico.createOS({ seed: options.seed || 1, baseSeconds: options.baseSeconds });
    var currentApp = null;

    function App(name) {
      this.name = name;
      this._state = {};
      this._draws = [];
      this._keys = {};
      this._timers = [];
      this._loops = [];
      this._computes = [];
      this._focusables = [];
      this._focusInput = null;
      this._statusbar = null;
      this._reserved = 1;
      this._panels = {};
      this._layout = null;
      this._mounted = false;
      this._exited = false;
      var self = this;
      var loopFn = function (fps, fn) {
        var loop = { fps: fps, fn: fn, acc: 0, paused: false };
        self._loops.push(loop);
        return loop;
      };
      loopFn.toggle = function () {
        var loop = self._loops[self._loops.length - 1];
        if (loop) loop.paused = !loop.paused;
      };
      this.loop = loopFn;
    }
    App.prototype.state = function (obj) { this._state = obj || {}; return this._state; };
    App.prototype.layout = function (fn) { var l = new Layout(this); fn(l); this._layout = l; return this; };
    App.prototype.panel = function (id) {
      var rect = { x: 0, y: 0, w: screen.cols, h: screen.rows - this._reserved };
      if (this._layout && this._layout.regions[id]) rect = this._layout.regions[id];
      var p = new Panel(this, rect);
      this._panels[id] = p;
      return p;
    };
    App.prototype.key = function (spec, fn) {
      spec = String(spec);
      if (spec.indexOf("↑") >= 0 || spec.indexOf("↓") >= 0 || spec.indexOf("←") >= 0 || spec.indexOf("→") >= 0) {
        for (var i = 0; i < spec.length; i++) if ("↑↓←→".indexOf(spec.charAt(i)) >= 0) this._keys[spec.charAt(i)] = fn;
      } else {
        this._keys[spec] = fn;
      }
      return this;
    };
    App.prototype.on = function (event, ms, fn) { if (event === "tick") this._timers.push({ ms: ms, fn: fn, acc: 0 }); return this; };
    App.prototype.compute = function (fn) { this._computes.push(fn); return this; };
    App.prototype.statusbar = function (v) { this._statusbar = v; return this; };
    App.prototype.refresh = function () { return this; };
    App.prototype.dispatch = function (tok) { this._fireKey(tok); return this; };
    App.prototype.exit = function () { this._exited = true; return this; };
    App.prototype.mount = function () { this._mounted = true; currentApp = this; return this; };
    App.prototype._focus = function () { return this._focusInput || this._focusables[0] || null; };
    App.prototype._fireKey = function (tok) {
      if (this._keys[tok]) { this._keys[tok](this, tok); return; }
      var f = this._focus();
      if (!f) return;
      if ("↑↓←→".indexOf(tok) >= 0 && f.move) f.move(tok);
      else if (tok === "⏎" && f.activate) f.activate();
      else if (f.type) f.type(tok);
    };
    App.prototype._frame = function (dt) {
      for (var i = 0; i < this._timers.length; i++) {
        var t = this._timers[i];
        t.acc += dt;
        if (t.acc >= t.ms) { t.acc = 0; t.fn(this); }
      }
      for (i = 0; i < this._loops.length; i++) {
        var l = this._loops[i];
        if (l.paused) continue;
        l.acc += dt;
        var step = 1000 / l.fps;
        while (l.acc >= step) { l.acc -= step; l.fn(this); }
      }
      for (i = 0; i < this._computes.length; i++) this._computes[i](this);
      screen.clear();
      var draws = this._draws.slice().sort(function (a, b) { return a.z - b.z; });
      for (i = 0; i < draws.length; i++) draws[i].fn(screen);
      if (this._statusbar) drawStatusbar(screen, this._statusbar);
    };

    function Layout(app) { this.app = app; this.regions = {}; this._axis = null; this._segs = []; }
    Layout.prototype._add = function (axis, size, id) { this._axis = this._axis || axis; this._segs.push({ size: size, id: id }); this._recompute(); return this; };
    Layout.prototype.row = function (size, id) { return this._add("rows", size, id); };
    Layout.prototype.col = function (size, id) { return this._add("cols", size, id); };
    Layout.prototype._recompute = function () {
      var total = this._axis === "cols" ? screen.cols : screen.rows - (this.app._reserved || 0);
      var fixed = 0, stars = 0;
      for (var i = 0; i < this._segs.length; i++) {
        if (this._segs[i].size === "*") stars++; else fixed += parseInt(this._segs[i].size, 10) || 0;
      }
      var starSize = stars ? Math.max(1, Math.floor((total - fixed) / stars)) : 0;
      var pos = 0;
      for (i = 0; i < this._segs.length; i++) {
        var seg = this._segs[i];
        var sz = seg.size === "*" ? starSize : (parseInt(seg.size, 10) || 0);
        if (i === this._segs.length - 1) sz = total - pos;
        this.regions[seg.id] = this._axis === "cols" ? { x: pos, y: 0, w: sz, h: total } : { x: 0, y: pos, w: screen.cols, h: sz };
        pos += sz;
      }
    };

    function Panel(app, rect) {
      this.app = app; this.rect = rect;
      this._frame = null; this._fst = {}; this._title = null; this._titleR = null; this._footer = null;
      this._reg(0, this._drawChrome.bind(this));
    }
    Panel.prototype.content = function () {
      var r = this.rect;
      return this._frame ? { x: r.x + 1, y: r.y + 1, w: Math.max(0, r.w - 2), h: Math.max(0, r.h - 2) } : { x: r.x, y: r.y, w: r.w, h: r.h };
    };
    Panel.prototype._reg = function (z, fn) { this.app._draws.push({ z: z, fn: fn }); };
    Panel.prototype._drawChrome = function (s) {
      var r = this.rect;
      if (this._frame) s.box(r.x, r.y, r.w, r.h, this._frame, { fg: this._fst.fg || "dim" });
      if (this._title != null) s.text(r.x + 2, r.y, " " + asText(this._title).replace(/^\s+|\s+$/g, "") + " ", { fg: "white", bold: true });
      if (this._titleR != null) { var t = " " + asText(this._titleR).replace(/^\s+|\s+$/g, "") + " "; s.text(r.x + r.w - 2 - t.length, r.y, t, { fg: "amber" }); }
      if (this._footer != null) { var f = " " + asText(this._footer).replace(/^\s+|\s+$/g, "") + " "; s.text(r.x + 2, r.y + r.h - 1, f, { fg: "dim" }); }
    };
    Panel.prototype.frame = function (frame, st) { this._frame = frame || "single"; this._fst = st || {}; return this; };
    Panel.prototype.title = function (v) { this._title = v; return this; };
    Panel.prototype.titleRight = function (v) { this._titleR = v; return this; };
    Panel.prototype.footer = function (v) { this._footer = v; return this; };
    Panel.prototype.text = function (v) { return new Text(this, v); };
    Panel.prototype.gauge = function () { return new Gauge(this); };
    Panel.prototype.spark = function () { return new Spark(this); };
    Panel.prototype.menu = function () { return new Menu(this, "grid"); };
    Panel.prototype.list = function () { return new Menu(this, "list"); };
    Panel.prototype.table = function () { return new Table(this); };
    Panel.prototype.grid = function () { return new Grid(this); };
    Panel.prototype.progress = function () { return new Progress(this); };

    function W(panel, z) { this.panel = panel; this._x = 0; this._y = 0; this._fg = null; this._bold = false; this._dim = false; panel._reg(z || 1, this._draw.bind(this)); }
    W.prototype.at = function (x, y) { this._x = x; this._y = y == null ? 0 : y; return this; };
    W.prototype.fg = function (c) { this._fg = c; return this; };
    W.prototype.bold = function () { this._bold = true; return this; };
    W.prototype.dim = function () { this._dim = true; return this; };
    W.prototype.abs = function (localX, width) { var c = this.panel.content(); return { x: c.x + Pico.resolveX(localX, width || 0, c.w), y: c.y + this._y, c: c }; };
    W.prototype._st = function (extra) { var st = style(this._fg || "fg", this._bold, this._dim); for (var k in (extra || {})) st[k] = extra[k]; return st; };
    W.prototype._draw = function () {};

    function Text(panel, v) { W.call(this, panel, 1); this._v = v; }
    Text.prototype = Object.create(W.prototype); Text.prototype.constructor = Text;
    Text.prototype._draw = function (s) { var str = asText(this._v); var p = this.abs(this._x, str.length); s.text(p.x, p.y, str, this._st()); };

    function Gauge(panel) { W.call(this, panel, 1); this._label = ""; this._val = 0; this._max = 100; this._w = 10; this._pct = false; }
    Gauge.prototype = Object.create(W.prototype); Gauge.prototype.constructor = Gauge;
    Gauge.prototype.label = function (v) { this._label = v; return this; };
    Gauge.prototype.value = function (v) { this._val = v; return this; };
    Gauge.prototype.max = function (v) { this._max = v; return this; };
    Gauge.prototype.width = function (v) { this._w = v; return this; };
    Gauge.prototype.style = function () { return this; };
    Gauge.prototype.showPct = function () { this._pct = true; return this; };
    Gauge.prototype._draw = function (s) {
      var max = Number(Pico.resolve(this._max) || 1);
      var val = Pico.clamp(Number(Pico.resolve(this._val) || 0), 0, max);
      var frac = max ? val / max : 0;
      var fill = Math.round(frac * this._w);
      var bar = Pico.repeat("█", fill) + Pico.repeat("░", this._w - fill);
      var label = this._label ? String(this._label).substr(0, 4).padEnd ? String(this._label).substr(0, 4).padEnd(4) + " " : String(this._label) + " " : "";
      while (this._label && label.length < 5) label += " ";
      var pct = this._pct ? " " + Pico.pad(Math.round(frac * 100), 2, " ") + "%" : "";
      var p = this.abs(this._x, label.length + bar.length + pct.length);
      s.text(p.x, p.y, label + bar + pct, this._st({ fg: this._fg || "green" }));
    };

    function Spark(panel) { W.call(this, panel, 1); this._label = ""; this._data = []; this._min = 0; this._max = 100; this._glyphs = "▁▂▃▄▅▆▇█"; }
    Spark.prototype = Object.create(W.prototype); Spark.prototype.constructor = Spark;
    Spark.prototype.label = function (v) { this._label = v; return this; };
    Spark.prototype.data = function (v) { this._data = v; return this; };
    Spark.prototype.range = function (a, b) { this._min = a; this._max = b; return this; };
    Spark.prototype.glyphs = function (v) { this._glyphs = v; return this; };
    Spark.prototype._draw = function (s) {
      var d = Pico.resolve(this._data) || [], g = this._glyphs, out = "";
      for (var i = 0; i < d.length; i++) { var f = Pico.clamp((d[i] - this._min) / (this._max - this._min || 1), 0, 1); out += g.charAt(Math.min(g.length - 1, Math.round(f * (g.length - 1)))); }
      var label = this._label ? this._label + " " : "";
      var p = this.abs(this._x, label.length + out.length);
      s.text(p.x, p.y, label + out, this._st({ fg: this._fg || "cyan" }));
    };

    function Menu(panel, kind) { W.call(this, panel, 1); this._kind = kind; this._items = []; this._grid = 1; this._sel = 0; this._marker = "›"; this._accent = "cyan"; this._onPick = function () {}; panel.app._focusables.push(this); }
    Menu.prototype = Object.create(W.prototype); Menu.prototype.constructor = Menu;
    Menu.prototype.grid = function (n) { this._grid = Math.max(1, n); return this; };
    Menu.prototype.items = function (v) { this._items = v; return this; };
    Menu.prototype.marker = function (v) { this._marker = v; return this; };
    Menu.prototype.accent = function (v) { this._accent = v; return this; };
    Menu.prototype.onPick = function (fn) { this._onPick = fn; return this; };
    Menu.prototype._list = function () { return Pico.resolve(this._items) || []; };
    Menu.prototype.move = function (tok) { var n = this._list().length; if (!n) return; if (tok === "↑") this._sel = Pico.clamp(this._sel - this._grid, 0, n - 1); if (tok === "↓") this._sel = Pico.clamp(this._sel + this._grid, 0, n - 1); if (tok === "←") this._sel = Pico.clamp(this._sel - 1, 0, n - 1); if (tok === "→") this._sel = Pico.clamp(this._sel + 1, 0, n - 1); };
    Menu.prototype.activate = function () { this._onPick(this._list()[this._sel]); };
    Menu.prototype._draw = function (s) {
      var items = this._list(), c = this.panel.content(), ox = c.x + this._x, oy = c.y + this._y;
      if (this._kind === "grid") {
        var colW = Math.max(1, Math.floor((c.w - this._x) / this._grid));
        for (var i = 0; i < items.length; i++) { var r = Math.floor(i / this._grid), ci = i % this._grid, sel = i === this._sel; s.text(ox + ci * colW, oy + r, (sel ? this._marker + " " : "  ") + String(items[i]), { fg: sel ? this._accent : "fg", bold: sel }); }
      } else {
        for (i = 0; i < items.length; i++) { sel = i === this._sel; s.text(ox, oy + i, (sel ? this._marker : " ") + " " + String(items[i]), { fg: sel ? this._accent : "fg", bold: sel }); }
      }
    };

    function Table(panel) { W.call(this, panel, 1); this._cols = []; this._rows = []; this._sel = 0; this._marker = "›"; panel.app._focusables.push(this); }
    Table.prototype = Object.create(W.prototype); Table.prototype.constructor = Table;
    Table.prototype.columns = function (v) { this._cols = v; return this; };
    Table.prototype.rows = function (v) { this._rows = v; return this; };
    Table.prototype.select = function (v) { this._sel = v; return this; };
    Table.prototype.marker = function (v) { this._marker = v; return this; };
    Table.prototype.sortBy = function () { return this; };
    Table.prototype.current = function () { return (Pico.resolve(this._rows) || [])[this._sel]; };
    Table.prototype.move = function (tok) { var rows = Pico.resolve(this._rows) || []; if (!rows.length) return; if (tok === "↑") this._sel = (this._sel - 1 + rows.length) % rows.length; if (tok === "↓") this._sel = (this._sel + 1) % rows.length; };
    Table.prototype._draw = function (s) { var rows = Pico.resolve(this._rows) || [], c = this.panel.content(), y = c.y + this._y; s.text(c.x + 2, y, this._cols.join(" "), { fg: "dim" }); for (var i = 0; i < rows.length && y + i + 1 < c.y + c.h; i++) { var line = ""; for (var j = 0; j < this._cols.length; j++) line += String(rows[i][this._cols[j]] == null ? "" : rows[i][this._cols[j]]) + (j + 1 < this._cols.length ? " " : ""); s.text(c.x, y + i + 1, (i === this._sel ? this._marker : " ") + " " + line, { fg: i === this._sel ? "cyan" : "fg", bold: i === this._sel }); } };

    function Grid(panel) { W.call(this, panel, 1); this._w = 10; this._h = 10; this._cell = "·"; this._layers = []; }
    Grid.prototype = Object.create(W.prototype); Grid.prototype.constructor = Grid;
    Grid.prototype.size = function (w, h) { this._w = w; this._h = h; return this; };
    Grid.prototype.cell = function (v) { this._cell = v; return this; };
    Grid.prototype.layer = function (name, fn, glyph) { this._layers.push({ name: name, fn: fn, glyph: glyph }); return this; };
    Grid.prototype.render = function () { return this; };
    Grid.prototype._draw = function (s) { var c = this.panel.content(), ox = c.x + this._x, oy = c.y + this._y, cw = this._cell.length; for (var y = 0; y < this._h; y++) s.text(ox, oy + y, Pico.repeat(this._cell, this._w), { fg: "dim" }); for (var l = 0; l < this._layers.length; l++) { var L = this._layers[l], cells = Pico.resolve(L.fn) || []; for (var i = 0; i < cells.length; i++) if (cells[i]) s.text(ox + cells[i].x * cw, oy + cells[i].y, L.glyph, { fg: L.name === "food" ? "amber" : "green", bold: true }); } };

    function Progress(panel) { W.call(this, panel, 1); this._val = 0; this._max = 100; this._w = null; }
    Progress.prototype = Object.create(W.prototype); Progress.prototype.constructor = Progress;
    Progress.prototype.value = function (v) { this._val = v; return this; };
    Progress.prototype.max = function (v) { this._max = v; return this; };
    Progress.prototype._draw = function (s) { var c = this.panel.content(), w = this._w || Math.max(1, c.w - this._x), max = Number(Pico.resolve(this._max) || 1), val = Pico.clamp(Number(Pico.resolve(this._val) || 0), 0, max), pos = Math.round((val / max) * (w - 1)), bar = ""; for (var i = 0; i < w; i++) bar += i === pos ? "●" : "─"; s.text(c.x + this._x, c.y + this._y, bar, this._st({ fg: this._fg || "green" })); };

    function drawStatusbar(s, v) {
      var y = screen.rows - 1;
      s.hline(0, y, screen.cols, " ", { fg: "dim" });
      if (typeof v === "function") {
        var b = { _l: "", _c: "", _r: "", left: function (x) { this._l = asText(x); return this; }, center: function (x) { this._c = asText(x); return this; }, right: function (x) { this._r = asText(x); return this; } };
        v(b);
        s.text(1, y, b._l, { fg: "amber" });
        s.text(Math.floor((screen.cols - b._c.length) / 2), y, b._c, { fg: "dim" });
        s.text(screen.cols - 1 - b._r.length, y, b._r, { fg: "dim" });
      } else s.text(1, y, asText(v), { fg: "dim" });
    }

    OS.app = function (name) { currentApp = new App(name); OS._app = currentApp; return currentApp; };
    OS._app = null;

    return {
      OS: OS,
      screen: screen,
      runFrame: function (dt) { OS._evolve(dt || 0); if (currentApp && currentApp._mounted && !currentApp._exited) currentApp._frame(dt || 0); },
      renderText: function (trimRight) { return screen.toText(trimRight); },
      sendKey: function (tok) { if (currentApp) currentApp._fireKey(tok); },
      getApp: function () { return currentApp; }
    };
  }

  Pico.createRuntime = createRuntime;
  return Pico;
})(globalThis);
