// Purpose: smallest portable QuickJS script for desktop/device smoke testing.
// Expected output includes "SMOKE PASS" and arithmetic result 3.
// Required globals: print, millis, gc.

function assertEqual(name, actual, expected) {
  if (actual !== expected) {
    throw new Error(name + ": expected " + expected + ", got " + actual);
  }
  print("PASS", name);
}

function main() {
  const start = millis();
  print("PicoCalc QuickJS smoke start", start);
  assertEqual("arithmetic", 1 + 2, 3);
  assertEqual("array", ["a", "b", "c"].join(""), "abc");
  assertEqual("object", ({ answer: 42 }).answer, 42);
  gc();
  print("SMOKE PASS", millis() - start);
}

main();
