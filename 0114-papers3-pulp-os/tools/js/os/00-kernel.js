// PULP OS v2 - "the paperback of computers".
// One bytecode image: launcher + apps on the native builder API (ESP-51).
// Widgets/Pages are native class instances; closures live in __cbs; the
// kernel provides G = gesture constants. Swipe-down goes home via
// paper.home unless an app registers its own G.DOWN handler.

var P = { app: 'home' };
var ROUTES_READY = false;  // ESP-54: routes register once serve is up
var PENDING_LAUNCH = '';   // ESP-55: set by GET /apps/run, run by the home tick

// Global content margin (px). Toggled by long-pressing the launcher and
// persisted in the settings store; every app reads it at build time.
var M = 40;

function pad2(n) { return (n < 10 ? '0' : '') + n; }
function fmtClock(ms) {
  if (ms < 0) { ms = 0; }
  var s = Math.floor(ms / 1000);
  return pad2(Math.floor(s / 60)) + ':' + pad2(s % 60);
}

// OS-owned web routes: re-registered at every app switch so they survive
// resetTree as long as the server runs (found by the P7 soak: a phantom
// swipe-home wiped the probe's routes mid-soak). Registered whenever the
// server is actually running (ESP-54: serve may start after boot, so the
// home tick re-checks serve.url() and registers on first availability).
function osRoutes() {
  if (serve.url() === '') { return; }
  ROUTES_READY = true;
  serve.get('/status').handle(function (req) {
    return serve.json('{"battery":' + batteryLevel()
      + ',"charging":' + battery.charging()
      + ',"ssid":"' + wifi.ssidCurrent() + '"'
      + ',"rssi":' + wifi.rssiCurrent()
      + ',"app":"' + P.app + '"'
      + ',"mdns":"' + mdns.url() + '"'
      + ',"uptime_ms":' + millis() + '}');
  });
  // ESP-54: list stored images for the upload page's gallery sidebar.
  // ESP-55 P5: catalog listing + remote launch (picked up by the home
  // tick — route handlers must not present or resetTree).
  serve.get('/apps/list').handle(function (req) {
    var c = catalog();
    var s = '{"apps":[';
    var i;
    var first = true;
    for (i = 0; i < c.length; i++) {
      if (c[i].hidden) { continue; }
      if (!first) { s += ','; }
      first = false;
      s += '{"id":"' + c[i].id + '","title":"' + c[i].title +
           '","source":"' + c[i].source +
           (c[i].broken ? '","broken":"' + c[i].broken : '') + '"}';
    }
    s += ']}';
    return serve.json(s);
  });
  serve.get('/apps/run').handle(function (req) {
    var q = serve.query(req);
    var id = q.indexOf('id=') === 0 ? q.slice(3) : '';
    if (!catalogFind(id)) { return serve.status(404); }
    if (P.app !== 'home') { return serve.status(409); }
    PENDING_LAUNCH = id;
    return serve.json('{"run":"' + id + '"}');
  });
  serve.get('/images/list').handle(function (req) {
    var n = images.count();
    var s = '{"count":' + n + ',"images":[';
    var i;
    for (i = 0; i < n; i++) {
      if (i > 0) { s += ','; }
      s += '"' + images.name(i) + '"';
    }
    s += ']}';
    return serve.json(s);
  });
}

// App-switch boundary: drop the whole tree/page/callback state, then
// re-register the OS chrome callbacks that resetTree cleared.
function enter(name) {
  P.app = name;
  resetTree();
  paper.home(function () { home(); });
  paper.sleepImage(function () {
    return col().pad(0, M, 0, M).gap(24).mainAlign(1)
      .add(text('PULP').size('xl').center(),
           text('asleep - press the side button').size('xs').center());
  });
  osRoutes();
  appsWatch();
}

function announce(name) { print('pulp screen: ' + name); }

function chrome(title) {
  return col().pad(16, M, 6, M).gap(8)
    .add(text(title).size('lg'), divider(6, 0));
}

function hintFooter(hint) {
  return col().pad(4, M, 10, M).gap(5)
    .add(divider(1, 0), text(hint).size('xs').gray(96));
}

