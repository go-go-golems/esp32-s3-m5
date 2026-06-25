// Purpose: dashboard example with layout, title, gauge, menu, and statusbar.
// Expected output: 40-column snapshot containing "picoOS", "batt", and app names.
// Assumptions: run with host-shim.js and js/lib/*.js preloaded.
// Required globals: print, millis, gc.

var rt = Pico.createRuntime({ cols: 40, rows: 30, seed: 2 });
var OS = rt.OS;
var home = OS.app("home");

home.layout(function (l) { l.row(1, "bar").row("*", "body"); });
home.panel("bar").frame("rounded").title(" picoOS ").titleRight(function () { return OS.clock("HH:mm"); });

var body = home.panel("body").frame("rounded");
body.text("sunny 72F").at(2, 1).fg("amber");
body.gauge().at(14, 1).label("batt").value(function () { return OS.battery; }).width(8).showPct();
body.text("apps").at(2, 3).bold().fg("white");
body.menu().at(2, 5).grid(3)
  .items(["term", "notes", "files", "music", "sysmon", "snake"])
  .marker("›").accent("cyan")
  .onPick(function (name) { OS.launch(name); });
body.text(function () { return OS.toast; }).at(2, 9).fg("dim");

home.statusbar("arrows select · enter open");
home.mount();
rt.runFrame(16);
rt.sendKey("→");
rt.sendKey("⏎");
rt.runFrame(16);
print(rt.renderText());
