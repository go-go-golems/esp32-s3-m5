// Purpose: self-test the portable picoOS core helpers and screen buffer.
// Expected output: PASS lines followed by API TESTS PASS.
// Required globals: print, millis, gc.

Pico.runTest("core pad clamp rng", function () {
  Pico.assertEqual("pad", Pico.pad(7, 3), "007");
  Pico.assertEqual("clamp low", Pico.clamp(-1, 0, 9), 0);
  Pico.assertEqual("clamp high", Pico.clamp(12, 0, 9), 9);
  var a = new Pico.Lcg(123);
  var b = new Pico.Lcg(123);
  Pico.assertEqual("rng deterministic", a.nextInt(), b.nextInt());
});

Pico.runTest("screen text and clipping", function () {
  var s = Pico.makeScreen(10, 4);
  s.text(1, 1, "hello", { fg: "green" });
  s.text(8, 1, "abcd");
  s.text(-3, 2, "xy");
  Pico.assertEqual("cell h", s.get(1, 1).ch, "h");
  Pico.assertEqual("cell clipped a", s.get(8, 1).ch, "a");
  Pico.assertEqual("cell clipped b", s.get(9, 1).ch, "b");
  Pico.assertEqual("offscreen read", s.get(-1, -1).ch, " ");
});

Pico.runTest("screen boxes", function () {
  var s = Pico.makeScreen(12, 5);
  s.box(0, 0, 12, 5, "rounded");
  s.text(2, 2, "pico");
  var out = s.toText();
  Pico.assertContains("top border", out, "╭──────────╮");
  Pico.assertContains("payload", out, "pico");
  Pico.assertEqual("bottom left", s.get(0, 4).ch, "╰");
  Pico.assertEqual("bottom right", s.get(11, 4).ch, "╯");
});

Pico.runTest("screen row widths", function () {
  var s = Pico.makeScreen(40, 30);
  s.box(0, 0, 40, 30, "single");
  s.text(2, 2, "40-column portable snapshot");
  var lines = s.toLines(false);
  Pico.assertEqual("row count", lines.length, 30);
  for (var i = 0; i < lines.length; i++) {
    Pico.assertEqual("row width " + i, lines[i].length, 40);
  }
});

print("API TESTS PASS", millis());
