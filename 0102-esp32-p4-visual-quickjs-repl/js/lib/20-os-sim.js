// picoOS deterministic device/OS simulation for portable QuickJS examples.
// No browser, Node, module, network, filesystem, std/os, or eval dependency.
var Pico = (function (root) {
  var Pico = root.Pico || {};

  function cloneRow(obj) {
    var out = {};
    for (var k in obj) if (Object.prototype.hasOwnProperty.call(obj, k)) out[k] = obj[k];
    return out;
  }

  function pushHist(hist, key, value, max) {
    if (!hist[key]) hist[key] = [];
    hist[key].push(value);
    while (hist[key].length > max) hist[key].shift();
  }

  function formatClock(totalSeconds, fmt) {
    totalSeconds = Math.floor(totalSeconds) % 86400;
    if (totalSeconds < 0) totalSeconds += 86400;
    var h = Math.floor(totalSeconds / 3600);
    var m = Math.floor((totalSeconds % 3600) / 60);
    var s = totalSeconds % 60;
    return String(fmt || "HH:mm")
      .replace("HH", Pico.pad(h, 2))
      .replace("mm", Pico.pad(m, 2))
      .replace("ss", Pico.pad(s, 2));
  }

  function ExprParser(source) {
    this.s = String(source == null ? "" : source)
      .replace(/×/g, "*")
      .replace(/÷/g, "/")
      .replace(/π/g, "pi")
      .replace(/√/g, "sqrt");
    this.i = 0;
  }
  ExprParser.prototype.peek = function () { return this.s.charAt(this.i); };
  ExprParser.prototype.skip = function () { while (/\s/.test(this.peek())) this.i++; };
  ExprParser.prototype.eat = function (ch) {
    this.skip();
    if (this.s.substr(this.i, ch.length) === ch) { this.i += ch.length; return true; }
    return false;
  };
  ExprParser.prototype.number = function () {
    this.skip();
    var start = this.i;
    while (/[0-9.]/.test(this.peek())) this.i++;
    if (start === this.i) return null;
    var n = Number(this.s.slice(start, this.i));
    if (!isFinite(n)) throw new Error("bad number");
    return n;
  };
  ExprParser.prototype.ident = function () {
    this.skip();
    var start = this.i;
    while (/[A-Za-z_]/.test(this.peek())) this.i++;
    return this.s.slice(start, this.i);
  };
  ExprParser.prototype.primary = function () {
    this.skip();
    if (this.eat("+")) return this.primary();
    if (this.eat("-")) return -this.primary();
    if (this.eat("(")) {
      var inner = this.expr();
      if (!this.eat(")")) throw new Error("missing )");
      return inner;
    }
    var n = this.number();
    if (n !== null) return n;
    var id = this.ident();
    if (id) {
      if (id === "pi") return Math.PI;
      if (id === "e") return Math.E;
      var arg;
      if (!this.eat("(")) throw new Error("expected ( after " + id);
      arg = this.expr();
      if (!this.eat(")")) throw new Error("missing ) after " + id);
      if (id === "sin") return Math.sin(arg * Math.PI / 180);
      if (id === "cos") return Math.cos(arg * Math.PI / 180);
      if (id === "tan") return Math.tan(arg * Math.PI / 180);
      if (id === "sqrt") return Math.sqrt(arg);
      if (id === "log") return Math.log(arg) / Math.LN10;
      if (id === "ln") return Math.log(arg);
      if (id === "abs") return Math.abs(arg);
      if (id === "round") return Math.round(arg);
      throw new Error("unknown function " + id);
    }
    throw new Error("unexpected token at " + this.i);
  };
  ExprParser.prototype.power = function () {
    var left = this.primary();
    this.skip();
    if (this.eat("**") || this.eat("^")) {
      return Math.pow(left, this.power());
    }
    return left;
  };
  ExprParser.prototype.term = function () {
    var v = this.power();
    while (true) {
      if (this.eat("*")) v *= this.power();
      else if (this.eat("/")) v /= this.power();
      else return v;
    }
  };
  ExprParser.prototype.expr = function () {
    var v = this.term();
    while (true) {
      if (this.eat("+")) v += this.term();
      else if (this.eat("-")) v -= this.term();
      else return v;
    }
  };
  ExprParser.prototype.parse = function () {
    var v = this.expr();
    this.skip();
    if (this.i < this.s.length) throw new Error("trailing input at " + this.i);
    return v;
  };

  function createOS(options) {
    options = options || {};
    var rng = new Pico.Lcg(options.seed || 1);
    var elapsed = 0;
    var tickAcc = 0;
    var toast = "";
    var baseSeconds = options.baseSeconds == null ? (14 * 3600 + 32 * 60) : options.baseSeconds;
    var hist = { cpu: [], mem: [], tmp: [], load: [] };
    var procs = [
      { pid: 1, name: "kernel", cpu: 2, mem: 18 },
      { pid: 7, name: "ui", cpu: 11, mem: 42 },
      { pid: 12, name: "music", cpu: 48, mem: 31 },
      { pid: 19, name: "netd", cpu: 4, mem: 9 },
      { pid: 23, name: "shell", cpu: 1, mem: 6 }
    ];
    for (var i = 0; i < 64; i++) {
      pushHist(hist, "cpu", 30 + rng.int(0, 39), 64);
      pushHist(hist, "mem", 35 + rng.int(0, 19), 64);
      pushHist(hist, "tmp", 42 + rng.int(0, 9), 64);
      pushHist(hist, "load", 30 + rng.int(0, 39), 64);
    }

    var tracks = [
      { title: "Cosmic Drift", artist: "Nebula Theory", album: "Parallax", length: 252, liked: true },
      { title: "Solder Joints", artist: "Flux Core", album: "Reflow", length: 198, liked: false },
      { title: "Baud Rate", artist: "115200", album: "Serial", length: 221, liked: true }
    ];
    var player = { idx: 0, pos: 107, playing: true, vol: 70, shuffle: false };

    var room = {
      online: 4,
      messages: [
        { user: "ada", text: "got it booting", mine: false, t: -90 },
        { user: "you", text: "flashed yours?", mine: true, t: -60 },
        { user: "lin", text: "check gauges", mine: false, t: -8 }
      ]
    };
    var userColors = { ada: "cyan", lin: "amber", you: "green", flux: "mag" };

    var FS = {
      "/home/user": [
        { name: "projects/", dir: true, size: "" },
        { name: "music/", dir: true, size: "" },
        { name: "readme.md", dir: false, size: "1k", body: "# picoOS\na tiny text OS." },
        { name: "notes.txt", dir: false, size: "4k", body: "shopping\n- oat milk\n- coffee" },
        { name: "todo.md", dir: false, size: "812", body: "- ship the DSL\n- sleep" }
      ]
    };
    var cwd = "/home/user";
    var selectedFile = FS[cwd][2];
    var cfg = { bright: 80, theme: "amber", font: "6x8", haptics: true, echo: false, sleep: "2 min" };

    var SNK = { w: 19, h: 9, body: [], dir: { x: 1, y: 0 }, food: { x: 12, y: 3 }, dead: false, ate: false };
    function resetSnake() {
      SNK.body = [{ x: 4, y: 3 }, { x: 5, y: 3 }, { x: 6, y: 3 }];
      SNK.dir = { x: 1, y: 0 };
      SNK.food = { x: 12, y: 3 };
      SNK.dead = false;
      SNK.ate = false;
    }
    function placeFood() {
      var p, tries = 0;
      do {
        p = { x: rng.int(0, SNK.w - 1), y: rng.int(0, SNK.h - 1) };
        tries++;
      } while (tries < 200 && SNK.body.some(function (s) { return s.x === p.x && s.y === p.y; }));
      SNK.food = p;
    }
    resetSnake();

    var OS = {
      battery: 64,
      metrics: { cpu: 62, mem: 41, tmp: 48 },
      history: function (key, n) { return (hist[key] || []).slice(-n); },
      processes: function () { return procs.map(cloneRow); },
      clock: function (fmt) { return formatClock(baseSeconds + elapsed / 1000, fmt || "HH:mm"); },
      launch: function (name) { toast = "launch -> " + name; },
      get toast() { return toast; },

      get library() { return { current: tracks[player.idx] }; },
      get position() { return player.pos; },
      get volume() { return player.vol; },
      set volume(v) { player.vol = Pico.clamp(v, 0, 100); },
      get mode() { return { get shuffle() { return player.shuffle; }, set shuffle(v) { player.shuffle = !!v; } }; },
      next: function () { player.idx = (player.idx + 1) % tracks.length; player.pos = 0; },
      prev: function () { player.idx = (player.idx - 1 + tracks.length) % tracks.length; player.pos = 0; },
      set playing(v) { player.playing = !!v; },
      get playing() { return player.playing; },
      fft: function (n) {
        var out = [];
        for (var i = 0; i < n; i++) out.push(player.playing ? rng.next() : 0.05);
        return out;
      },

      get snake() { return { cells: SNK.body.slice(), head: SNK.body[SNK.body.length - 1] }; },
      get food() { return { x: SNK.food.x, y: SNK.food.y }; },
      get ate() { return SNK.ate; },
      get dead() { return SNK.dead; },
      turn: function (tok) {
        var map = { "↑": { x: 0, y: -1 }, "↓": { x: 0, y: 1 }, "←": { x: -1, y: 0 }, "→": { x: 1, y: 0 } };
        var d = map[tok];
        if (!d) return;
        if (d.x === -SNK.dir.x && d.y === -SNK.dir.y) return;
        SNK.dir = d;
      },
      step: function () {
        SNK.ate = false;
        if (SNK.dead) return;
        var head = SNK.body[SNK.body.length - 1];
        var nx = head.x + SNK.dir.x, ny = head.y + SNK.dir.y;
        if (nx < 0 || ny < 0 || nx >= SNK.w || ny >= SNK.h || SNK.body.some(function (s) { return s.x === nx && s.y === ny; })) {
          SNK.dead = true;
          return;
        }
        SNK.body.push({ x: nx, y: ny });
        if (nx === SNK.food.x && ny === SNK.food.y) { SNK.ate = true; placeFood(); }
        else SNK.body.shift();
      },
      reset: resetSnake,

      get room() { return room; },
      send: function (text) { text = String(text || "").replace(/^\s+|\s+$/g, ""); if (text) room.messages.push({ user: "you", text: text, mine: true, t: 0 }); },
      colorOf: function (u) { return userColors[u] || "white"; },

      get cwd() { return cwd; },
      ls: function (p) { return (FS[p] || []).map(cloneRow); },
      cd: function (f) { var next = cwd + "/" + String(f.name || "").replace(/\/$/, ""); if (FS[next]) cwd = next; },
      get selected() { return selectedFile; },
      select: function (f) { if (f && !f.dir) selectedFile = f; },
      cfg: cfg,

      eval: function (expr) { return new ExprParser(expr).parse(); },
      _evolve: function (dt) {
        dt = Math.max(0, Number(dt || 0));
        elapsed += dt;
        tickAcc += dt;
        if (tickAcc >= 250) {
          tickAcc = 0;
          OS.metrics.cpu = Math.round(Pico.clamp(OS.metrics.cpu + rng.range(-13, 13), 8, 96));
          OS.metrics.mem = Math.round(Pico.clamp(OS.metrics.mem + rng.range(-5, 5), 20, 88));
          OS.metrics.tmp = Math.round(Pico.clamp(OS.metrics.tmp + rng.range(-3, 3), 38, 64));
          OS.battery = Pico.clamp(OS.battery - 0.05, 0, 100);
          pushHist(hist, "cpu", OS.metrics.cpu, 64);
          pushHist(hist, "mem", OS.metrics.mem, 64);
          pushHist(hist, "tmp", OS.metrics.tmp, 64);
          pushHist(hist, "load", OS.metrics.cpu, 64);
          for (var i = 0; i < procs.length; i++) procs[i].cpu = Math.round(Pico.clamp(procs[i].cpu + rng.range(-6, 6), 0, 99));
        }
        if (player.playing) {
          player.pos += dt / 1000;
          if (player.pos >= tracks[player.idx].length) OS.next();
        }
      }
    };
    return OS;
  }

  Pico.createOS = createOS;
  Pico._ExprParser = ExprParser;
  return Pico;
})(globalThis);
