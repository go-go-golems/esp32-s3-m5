// Purpose: calculator example using the safe tiny OS expression parser.
// Expected output: 40-column snapshot containing expression and 1.41421356.
// Assumptions: run with host-shim.js and js/lib/*.js preloaded.
// Required globals: print, millis, gc.

var rt = Pico.createRuntime({ cols: 40, rows: 20, seed: 5 });
var OS = rt.OS;
var calc = OS.app("calc");
var st = calc.state({ expr: "sin(45) × 2", result: 0 });

var p = calc.panel("main").frame("rounded").title(" calc ");
p.text(function () { return st.expr; }).at("right", 2).fg("dim");
p.text(function () { return String(st.result); }).at("right", 4).bold().fg("green");
p.text("parser: + - * / ^ sqrt pi").at(2, 7).fg("dim");

calc.compute(function () {
  var r = OS.eval(st.expr);
  st.result = Math.round(r * 100000000) / 100000000;
});
calc.statusbar("portable parser · no eval");
calc.mount();
rt.runFrame(16);
print(rt.renderText());
