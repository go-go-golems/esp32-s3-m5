// Embedded console probes: the hardware-validation surface for the v2
// builder API (js probe N). Each probe exercises one area and prints
// deterministic evidence; probes 9/10 additionally render through the fake
// backend and dump the full op trace.
#include "app_js.h"
#include "app_js_internal.h"
#include "s3paper_runtime/runtime.h"

namespace pulp {
namespace {

// 1: factories + fluent methods + invert + page show (expect 13 ops).
const char kProbe1Js[] =
    "resetTree();\n"
    "var header = col().pad(48,40,8,40).gap(12)\n"
    "  .add(text('PULP').size('xl'),\n"
    "       text('V2 BUILDER PROBE').size('xs'),\n"
    "       divider(6,0));\n"
    "var content = col().pad(32,40,24,40).gap(18)\n"
    "  .add(text('native classes, ROM prototypes').size('sm'),\n"
    "       text(' INVERT CHIP ').size('sm').invert(),\n"
    "       row().gap(12).add(text('progress:').size('sm'),\n"
    "                         progressBar(640,24).height(24)),\n"
    "       text('centered').size('sm').center().gray(96));\n"
    "var footer = col().pad(6,40,12,40).gap(8)\n"
    "  .add(divider(2,0),\n"
    "       text('abi v' + abiVersion()).size('xs').gray(96));\n"
    "var p1 = page('probe1').header(header).content(content)\n"
    "  .footer(footer);\n"
    "p1.show(true);\n"
    "print('pulp screen: probe1');\n";

// 2: fault containment — stale handle, bad prop, arena exhaustion.
const char kProbe2Js[] =
    "resetTree();\n"
    "var w = text('victim');\n"
    "resetTree();\n"
    "var msg = 'MISSED';\n"
    "try { w.set('x'); } catch (e) { msg = e.message; }\n"
    "print('probe2: stale -> ' + msg);\n"
    "var bad = 'MISSED';\n"
    "try { text('x').font(99); } catch (e2) { bad = e2.message; }\n"
    "print('probe2: badfont -> ' + bad);\n"
    "var full = 'MISSED';\n"
    "try { var i; var arr = [];\n"
    "  for (i = 0; i < 200; i++) { arr.push(row()); } }\n"
    "catch (e3) { full = e3.message; }\n"
    "print('probe2: arena-full -> ' + full);\n";

// 3: onTap dispatch + page gesture handler + diff update on tap.
const char kProbe3Js[] =
    "resetTree();\n"
    "var taps = 0;\n"
    "var counter = text('taps: 0').size('lg');\n"
    "var tapRow = row().pad(12,40,12,40).add(counter)\n"
    "  .onTap(function(k, x, y) {\n"
    "    taps = taps + 1;\n"
    "    print('probe3: tap ' + taps + ' at ' + x + ',' + y);\n"
    "    counter.set('taps: ' + taps);\n"
    "    p3.update();\n"
    "  });\n"
    "var p3 = page('probe3').content(col().pad(40,40,40,40).gap(20)\n"
    "  .add(text('TAP PROBE').size('lg'), tapRow));\n"
    "p3.on(G.DOWN, function() { print('probe3: swipe down'); });\n"
    "p3.show(true);\n"
    "print('pulp screen: probe3');\n";

// 4: dynamic text(fn) + page.every tick (expect one small damage rect/s).
const char kProbe4Js[] =
    "resetTree();\n"
    "var t0 = millis();\n"
    "var clock = text(function() {\n"
    "  return 'up ' + Math.floor((millis() - t0) / 1000) + 's';\n"
    "}).size('xl');\n"
    "var p4 = page('probe4').content(col().pad(60,40,40,40).gap(24)\n"
    "  .add(text('TICK PROBE').size('lg'), clock)).every(1000);\n"
    "p4.show(true);\n"
    "print('pulp screen: probe4');\n";

// 5: native services (battery, store round-trip, library).
const char kProbe5Js[] =
    "print('probe5: battery=' + batteryLevel());\n"
    "storeSet('probe', 42);\n"
    "print('probe5: store=' + storeGet('probe', -1));\n"
    "print('probe5: books=' + libraryCount());\n"
    "var i;\n"
    "for (i = 0; i < libraryCount(); i++) {\n"
    "  print('probe5: ' + libraryLine(i));\n"
    "}\n";

// 6-8: narrowing variants from the display-face MeasureText regression
// (a row-nested lg text once vanished: text.cpp gated measurement on the
// bitmap-fallback table and rejected TTF-only font ids).
const char kProbe6Js[] =
    "resetTree();\n"
    "var counter = text('taps: 0').size('lg');\n"
    "var tapRow = row().pad(12,40,12,40).add(counter);\n"
    "var p6 = page('probe6').content(col().pad(40,40,40,40).gap(20)\n"
    "  .add(text('TAP PROBE').size('lg'), tapRow));\n"
    "p6.show(true);\n"
    "print('pulp screen: probe6');\n";

const char kProbe7Js[] =
    "resetTree();\n"
    "var counter = text('taps: 0').size('lg');\n"
    "var tapRow = row().pad(12,40,12,40).add(counter)\n"
    "  .onTap(function(k, x, y) {});\n"
    "var p7 = page('probe7').content(col().pad(40,40,40,40).gap(20)\n"
    "  .add(text('TAP PROBE').size('lg'), tapRow));\n"
    "p7.show(true);\n"
    "print('pulp screen: probe7');\n";

const char kProbe8Js[] =
    "resetTree();\n"
    "var tapRow = row().pad(12,40,12,40)\n"
    "  .add(text('taps: 0').size('lg'))\n"
    "  .onTap(function(k, x, y) {});\n"
    "var p8 = page('probe8').content(col().pad(40,40,40,40).gap(20)\n"
    "  .add(text('TAP PROBE').size('lg'), tapRow));\n"
    "p8.show(true);\n"
    "print('pulp screen: probe8');\n";

// 10: row-variant matrix (all five texts must appear in the trace).
const char kProbe10Js[] =
    "resetTree();\n"
    "var p = page('probe10').content(col().pad(40,40,40,40).gap(20)\n"
    "  .add(row().add(text('AAA')),\n"
    "       row().pad(12,40,12,40).add(text('BBB')),\n"
    "       col().add(text('CCC')),\n"
    "       row().gap(12).add(text('DDD'), text('EEE'))));\n"
    "p.show(true);\n"
    "print('pulp screen: probe10');\n";

StatusCode RunTraced(const char *code, const char *name) {
    s3paper_runtime::SetTracePresent(true);
    const StatusCode ran = jsi::EvalBounded(code, 3000, name);
    s3paper_runtime::SetTracePresent(false);
    s3paper_runtime::PrintFakeTrace();
    return ran;
}

}  // namespace

StatusCode JsRunProbe(uint32_t which) {
    const StatusCode init = JsInit();
    if (init != StatusCode::Ok) {
        return init;
    }
    switch (which) {
        case 1: return jsi::EvalBounded(kProbe1Js, 3000, "<probe1>");
        case 2: return jsi::EvalBounded(kProbe2Js, 3000, "<probe2>");
        case 3: return jsi::EvalBounded(kProbe3Js, 3000, "<probe3>");
        case 4: return jsi::EvalBounded(kProbe4Js, 3000, "<probe4>");
        case 5: return jsi::EvalBounded(kProbe5Js, 3000, "<probe5>");
        case 6: return jsi::EvalBounded(kProbe6Js, 3000, "<probe6>");
        case 7: return jsi::EvalBounded(kProbe7Js, 3000, "<probe7>");
        case 8: return jsi::EvalBounded(kProbe8Js, 3000, "<probe8>");
        case 9: return RunTraced(kProbe6Js, "<probe9>");
        case 10: return RunTraced(kProbe10Js, "<probe10>");
        default: return StatusCode::InvalidArgument;
    }
}

}  // namespace pulp
