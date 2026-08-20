// Embedded console probes: the hardware-validation surface for the v2
// builder API (js probe N). Each probe exercises one area and prints
// deterministic evidence; probes 9/10 additionally render through the fake
// backend and dump the full op trace.
#include "esp_timer.h"
#include <cstdio>
#include "app_js.h"
#include "app_js_internal.h"
#include "s3paper/text.h"
#include "esp_heap_caps.h"
#include <cstring>
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

// 11: canvas primitives on the panel (ESP-52).
const char kProbe11Js[] =
    "resetTree();\n"
    "var cv = canvas().height(700);\n"
    "cv.box(20, 20, 460, 660, 0, 3);\n"
    "cv.line(20, 20, 480, 680, 0, 2);\n"
    "cv.line(480, 20, 20, 680, 128, 1);\n"
    "cv.disc(250, 200, 80, 0);\n"
    "cv.ring(250, 460, 100, 0, 6);\n"
    "cv.ring(250, 460, 70, 128, 3);\n"
    "cv.paint(60, 560, 120, 80, 200);\n"
    "var p11 = page('probe11')\n"
    "  .header(col().pad(16,40,6,40).gap(8)\n"
    "    .add(text('CANVAS PROBE').size('lg'), divider(6,0)))\n"
    "  .content(col().pad(10,40,10,40).add(cv));\n"
    "p11.show(true);\n"
    "print('pulp screen: probe11');\n"
    "var bad = 'MISSED';\n"
    "try { text('x').line(0,0,1,1,0); } catch (e) { bad = e.message; }\n"
    "print('probe11: kindcheck -> ' + bad);\n"
    "var full = 'MISSED';\n"
    "try { var i; for (i = 0; i < 200; i++) {\n"
    "  cv.line(0, i, 480, i, 0); } }\n"
    "catch (e2) { full = e2.message; }\n"
    "print('probe11: capacity -> ' + full);\n";

// probe 14: trace equivalence (ESP-50 harness ported). A deterministic
// native tree and its JS mirror must produce byte-identical normalized
// draw-op traces through the fake backend.
const char kProbe14MirrorJs[] =
    "resetTree();\n"
    "var header = col().pad(16,40,4,40).gap(8)\n"
    "  .add(text('trace fixture').font(0), divider(2,0));\n"
    "var content = col().pad(24,40,24,40).gap(16)\n"
    "  .add(text('alpha').font(1),\n"
    "       text('beta').font(1).center().gray(96),\n"
    "       text(' chip ').font(0).invert());\n"
    "var footer = col().pad(6,40,10,40).gap(6)\n"
    "  .add(divider(1,0), text('native == js').font(0).gray(96));\n"
    "page('trace14').header(header).content(content).footer(footer)\n"
    "  .show(1);\n";

StatusCode BuildProbe14NativeSlots(s3paper::PageSlots *out) {
    s3paper::WidgetArena &a = s3paper_runtime::Arena();
    a.Reset();
    s3paper::WidgetHandle header = s3paper::NewCol(a).value;
    s3paper::WidgetNode *hn = a.Configure(header);
    hn->padding = s3paper::Insets{16, 40, 4, 40};
    hn->gap = 8;
    (void)a.AddChild(header, s3paper::NewText(a, "trace fixture",
                                              s3paper::kFontUi, 0)
                                 .value);
    (void)a.AddChild(header, s3paper::NewDivider(a, 2, 0).value);
    s3paper::WidgetHandle content = s3paper::NewCol(a).value;
    s3paper::WidgetNode *cn = a.Configure(content);
    cn->padding = s3paper::Insets{24, 40, 24, 40};
    cn->gap = 16;
    (void)a.AddChild(content,
                     s3paper::NewText(a, "alpha", s3paper::kFontBody, 0)
                         .value);
    (void)a.AddChild(content,
                     s3paper::NewText(a, "beta", s3paper::kFontBody, 96,
                                      s3paper::TextAlign::Center)
                         .value);
    s3paper::WidgetHandle chip =
        s3paper::NewText(a, " chip ", s3paper::kFontUi, 0).value;
    a.Configure(chip)->props.text.invert = 1;
    (void)a.AddChild(content, chip);
    s3paper::WidgetHandle footer = s3paper::NewCol(a).value;
    s3paper::WidgetNode *fn = a.Configure(footer);
    fn->padding = s3paper::Insets{6, 40, 10, 40};
    fn->gap = 6;
    (void)a.AddChild(footer, s3paper::NewDivider(a, 1, 0).value);
    (void)a.AddChild(footer,
                     s3paper::NewText(a, "native == js", s3paper::kFontUi,
                                      96)
                         .value);
    *out = s3paper::PageSlots{header, content, footer,
                              s3paper::kNullWidget};
    return StatusCode::Ok;
}

// Copies a fake-backend trace, replacing volatile "id=NNN" frame ids with
// "id=#" so two presents of the same content compare equal.
void NormalizeTraceInto(const char *src, char *dst, uint32_t cap) {
    uint32_t w = 0;
    for (const char *p = src; *p != '\0' && w + 5 < cap;) {
        if (strncmp(p, "id=", 3) == 0) {
            dst[w++] = 'i';
            dst[w++] = 'd';
            dst[w++] = '=';
            dst[w++] = '#';
            p += 3;
            while (*p >= '0' && *p <= '9') {
                p++;
            }
            continue;
        }
        dst[w++] = *p++;
    }
    dst[w] = '\0';
}

StatusCode RunTraceEquivalence() {
    constexpr uint32_t kTraceCap = 12 * 1024;
    static char *s_native = nullptr;
    static char *s_js = nullptr;
    if (s_native == nullptr) {
        s_native = static_cast<char *>(
            heap_caps_malloc(kTraceCap, MALLOC_CAP_SPIRAM));
        s_js = static_cast<char *>(
            heap_caps_malloc(kTraceCap, MALLOC_CAP_SPIRAM));
        if (s_native == nullptr || s_js == nullptr) {
            return StatusCode::OutOfMemory;
        }
    }
    s3paper::PageSlots slots{};
    (void)BuildProbe14NativeSlots(&slots);
    s3paper_runtime::SetTracePresent(true);
    const s3paper_runtime::PresentPageResult native =
        s3paper_runtime::PresentPage(slots,
                                     s3paper::PresentIntent::CleanFull,
                                     true, nullptr, 0, nullptr);
    if (native.status != StatusCode::Ok) {
        s3paper_runtime::SetTracePresent(false);
        return native.status;
    }
    NormalizeTraceInto(s3paper_runtime::FakeTrace(), s_native, kTraceCap);
    const StatusCode js =
        jsi::EvalBounded(kProbe14MirrorJs, 3000, "<trace14>");
    s3paper_runtime::SetTracePresent(false);
    if (js != StatusCode::Ok) {
        return js;
    }
    NormalizeTraceInto(s3paper_runtime::FakeTrace(), s_js, kTraceCap);
    const bool equal = strcmp(s_native, s_js) == 0;
    printf("probe14 trace-compare: %s (native=%u bytes, js=%u bytes)\n",
           equal ? "EQUAL" : "DIFFER",
           static_cast<unsigned>(strlen(s_native)),
           static_cast<unsigned>(strlen(s_js)));
    if (!equal) {
        printf("--- native ---\n%s--- js ---\n%s---\n", s_native, s_js);
    }
    return equal ? StatusCode::Ok : StatusCode::CorruptData;
}

// Probe 15 (ESP-53 P2): files module denials, caps, busy rejection, and a
// full write->read->list->remove chain. The chain rides completion
// callbacks, so its prints land on later owner-loop passes — read the
// whole transcript, not just the eval result.
const char kProbe15Js[] =
    "print('probe15: exists /books=' + files.exists('/books')"
    "      + ' rel=' + files.exists('books'));"
    "print('probe15: deny dotdot=' + files.remove('/../x', function(){})"
    "      + ' state=' + files.list('/.s3paper', function(){})"
    "      + ' slashes=' + files.read('//x', function(){}));"
    "var big = '0123456789abcdef';"
    "while (big.length < 17000) { big = big + big; }"
    "print('probe15: cap=' + files.write('/notes/big.txt', big,"
    "      function(){}));"
    "files.list('/books', function (k, n, e) {"
    "  print('probe15: list books k=' + k + ' n=' + n + ' e=' + e);"
    "  files.write('/notes/probe.txt', 'alpha\\nbeta\\ngamma',"
    "      function (k2, wrote, e2) {"
    "    print('probe15: write k=' + k2 + ' bytes=' + wrote + ' e=' + e2);"
    "    files.read('/notes/probe.txt', function (k3, lines, e3) {"
    "      print('probe15: read k=' + k3 + ' lines=' + lines + ' e=' + e3"
    "            + ' l0=' + files.line(0) + ' l2=' + files.line(2));"
    "      files.list('/notes', function (k4, n4, e4) {"
    "        print('probe15: list notes n=' + n4 + ' first='"
    "              + files.name(0) + ' size=' + files.size(0)"
    "              + ' dir=' + files.isDir(0));"
    "        files.remove('/notes/probe.txt', function (k5, ok, e5) {"
    "          print('probe15: remove ok=' + ok + ' e=' + e5"
    "                + ' exists=' + files.exists('/notes/probe.txt'));"
    "        });"
    "      });"
    "    });"
    "  });"
    "});"
    "var busy = 'no';"
    "try { files.list('/books', function () {}); }"
    "catch (err) { busy = 'yes'; }"
    "print('probe15: busy=' + busy);";

// Probe 16 (ESP-53 P4): http fetch battery — plain http, https via the
// cert bundle, truncation at a small limit, and a clean timeout error on
// an unroutable address. Requires the network to be up (net joinsaved
// first). Chained completions; the timeout leg takes ~10 s.
const char kProbe16Js[] =
    "http.get('http://example.com/').limit(4096)"
    "    .done(function (k, st, len) {"
    "  print('probe16: http st=' + st + ' len=' + len"
    "        + ' l0=' + http.bodyLine(0).slice(0, 30));"
    "  http.get('https://example.com/').limit(4096)"
    "      .done(function (k2, st2, len2) {"
    "    print('probe16: https st=' + st2 + ' len=' + len2);"
    "    http.get('https://example.com/').limit(256)"
    "        .done(function (k3, st3, len3) {"
    "      print('probe16: trunc st=' + st3 + ' len=' + len3"
    "            + ' (limit 256)');"
    "      http.get('http://10.255.255.1/')"
    "          .done(function (k4, st4, e4) {"
    "        print('probe16: timeout st=' + st4 + ' err=' + e4);"
    "      }).send();"
    "    }).send();"
    "  }).send();"
    "}).send();"
    "var busy = 'no';"
    "try { http.get('http://example.com/x'); }"
    "catch (err) { busy = 'yes'; }"
    "print('probe16: busy=' + busy);";

// Probe 17 (ESP-53 P5): brings the web server up with three JS routes and
// the static mount, then prints the URL for the workstation-side curl
// gate. /slow busy-waits 600 ms so two parallel curls provoke the
// single-slot 503; /note appends its query to the postcard journal.
const char kProbe17Js[] =
    "serve.get('/status').handle(function (req) {"
    "  return serve.json('{\"battery\":' + batteryLevel()"
    "      + ',\"ssid\":\"' + wifi.ssidCurrent() + '\"'"
    "      + ',\"rssi\":' + wifi.rssiCurrent()"
    "      + ',\"uptime_ms\":' + millis() + '}');"
    "});"
    "serve.get('/note').handle(function (req) {"
    "  var q = serve.query(req);"
    "  if (q === '') { return serve.status(400); }"
    "  appendPostcard(q);"
    "  return serve.text('noted: ' + q);"
    "});"
    "serve.get('/slow').handle(function (req) {"
    "  var t = millis();"
    "  while (millis() - t < 600) {}"
    "  return serve.text('slow done');"
    "});"
    "serve.files('/', '/sdcard/www');"
    "print('probe17: start=' + serve.start(80)"
    "      + ' url=' + serve.url());";

// Probe 18 (ESP-53 P7): module fault battery — malformed/overlong melody,
// wifi op overlap (join during scan throws "module busy"), double server
// start (Busy), and a clean stop. Expected: bad=1 overflow=2, scan=0,
// busy=yes, start1=0 start2=3, stop=0; scan-done arrives asynchronously.
const char kProbe18Js[] =
    "print('probe18: melody bad=' + buzzer.melody('garbage')"
    "      + ' overflow=' + buzzer.melody("
    "  '100:9,100:9,100:9,100:9,100:9,100:9,100:9,100:9,100:9,100:9,"
    "100:9,100:9,100:9,100:9,100:9,100:9,100:9'));"
    "buzzer.stop();"
    "var r1 = wifi.scan(function (k, n, e) {"
    "  print('probe18: scan done n=' + n + ' e=' + e);"
    "});"
    "var busy = 'no';"
    "try { wifi.join('x', 'y', function () {}); }"
    "catch (err) { busy = 'yes'; }"
    "serve.stop();"
    "print('probe18: scan=' + r1 + ' join-during-scan=' + busy"
    "      + ' start1=' + serve.start(8080)"
    "      + ' start2=' + serve.start(8080)"
    "      + ' stop=' + serve.stop());";

// Probe 19 (ESP-54 P1): battery singleton + legacy alias. Asserts level is
// in range, charging is a known flag, and the formatted status text is
// non-empty. The legacy batteryLevel() global must still equal battery.level().
const char kProbe19Js[] =
    "var lvl = battery.level();"
    "var mv = battery.mv();"
    "var ch = battery.charging();"
    "var st = battery.statusText();"
    "var legacy = batteryLevel();"
    "var lvlOk = (lvl >= -1 && lvl <= 100) ? 'ok' : 'BAD';"
    "var chOk = (ch >= -1 && ch <= 1) ? 'ok' : 'BAD';"
    "var match = (lvl === legacy) ? 'match' : 'MISMATCH';"
    "print('probe19: level=' + lvl + ' (' + lvlOk + ') mv=' + mv"
    "      + ' charging=' + ch + ' (' + chOk + ') statusText=\"' + st + '\"'"
    "      + ' legacy=' + match);";

// Probe 20 (ESP-54 P2): mDNS accessors. Asserts status is 0 or 1, host is
// "pulp", and url is empty when not announced or http://pulp.local when up.
const char kProbe20Js[] =
    "var st = mdns.status();"
    "var h = mdns.host();"
    "var u = mdns.url();"
    "var stOk = (st === 0 || st === 1) ? 'ok' : 'BAD';"
    "var hOk = (h === 'pulp') ? 'ok' : 'BAD';"
    "var uOk = (st === 0 ? u === '' : u.indexOf('pulp.local') >= 0)"
    "  ? 'ok' : 'BAD';"
    "print('probe20: status=' + st + ' (' + stOk + ') host=' + h"
    "      + ' (' + hOk + ') url=\"' + u + '\" (' + uOk + ')');";

// Probe 21 (ESP-54 P4): images catalog + display. Lists the catalog and, if
// non-empty, displays the first image. Expected: count >= 0, name(0) set
// when count > 0, display returns 0 when an image exists.
const char kProbe21Js[] =
    "var n = images.count();"
    "var first = n > 0 ? images.name(0) : '';"
    "var disp = n > 0 ? images.display(first) : -1;"
    "print('probe21: count=' + n + ' first=\"' + first + '\"'"
    "      + ' display=' + disp);";

// Probe 22 (ESP-54 P3): images upload-received registration. Registers a
// received callback and asserts the module-cb path accepts it; a second
// registration without completion must throw "module busy".
const char kProbe22Js[] =
    "var ok = 'no';"
    "images.received(function (k, bytes, err) {"
    "  print('probe22: received k=' + k + ' bytes=' + bytes + ' err=' + err);"
    "});"
    "ok = 'registered';"
    "var busy = 'no';"
    "try { images.received(function () {}); }"
    "catch (e) { busy = 'yes'; }"
    "print('probe22: cb=' + ok + ' second-busy=' + busy);";

// 23 (ESP-55 P3): load() happy path + every failure mode is a catchable
// JS exception (missing asset, bad SD path, missing file).
const char kProbe23Js[] =
    "var d = load('rom:dice');"
    "print('probe23: dice typeof=' + typeof d.main + ' id=' + d.id"
    "      + ' abi=' + d.abi);"
    "var miss = 'no'; try { load('rom:nope'); } catch (e) { miss = '' + e; }"
    "print('probe23: miss=' + miss);"
    "var bad = 'no'; try { load('/../x.js'); } catch (e2) { bad = '' + e2; }"
    "print('probe23: badpath=' + bad);"
    "var sd = 'no'; try { load('/apps/missing.js'); }"
    "catch (e3) { sd = '' + e3; }"
    "print('probe23: sd=' + sd);";

StatusCode RunTraced(const char *code, const char *name) {
    s3paper_runtime::SetTracePresent(true);
    const StatusCode ran = jsi::EvalBounded(code, 3000, name);
    s3paper_runtime::SetTracePresent(false);
    s3paper_runtime::PrintFakeTrace();
    return ran;
}

#include "js_measure_src.h"  // ESP-55 Phase 0: dice + settings sources

// Evals one embedded module source under the standard deadline, printing
// wall time and arena use (before, after, after GC). Returns the eval rc.
StatusCode MeasureOne(const char *name, const unsigned char *src,
                      unsigned len) {
    const uint32_t before = JS_GetHeapUsed(jsi::g_ctx);
    const int64_t t0 = esp_timer_get_time();
    const StatusCode rc =
        jsi::EvalBounded(reinterpret_cast<const char *>(src), 3000, name);
    const int64_t dt = esp_timer_get_time() - t0;
    const uint32_t after = JS_GetHeapUsed(jsi::g_ctx);
    JS_GC(jsi::g_ctx);
    const uint32_t gc = JS_GetHeapUsed(jsi::g_ctx);
    printf("measure: %s bytes=%u rc=%s us=%lld arena before=%u after=%u "
           "(+%d) gc=%u (+%d)\n",
           name, len, StatusCodeName(rc), static_cast<long long>(dt),
           static_cast<unsigned>(before), static_cast<unsigned>(after),
           static_cast<int>(after - before), static_cast<unsigned>(gc),
           static_cast<int>(gc - before));
    return rc;
}

}  // namespace

// ESP-55 Phase 0 (console: js measure). Evidence lines are stable-prefixed
// "measure:" for the console transcript.
StatusCode JsRunMeasure() {
    const StatusCode init = JsInit();
    if (init != StatusCode::Ok) {
        return init;
    }
    const uint32_t boot_used = JS_GetHeapUsed(jsi::g_ctx);
    JS_GC(jsi::g_ctx);
    printf("measure: baseline arena_used=%u after_gc=%u internal_free=%u "
           "psram_free=%u\n",
           static_cast<unsigned>(boot_used),
           static_cast<unsigned>(JS_GetHeapUsed(jsi::g_ctx)),
           static_cast<unsigned>(
               heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
           static_cast<unsigned>(
               heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
    StatusCode rc = MeasureOne("<dice>", kMeasureDice, kMeasureDiceLen);
    if (rc != StatusCode::Ok) { return rc; }
    rc = MeasureOne("<settings>", kMeasureSettings, kMeasureSettingsLen);
    if (rc != StatusCode::Ok) { return rc; }
    // Flatness: ten repeated evals of dice with GC between them must not
    // grow the arena (redefinition garbage is fully collectable).
    uint32_t used[10];
    for (int i = 0; i < 10; ++i) {
        const StatusCode r = jsi::EvalBounded(
            reinterpret_cast<const char *>(kMeasureDice), 3000, "<dice-n>");
        if (r != StatusCode::Ok) { return r; }
        JS_GC(jsi::g_ctx);
        used[i] = JS_GetHeapUsed(jsi::g_ctx);
    }
    printf("measure: dice x10 arena_after_gc:");
    for (int i = 0; i < 10; ++i) {
        printf(" %u", static_cast<unsigned>(used[i]));
    }
    printf("\n");
    printf("measure: done internal_free=%u\n",
           static_cast<unsigned>(
               heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
    return StatusCode::Ok;
}

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
        case 11: return jsi::EvalBounded(kProbe11Js, 3000, "<probe11>");
        case 12: return RunTraced(kProbe11Js, "<probe12>");
        case 13: {
            // Fault battery (ESP-51 P9): a runaway loop dies at the
            // deadline, an exception storm is counted, and the context
            // stays usable through all of it.
            const int64_t t0 = esp_timer_get_time();
            const StatusCode runaway =
                jsi::EvalBounded("for(;;){}", 800, "<runaway>");
            const int64_t elapsed_ms =
                (esp_timer_get_time() - t0) / 1000;
            printf("probe13: runaway -> %s after %lld ms\n",
                   StatusCodeName(runaway),
                   static_cast<long long>(elapsed_ms));
            for (int i = 0; i < 50; ++i) {
                (void)jsi::EvalBounded("throw new Error('storm');", 200,
                                       "<storm>");
            }
            return jsi::EvalBounded(
                "print('probe13: context alive after storm');", 500,
                "<alive>");
        }
        case 14: return RunTraceEquivalence();
        case 15: return jsi::EvalBounded(kProbe15Js, 3000, "<probe15>");
        case 16: return jsi::EvalBounded(kProbe16Js, 3000, "<probe16>");
        case 17: return jsi::EvalBounded(kProbe17Js, 3000, "<probe17>");
        case 18: return jsi::EvalBounded(kProbe18Js, 3000, "<probe18>");
        case 19: return jsi::EvalBounded(kProbe19Js, 3000, "<probe19>");
        case 20: return jsi::EvalBounded(kProbe20Js, 3000, "<probe20>");
        case 21: return jsi::EvalBounded(kProbe21Js, 5000, "<probe21>");
        case 22: return jsi::EvalBounded(kProbe22Js, 3000, "<probe22>");
        case 23: return jsi::EvalBounded(kProbe23Js, 5000, "<probe23>");
        default: return StatusCode::InvalidArgument;
    }
}

}  // namespace pulp
