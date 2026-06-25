// Purpose: system monitor example with gauges, sparkline, and process table.
// Expected output: 40-column snapshot containing "sysmon", "cpu", and process rows.
// Assumptions: run with host-shim.js and js/lib/*.js preloaded.
// Required globals: print, millis, gc.

var rt = Pico.createRuntime({ cols: 40, rows: 30, seed: 3 });
var OS = rt.OS;
var mon = OS.app("sysmon");
var ui = mon.panel("main").frame("rounded").title(" sysmon ");

ui.gauge().at(2, 1).label("cpu").value(function () { return OS.metrics.cpu; }).width(18).showPct();
ui.gauge().at(2, 2).label("mem").value(function () { return OS.metrics.mem; }).width(18).showPct();
ui.gauge().at(2, 3).label("tmp").value(function () { return OS.metrics.tmp; }).max(80).width(18).showPct();
ui.spark().at(2, 5).label("load").data(function () { return OS.history("load", 26); }).range(0, 100);
ui.table().at(2, 7).columns(["pid", "name", "cpu", "mem"]).rows(function () { return OS.processes(); }).select(2).marker("›");

mon.statusbar("q quit · up/down select");
mon.key("q", function (m) { m.exit(); });
mon.mount();
rt.runFrame(500);
print(rt.renderText());
