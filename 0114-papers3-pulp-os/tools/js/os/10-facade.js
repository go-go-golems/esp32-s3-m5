// ------------------------------------------------------------- facade --
// The OS facade: the ONLY contract between the OS core and app modules.
// Apps receive it as main(os, arg). Everything here maps onto kernel
// helpers; keep it small — a member added here is API forever.
//
// ESP-56: os.M is a plain NUMBER. mquickjs parses object-literal getters
// WITHOUT accessor semantics (`get M() {…}` becomes a property holding
// the function), which silently zeroed every app margin. setMargin keeps
// the kernel global, this field and the store in sync.

// Per-app state that survives app switches (resetTree wipes widgets and
// callbacks, never JS globals). Cleared by os.clearState / Settings.
var STATE = {};

var os = {
  M: 40,
  abi: abiVersion(),
  chrome: chrome,
  hintFooter: hintFooter,
  announce: announce,
  pad2: pad2,
  fmtClock: fmtClock,
  home: function () { home(); },
  launch: function (id, arg) { launch(id, arg); },
  state: function (id, init) {
    if (!STATE.hasOwnProperty(id)) { STATE[id] = init ? init() : {}; }
    return STATE[id];
  },
  clearState: function (id) { delete STATE[id]; },
  clearAllState: function () { STATE = {}; },
  setMargin: function (v) {
    M = v;
    os.M = v;
    storeSet('margin', v);
  },
  // Brings the network up with saved credentials, then fn(ok).
  netUp: function (fn) {
    if (wifi.status() === 4) { fn(1); return; }
    if (wifi.savedCount() === 0) { fn(0); return; }
    wifi.joinSaved(function (k, ok, err) { fn(ok); });
  },

  // ---- design-system idioms (ESP-56) --------------------------------
  // Identity = grotesque (lg/xl) for titles and hero numerals only.
  // Text + CONTROLS = serif. Buttons are serif md, fat targets, no
  // brackets; the primary action is an inverted chip.

  // The content column: everything an app draws lives inside the margin.
  body: function (padTop) {
    return col().pad(typeof padTop === 'number' ? padTop : 16,
                    os.M, 0, os.M);
  },
  // THE menu row (launcher, settings, catalogs share this one shape).
  menuRow: function (menu, label, sub, fn) {
    var line = row().pad(6, 0, 4, 0).gap(10).crossAlign(3)
      .add(text(label).size('lg'), spacer(0, 1),
           text(sub).size('xs').gray(112));
    var entry = col().pad(0, os.M, 0, os.M).add(line, divider(2, 0));
    if (fn) { entry.onTap(fn); }
    menu.add(entry);
  },
  button: function (label, fn, opts) {
    var o = opts || {};
    var t = text(label).size(o.size || 'md').center()
      .width(o.w || 140).height(56).onTap(fn);
    if (o.primary) { t.invert(); }
    return t;
  },
  buttonRow: function () {
    return row().pad(10, 0, 0, 0).gap(16).mainAlign(1);
  },
  // Section label: prompts and group headings are grotesque (ESP-56 v2:
  // the small serif is reading text ONLY, never labels).
  label: function (t) { return text(t).size('lg'); },
  // One keyboard for every text-entry screen, ON THE GRID: 42x56 keys so
  // a ten-column row (438 px) sits inside the 40 px margins; hairline
  // rules span the content column. Delete lives in the screen's action
  // row (os.key), not inside the key block.
  keyboard: function (body, rows, onKey) {
    var r, i;
    for (r = 0; r < rows.length; r++) {
      var line = row().gap(2).mainAlign(1);
      for (i = 0; i < rows[r].length; i++) {
        (function (ch) {
          line.add(text(ch).size('lg').center().width(42).height(56)
            .onTap(function () { onKey(ch); }));
        })(rows[r].charAt(i));
      }
      body.add(line);
      body.add(divider(1, 200));
    }
  },
  // A single wide key for action rows (del, symbols).
  key: function (label, fn, w) {
    return text(label).size('lg').center().width(w || 52).height(56)
      .onTap(fn);
  }
};
