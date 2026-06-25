// Purpose: minimal portable picoOS app using App/Panel/Text/tick/key APIs.
// Expected output: 40-column snapshot containing "picoOS DSL" and "ticks: 1".
// Assumptions: run with host-shim.js and js/lib/*.js preloaded.
// Required globals: print, millis, gc.

var rt = Pico.createRuntime({ cols: 40, rows: 30, seed: 1 });
var OS = rt.OS;
var app = OS.app("hello");
var st = app.state({ n: 0, last: "" });

var p = app.panel("main").frame("rounded").title(" hello ");
p.text("picoOS DSL").at("center", 2).bold().fg("cyan");
p.text(function () { return "ticks: " + st.n; }).at("center", 4).fg("white");
p.text(function () { return OS.clock("HH:mm:ss"); }).at("center", 6).fg("amber");
p.text("press a ->").at("center", 9).fg("dim");
p.text(function () { return st.last ? "you pressed: " + st.last : ""; }).at("center", 10);

app.on("tick", 1000, function () { st.n++; });
app.key("a", function () { st.last = "a"; });
app.statusbar("a working starter");
app.mount();

rt.runFrame(1000);
rt.sendKey("a");
rt.runFrame(16);
print(rt.renderText());
