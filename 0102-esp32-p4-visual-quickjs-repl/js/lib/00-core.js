// picoOS portable core helpers for desktop/device QuickJS scripts.
// Required globals: print, millis, gc (tests use print; helpers are pure).
var Pico = (function (root) {
  var Pico = root.Pico || {};

  function fail(message) {
    throw new Error(String(message || "assertion failed"));
  }

  function assert(value, message) {
    if (!value) fail(message);
  }

  function assertEqual(name, actual, expected) {
    if (actual !== expected) {
      fail(name + ": expected " + expected + ", got " + actual);
    }
  }

  function assertContains(name, text, needle) {
    text = String(text);
    needle = String(needle);
    if (text.indexOf(needle) < 0) {
      fail(name + ": expected to contain " + needle);
    }
  }

  function runTest(name, fn) {
    try {
      fn();
      print("PASS", name);
    } catch (e) {
      print("FAIL", name, e && e.message ? e.message : e);
      throw e;
    }
  }

  function clamp(value, min, max) {
    value = Number(value);
    if (value < min) return min;
    if (value > max) return max;
    return value;
  }

  function pad(value, width, fill) {
    var s = String(value);
    fill = fill == null ? "0" : String(fill);
    while (s.length < width) s = fill + s;
    return s;
  }

  function repeat(ch, count) {
    var out = "";
    for (var i = 0; i < count; i++) out += ch;
    return out;
  }

  function resolve(value) {
    if (typeof value === "function") return value();
    return value;
  }

  function resolveX(x, width, containerWidth) {
    if (x === "right") return containerWidth - width;
    if (x === "center") return Math.floor((containerWidth - width) / 2);
    var n = Number(x || 0);
    if (!isFinite(n)) return 0;
    return Math.floor(n);
  }

  function Lcg(seed) {
    this.state = (seed == null ? 1 : seed) >>> 0;
    if (this.state === 0) this.state = 1;
  }
  Lcg.prototype.nextInt = function () {
    this.state = (Math.imul(1664525, this.state) + 1013904223) >>> 0;
    return this.state;
  };
  Lcg.prototype.next = function () {
    return this.nextInt() / 4294967296;
  };
  Lcg.prototype.range = function (min, max) {
    return min + this.next() * (max - min);
  };
  Lcg.prototype.int = function (min, max) {
    return Math.floor(this.range(min, max + 1));
  };

  Pico.fail = fail;
  Pico.assert = assert;
  Pico.assertEqual = assertEqual;
  Pico.assertContains = assertContains;
  Pico.runTest = runTest;
  Pico.clamp = clamp;
  Pico.pad = pad;
  Pico.repeat = repeat;
  Pico.resolve = resolve;
  Pico.resolveX = resolveX;
  Pico.Lcg = Lcg;
  return Pico;
})(globalThis);
