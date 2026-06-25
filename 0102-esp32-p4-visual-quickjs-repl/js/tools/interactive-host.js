// Interactive desktop host for portable picoOS examples.
// Host-only: run with qjs --std. Example/runtime scripts remain device-portable.
(function () {
  if (typeof std === "undefined") {
    throw new Error("interactive-host.js requires qjs --std for stdin");
  }

  var jsDir = scriptArgs[1] || "0102-esp32-p4-visual-quickjs-repl/js";
  var initial = scriptArgs[2] || "hello-api";
  var current = null;

  function puts(s) { std.out.puts(String(s) + "\n"); }
  function trim(s) { return String(s == null ? "" : s).replace(/^\s+|\s+$/g, ""); }
  function examplePath(name) {
    if (name.indexOf("/") >= 0) return name;
    return jsDir + "/examples/" + name + ".js";
  }
  function examples() { return ["hello-api", "dashboard", "sysmon", "snake", "calc"]; }
  function normalizeKey(tok) {
    var map = {
      up: "↑", down: "↓", left: "←", right: "→",
      enter: "⏎", return: "⏎", backspace: "⌫", bs: "⌫",
      esc: "esc", escape: "esc", space: " "
    };
    return map[tok] || tok;
  }
  function setCurrentFromGlobals(name) {
    if (typeof rt === "undefined" || !rt || !rt.renderText) {
      throw new Error("example did not create global rt");
    }
    current = { name: name, rt: rt };
  }
  function loadExample(name) {
    name = name || "hello-api";
    puts("\nloading " + name + " ...");
    var savedPrint = globalThis.print;
    globalThis.print = function () {}; // examples print snapshots; host renders explicitly
    try {
      std.loadScript(examplePath(name));
    } finally {
      globalThis.print = savedPrint;
    }
    setCurrentFromGlobals(name);
    puts("loaded " + name + " (commands: help, show, key, frame, run, open, quit)");
  }
  function show() {
    if (!current) { puts("no example loaded"); return; }
    puts("\n--- " + current.name + " ---");
    puts(current.rt.renderText());
  }
  function frame(ms) {
    if (!current) { puts("no example loaded"); return; }
    current.rt.runFrame(ms == null ? 100 : Number(ms));
  }
  function key(tok) {
    if (!current) { puts("no example loaded"); return; }
    current.rt.sendKey(normalizeKey(tok));
  }
  function help() {
    puts("commands:");
    puts("  help                 show this help");
    puts("  examples             list bundled examples");
    puts("  open <name>          load hello-api/dashboard/sysmon/snake/calc");
    puts("  show                 render the current 40-column screen");
    puts("  key <token>          send key: up/down/left/right/enter/space/a/...");
    puts("  frame [ms]           advance simulated time, default 100 ms");
    puts("  run [n] [ms]         advance n frames, default 10 frames of 100 ms");
    puts("  quit                 exit");
  }
  function dispatch(line) {
    line = trim(line);
    if (!line) return true;
    var parts = line.split(/\s+/);
    var cmd = parts[0];
    if (cmd === "help" || cmd === "?") help();
    else if (cmd === "examples") puts(examples().join("\n"));
    else if (cmd === "open") { loadExample(parts[1] || "hello-api"); show(); }
    else if (cmd === "show") show();
    else if (cmd === "key") { key(parts.slice(1).join(" ")); show(); }
    else if (cmd === "frame" || cmd === "f") { frame(parts[1] || 100); show(); }
    else if (cmd === "run") {
      var n = Number(parts[1] || 10), ms = Number(parts[2] || 100);
      for (var i = 0; i < n; i++) frame(ms);
      show();
    } else if (cmd === "quit" || cmd === "q" || cmd === "exit") return false;
    else puts("unknown command: " + cmd + " (try: help)");
    return true;
  }

  puts("picoOS QuickJS interactive host");
  puts("host-only stdin uses qjs --std; loaded apps still use portable Pico APIs.");
  loadExample(initial);
  show();
  help();

  while (true) {
    std.out.puts("picoOS> ");
    var line = std.in.getline();
    if (line === null) break;
    try {
      if (!dispatch(line)) break;
    } catch (e) {
      puts("ERROR " + (e && e.message ? e.message : e));
    }
  }
})();
