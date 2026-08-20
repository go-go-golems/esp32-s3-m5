// ------------------------------------------------------------ catalog --
// App catalog (ESP-55 P3). Built-in apps live as flash assets
// (load('rom:<id>')); the entry carries the launcher metadata so listing
// never evaluates a module. `hidden` entries (reader) are launchable via
// os.launch but not shown. Keep ROM_APPS in launcher row order.
// ROM subtitles may be functions (evaluated at row-build time); SD
// manifest subtitles (Phase 4) are strings only.
var ROM_APPS = [
  { id: 'library', title: 'Reader', subtitle: 'books on the card',
    src: 'rom:library' },
  { id: 'reader', title: 'Reader page', subtitle: 'the open book',
    src: 'rom:reader', hidden: true },
  { id: 'dice', title: 'Dice Tray', subtitle: '2d6 coin d20 d%',
    src: 'rom:dice' },
  { id: 'blitz', title: 'Blitz Ink', subtitle: 'chess clock 5+3',
    src: 'rom:blitz' },
  { id: '2048', title: '2048 INK',
    subtitle: function () { return 'best ' + storeGet('2048best', 0); },
    src: 'rom:2048' },
  { id: 'tea', title: 'Tea Timer', subtitle: 'steep watch',
    src: 'rom:tea' },
  { id: 'postcard', title: 'Postcard', subtitle: 'one line a day',
    src: 'rom:postcard' },
  { id: 'daily', title: 'Daily Pulp', subtitle: 'a page at random',
    src: 'rom:daily' },
  { id: 'ink', title: 'Ink', subtitle: 'the beauty of e-ink',
    src: 'rom:ink' },
  { id: 'gallery', title: 'Gallery', subtitle: 'your pictures',
    src: 'rom:gallery' },
  { id: 'radio', title: 'Radio', subtitle: 'words from the ether',
    src: 'rom:radio' },
  { id: 'browser', title: 'Browser', subtitle: 'pages from the ether',
    src: 'rom:browser' },
  { id: 'settings', title: 'Settings', subtitle: 'wifi - serve - margins',
    src: 'rom:settings' }
];

// SD side (ESP-55 P4): /sdcard/apps/<id>.json manifests, one JSON object
// per line 0. SD entries override ROM (that is how you hot-patch a
// built-in without reflashing) — except `settings`, the recovery app.
// Seeded copies (manifest carries `seed`) keep the ROM metadata (so
// 2048's dynamic subtitle survives) but load from the card.
var SD_APPS = [];
var CAT = null;

function catalogInvalidate() { CAT = null; }

function merge() {
  var out = [];
  var i, j, hit;
  for (i = 0; i < ROM_APPS.length; i++) {
    var e = ROM_APPS[i];
    out.push({ id: e.id, title: e.title, subtitle: e.subtitle,
               src: e.src, hidden: e.hidden === true, source: 'rom' });
  }
  for (j = 0; j < SD_APPS.length; j++) {
    var m = SD_APPS[j];
    if (m.id === 'settings') { continue; }
    hit = null;
    for (i = 0; i < out.length; i++) {
      if (out[i].id === m.id) { hit = out[i]; break; }
    }
    if (m.broken) {
      if (hit) { hit.broken = m.broken; }
      else { out.push({ id: m.id, title: m.id, subtitle: '',
                        broken: m.broken, source: 'sd' }); }
      continue;
    }
    if (m.abi !== os.abi) {
      if (hit) { continue; }                 // ROM copy stays authoritative
      out.push({ id: m.id, title: m.title || m.id, subtitle: '',
                 broken: 'abi ' + m.abi, source: 'sd' });
      continue;
    }
    var src = m.src || ('/apps/' + m.id + '.js');
    if (hit) {
      hit.src = src;
      hit.source = m.seed ? 'seeded' : 'sd';
      if (!m.seed) {
        hit.title = m.title || hit.title;
        hit.subtitle = m.subtitle || hit.subtitle;
      }
    } else {
      out.push({ id: m.id, title: m.title || m.id,
                 subtitle: m.subtitle || '', src: src, source: 'sd' });
    }
  }
  return out;
}

function catalog() {
  if (!CAT) { CAT = merge(); }
  return CAT;
}

function catalogFind(id) {
  var c = catalog();
  var i;
  for (i = 0; i < c.length; i++) {
    if (c[i].id === id) { return c[i]; }
  }
  return null;
}

// Async manifest scan (files.* is one-op-at-a-time: a sequential chain).
// A resetTree mid-scan cancels the pending completion and the scan simply
// dies — callers rescan on demand.
function scanApps(done) {
  if (files.exists('/apps') !== 1) {
    SD_APPS = [];
    catalogInvalidate();
    if (done) { done(); }
    return;
  }
  var rc = files.list('/apps', function (k, count, err) {
    if (err !== 0) { if (done) { done(); } return; }
    var names = [];
    var i;
    for (i = 0; i < count; i++) {
      var nm = files.name(i);
      if (nm.length > 5 && nm.slice(-5) === '.json') { names.push(nm); }
    }
    var acc = [];
    function next(j) {
      if (j >= names.length) {
        SD_APPS = acc;
        catalogInvalidate();
        print('pulp apps: scanned ' + acc.length + ' manifest(s)');
        if (done) { done(); }
        return;
      }
      var id = names[j].slice(0, -5);
      var rc2 = files.read('/apps/' + names[j], function (k2, lines, e2) {
        var m = null;
        if (e2 === 0 && lines > 0) {
          try { m = JSON.parse(files.line(0)); } catch (ex) { m = null; }
        }
        if (m && m.id) { acc.push(m); }
        else { acc.push({ id: id, broken: 'bad manifest' }); }
        next(j + 1);
      });
      if (rc2 !== 0) { next(j + 1); }
    }
    next(0);
  });
  if (rc !== 0 && done) { done(); }
}

// First-boot seeding: copy every ROM app + a seed-marked manifest to the
// card. Runs only when /apps does not exist (no card => no-op); a
// firmware update refreshes nothing automatically in v1.
function seedApps() {
  if (files.exists('/apps') === 1) { return; }
  var n = apps.count();
  var seeded = 0;
  var i;
  for (i = 0; i < n; i++) {
    var id = apps.name(i);
    if (apps.copy(id, '/apps/' + id + '.js') !== 0) { continue; }
    var e = null;
    var r;
    for (r = 0; r < ROM_APPS.length; r++) {
      if (ROM_APPS[r].id === id) { e = ROM_APPS[r]; break; }
    }
    var title = e ? e.title : id;
    var sub = (e && typeof e.subtitle === 'string') ? e.subtitle : '';
    apps.writeText('/apps/' + id + '.json',
      '{"id":"' + id + '","title":"' + title + '","subtitle":"' + sub +
      '","version":1,"abi":' + os.abi + ',"src":"/apps/' + id +
      '.js","seed":1}');
    seeded = seeded + 1;
  }
  print('pulp apps: seeded ' + seeded + '/' + n);
}

// Pull install (ESP-55 P6): fetch a module over http(s) and store it on
// the card. Lives in the OS core so Settings and test drivers share it.
// done(msg) always fires exactly once with a human-readable outcome.
function idFromUrl(u) {
  var q = u.indexOf('?');
  if (q >= 0) { u = u.slice(0, q); }
  var id = u.slice(u.lastIndexOf('/') + 1);
  if (id.slice(-3) === '.js') { id = id.slice(0, -3); }
  if (id.length === 0 || id.length > 24) { return ''; }
  var i;
  for (i = 0; i < id.length; i++) {
    var c = id.charAt(i);
    var ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
             c === '_' || c === '-';
    if (!ok) { return ''; }
  }
  return id;
}

function installFromUrl(url, done) {
  var id = idFromUrl(url);
  if (id === '') { done('bad url (no app id)'); return; }
  if (id === 'settings') { done('cannot replace settings'); return; }
  os.netUp(function (ok) {
    if (ok !== 1) { done('no network'); return; }
    var rc = http.get(url).limit(32768).done(function (k, status, len) {
      if (status !== 200 || len <= 0) { done('http ' + status); return; }
      var w = apps.writeText('/apps/' + id + '.js', http.body());
      if (w !== 0) { done('write failed (' + w + ')'); return; }
      if (files.exists('/apps/' + id + '.json') !== 1) {
        apps.writeText('/apps/' + id + '.json',
          '{"id":"' + id + '","title":"' + id + '","subtitle":' +
          '"installed from url","version":1,"abi":' + os.abi +
          ',"src":"/apps/' + id + '.js"}');
      }
      scanApps(function () { done('installed ' + id + ' (' + len + 'B)'); });
    }).send();
    if (rc !== 0) { done('http busy (' + rc + ')'); }
  });
}

// Upload watcher (ESP-55 P5): re-registered by enter() at every app
// switch, like osRoutes. A successful push rescans; the launcher rebuilds
// if it is showing.
function appsWatch() {
  apps.received(function (k, bytes, err) {
    print('pulp apps: upload ' + apps.uploadName() + ' ' + bytes +
          'B err=' + err);
    if (err === 0) {
      scanApps(function () { if (RUN.id === 'home') { home(); } });
    }
  });
}
