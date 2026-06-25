// Purpose: self-test the picoOS fluent App/Panel/widget runtime.
// Expected output: PASS lines followed by UI RUNTIME TESTS PASS.
// Required globals: print, millis, gc.

Pico.runTest("runtime renders hello panel", function () {
  var rt = Pico.createRuntime({ cols: 40, rows: 30, seed: 1 });
  var OS = rt.OS;
  var app = OS.app("hello");
  var st = app.state({ n: 0, last: "" });
  var p = app.panel("main").frame("rounded").title(" hello ");
  p.text("picoOS DSL").at("center", 2).bold().fg("cyan");
  p.text(function () { return "ticks: " + st.n; }).at("center", 4);
  app.on("tick", 1000, function () { st.n++; });
  app.key("a", function () { st.last = "a"; });
  app.statusbar("a working starter").mount();
  rt.runFrame(16);
  Pico.assertContains("title", rt.renderText(), "hello");
  Pico.assertContains("text", rt.renderText(), "picoOS DSL");
  rt.runFrame(1000);
  Pico.assertContains("timer", rt.renderText(), "ticks: 1");
  rt.sendKey("a");
  Pico.assertEqual("key", st.last, "a");
});

Pico.runTest("runtime layout gauge menu", function () {
  var rt = Pico.createRuntime({ cols: 40, rows: 20, seed: 1 });
  var OS = rt.OS;
  var picked = "";
  var app = OS.app("home");
  app.layout(function (l) { l.row(1, "bar").row("*", "body"); });
  app.panel("bar").frame("rounded").title(" picoOS ").titleRight(function () { return OS.clock("HH:mm"); });
  var body = app.panel("body").frame("rounded");
  body.gauge().at(2, 1).label("batt").value(function () { return OS.battery; }).width(8).showPct();
  body.menu().at(2, 3).grid(3).items(["term", "notes", "files"]).onPick(function (name) { picked = name; });
  app.mount();
  rt.runFrame(16);
  Pico.assertContains("gauge", rt.renderText(), "batt");
  rt.sendKey("→");
  rt.sendKey("⏎");
  Pico.assertEqual("picked", picked, "notes");
});

Pico.runTest("runtime grid follows OS snake", function () {
  var rt = Pico.createRuntime({ cols: 40, rows: 20, seed: 1 });
  var OS = rt.OS;
  OS.reset();
  var app = OS.app("snake");
  var board = app.panel("board").frame("rounded").title(" snake ");
  board.grid().at(1, 1).size(19, 9).cell("· ")
    .layer("body", function () { return OS.snake.cells; }, "█")
    .layer("head", function () { return [OS.snake.head]; }, "○")
    .layer("food", function () { return [OS.food]; }, "◆");
  app.key("↑↓←→", function (m, k) { OS.turn(k); });
  app.loop(8, function () { OS.step(); });
  app.mount();
  rt.runFrame(130);
  Pico.assertContains("snake title", rt.renderText(), "snake");
  Pico.assertContains("snake head", rt.renderText(), "○");
  rt.sendKey("↓");
  rt.runFrame(130);
  Pico.assert("snake alive", !OS.dead);
});

print("UI RUNTIME TESTS PASS", millis());
