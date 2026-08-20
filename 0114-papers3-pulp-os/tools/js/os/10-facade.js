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
  // One keyboard for every text-entry screen: 48x56 serif-capable keys
  // (keys render in the grotesque face deliberately — single glyphs read
  // as symbols, not prose), hairline rules inside a 24 px gutter.
  // rows: array of key strings; onKey(ch); opts.del adds a delete key on
  // the last row calling opts.del().
  keyboard: function (body, rows, onKey, opts) {
    var o = opts || {};
    var r, i;
    for (r = 0; r < rows.length; r++) {
      var line = row().gap(2).mainAlign(1);
      for (i = 0; i < rows[r].length; i++) {
        (function (ch) {
          line.add(text(ch).size('lg').center().width(48).height(56)
            .onTap(function () { onKey(ch); }));
        })(rows[r].charAt(i));
      }
      if (o.del && r === rows.length - 1) {
        line.add(text('<del>').size('xs').center().width(70).height(52)
          .onTap(function () { o.del(); }));
      }
      body.add(line);
      body.add(divider(1, 200));
    }
  }
};
