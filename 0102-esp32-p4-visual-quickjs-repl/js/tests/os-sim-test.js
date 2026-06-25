// Purpose: self-test deterministic picoOS simulated device services.
// Expected output: PASS lines followed by OS SIM TESTS PASS.
// Required globals: print, millis, gc.

Pico.runTest("os clock and metrics evolve", function () {
  var OS = Pico.createOS({ seed: 7, baseSeconds: 12 * 3600 });
  Pico.assertEqual("clock start", OS.clock("HH:mm:ss"), "12:00:00");
  var before = OS.metrics.cpu;
  OS._evolve(1000);
  Pico.assertEqual("clock after", OS.clock("HH:mm:ss"), "12:00:01");
  Pico.assert("cpu changed or bounded", OS.metrics.cpu >= 8 && OS.metrics.cpu <= 96);
  Pico.assertEqual("history len", OS.history("load", 5).length, 5);
  Pico.assert("battery drains", OS.battery < 64);
  Pico.assert("before captured", before >= 0);
});

Pico.runTest("os processes are copied", function () {
  var OS = Pico.createOS({ seed: 1 });
  var p = OS.processes();
  p[0].name = "mutated";
  Pico.assertEqual("copy protects process", OS.processes()[0].name, "kernel");
});

Pico.runTest("os calculator parser", function () {
  var OS = Pico.createOS({ seed: 1 });
  Pico.assertEqual("arithmetic", OS.eval("1 + 2 * 3"), 7);
  Pico.assertEqual("power", OS.eval("2^3^2"), 512);
  var trig = Math.round(OS.eval("sin(45) × 2") * 100000000) / 100000000;
  Pico.assertEqual("degrees trig", trig, 1.41421356);
  Pico.assertEqual("sqrt/pi", Math.round(OS.eval("√(9) + round(π)") * 100), 600);
});

Pico.runTest("os snake and launch", function () {
  var OS = Pico.createOS({ seed: 2 });
  OS.launch("term");
  Pico.assertEqual("toast", OS.toast, "launch -> term");
  var h = OS.snake.head;
  OS.step();
  Pico.assertEqual("snake x", OS.snake.head.x, h.x + 1);
  OS.turn("↓");
  OS.step();
  Pico.assertEqual("snake y", OS.snake.head.y, h.y + 1);
});

Pico.runTest("os chat files music settings", function () {
  var OS = Pico.createOS({ seed: 3 });
  var count = OS.room.messages.length;
  OS.send("  hello  ");
  Pico.assertEqual("message appended", OS.room.messages.length, count + 1);
  Pico.assertEqual("color", OS.colorOf("ada"), "cyan");
  Pico.assertEqual("cwd", OS.cwd, "/home/user");
  Pico.assertEqual("ls first", OS.ls(OS.cwd)[0].name, "projects/");
  OS.select(OS.ls(OS.cwd)[3]);
  Pico.assertEqual("selected", OS.selected.name, "notes.txt");
  var title = OS.library.current.title;
  OS.next();
  Pico.assert("track changed", OS.library.current.title !== title);
  OS.volume = 200;
  Pico.assertEqual("volume clamp", OS.volume, 100);
  OS.cfg.theme = "green";
  Pico.assertEqual("cfg", OS.cfg.theme, "green");
});

print("OS SIM TESTS PASS", millis());
