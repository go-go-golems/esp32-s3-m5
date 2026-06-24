// Desktop shim for ESP32-P4 PicoCalc visual QuickJS scripts.
// Load before portable scripts when running with desktop qjs.
(function installHostShim(global) {
  if (typeof global.print !== "function") {
    global.print = function (...args) {
      const text = args.map(String).join(" ");
      if (typeof console !== "undefined" && console.log) {
        console.log(text);
      } else if (typeof std !== "undefined" && std.out) {
        std.out.puts(text + "\n");
      }
    };
  }

  const started = Date.now();
  if (typeof global.millis !== "function") {
    global.millis = function () {
      return Date.now() - started;
    };
  }

  if (typeof global.gc !== "function") {
    global.gc = function () {};
  }
})(globalThis);
