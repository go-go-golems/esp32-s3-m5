// ---------------------------------------------------------------- home --

// Status glyph text for the launcher header (dynamic; refreshed on the
// home page's slow tick, presented only when it changes).
function wifiGlyph() {
  var st = wifi.status();
  if (st === 4) { return 'wifi ' + wifi.ip(); }
  if (st === 3) { return 'wifi joining'; }
  if (st === 2) { return 'wifi scanning'; }
  return st === 0 ? '' : 'wifi idle';
}

// Battery glyph (ESP-54): level% and a '+' when charging. A dyn value so
// the home tick refreshes it only when the string changes.
function batteryGlyph() {
  var s = battery.statusText();
  return s === '?' ? '' : s;
}

function home() {
  RUN.id = 'home';
  RUN.desc = null;
  enter('home');
  var header = col().pad(16, M, 6, M).gap(4).add(
    row().crossAlign(3).add(
      text('PULP').size('xl'), spacer(0, 1),
      text(function () { return batteryGlyph() + '  ' + wifiGlyph(); })
        .size('xs').gray(96)),
    text('THE PAPERBACK OF COMPUTERS').size('xs').gray(96),
    divider(8, 0));
  var menu = list().pad(4, 0, 0, 0);
  // ESP-56: one row idiom for launcher, settings and catalogs.
  function entryRow(label, sub, fn) { os.menuRow(menu, label, sub, fn); }
  // ESP-55: rows come from the merged catalog (ROM + SD). '*' marks an
  // operator-installed/patched copy; '!' marks a broken manifest.
  var apps_ = catalog();
  var i;
  for (i = 0; i < apps_.length; i++) {
    (function (e) {
      if (e.hidden) { return; }
      var sub = typeof e.subtitle === 'function' ? e.subtitle()
                                                 : (e.subtitle || '');
      if (e.broken) { sub = '! ' + e.broken; }
      else if (e.source === 'sd') { sub = sub + ' *'; }
      entryRow(e.title, sub, function () {
        if (e.broken) { errorPage(e.id, e.broken); }
        else { launch(e.id); }
      });
    })(apps_[i]);
  }
  var p = page('home').header(header).content(menu)
    .footer(hintFooter('tap to open - swipe down = home'));
  p.every(5000);
  // ESP-54: register OS web routes once serve becomes available. Runs
  // from the tick because serve may start after boot (osRoutes no-ops
  // when serve.url()==='' at enter() time).
  p.on(G.TICK, function () {
    if (PENDING_LAUNCH !== '') {           // ESP-55: GET /apps/run pickup
      var id = PENDING_LAUNCH;
      var parg = PENDING_ARG;
      PENDING_LAUNCH = '';
      PENDING_ARG = null;
      launch(id, parg);
      return;
    }
    if (!ROUTES_READY && serve.url() !== '') {
      ROUTES_READY = true;
      osRoutes();
    }
  });
  announce('home');
  p.show(true);
}
