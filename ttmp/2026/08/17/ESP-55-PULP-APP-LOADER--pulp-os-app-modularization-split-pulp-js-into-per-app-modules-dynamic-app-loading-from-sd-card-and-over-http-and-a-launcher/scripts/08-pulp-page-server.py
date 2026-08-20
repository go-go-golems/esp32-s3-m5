#!/usr/bin/env python3
"""ESP-55 P10: reference page server for the PULP browser. Serves the
example pages in ./pages-demo (created on first run) on --port 8123.
A page is a JS descriptor ({title, main(ui, nav)}) in the builder DSL.

usage: 08-pulp-page-server.py [--port 8123] [--dir DIR]
Then on the device: Browser -> http://<host-ip>:8123/menu.js
"""
import argparse, http.server, os, sys

MENU = """// demo menu page
({
  title: 'Menu',
  main: function (ui, nav) {
    var p = page('menu').header(ui.chrome('PULP PAGES'))
      .content(col().pad(4, 0, 0, 0).gap(0).add(
        ui.row('Clock', 'a live dyn-text page', function () {
          nav.go('clock.js');
        }),
        ui.row('About', 'what pages are', function () {
          nav.go('about.js');
        }),
        ui.row('Reload', 'fetch this menu again', function () {
          nav.reload();
        })))
      .footer(ui.hintFooter('served over http - swipe down = home'));
    p.show(true);
  }
})
"""

CLOCK = """// demo clock page: dyn text + tick inside the sandbox
({
  title: 'Clock',
  main: function (ui, nav) {
    var t0 = millis();
    var p = page('clock').header(ui.chrome('PAGE CLOCK'))
      .content(col().pad(40, ui.M, 0, ui.M).gap(16).add(
        text(function () {
          var s = Math.floor((millis() - t0) / 1000);
          return 'on page ' + s + 's';
        }).size('xl').center(),
        ui.row('back to menu', 'nav.back()', function () { nav.back(); })))
      .footer(ui.hintFooter('ticks once a second - swipe down = home'));
    p.every(1000);
    p.show(true);
  }
})
"""

ABOUT = """// demo about page
({
  title: 'About',
  main: function (ui, nav) {
    var p = page('about').header(ui.chrome('ABOUT PAGES'))
      .content(col().pad(20, ui.M, 0, ui.M).gap(10).add(
        text('this page is a JS script').size('md'),
        text('it can draw and navigate.').size('sm').gray(64),
        text('files, http, wifi, store: all denied.').size('sm').gray(64),
        ui.row('menu', 'nav.go absolute-relative', function () {
          nav.go('menu.js');
        })))
      .footer(ui.hintFooter('swipe down = home'));
    p.show(true);
  }
})
"""

HOSTILE = """// hostile page: tries to escape, then spins. Expected: denials
// throw, the loop is killed by the deadline, the browser shows an error.
({
  title: 'Hostile',
  main: function (ui, nav) {
    try { files.remove('/apps/dice.js', function () {}); } catch (e) {}
    try { wifi.forget('yolobolo'); } catch (e2) {}
    for (;;) { }
  }
})
"""

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8123)
    ap.add_argument("--dir", default=None)
    a = ap.parse_args()
    d = a.dir or os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              "pages-demo")
    os.makedirs(d, exist_ok=True)
    for name, body in [("menu.js", MENU), ("clock.js", CLOCK),
                       ("about.js", ABOUT), ("hostile.js", HOSTILE)]:
        with open(os.path.join(d, name), "w") as f:
            f.write(body)
    os.chdir(d)
    http.server.HTTPServer(
        ("", a.port),
        http.server.SimpleHTTPRequestHandler).serve_forever()

if __name__ == "__main__":
    sys.exit(main())
