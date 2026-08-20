// ------------------------------------------------------------- facade --
// The OS facade: the ONLY contract between the OS core and app modules.
// Apps receive it as main(os, arg). Everything here maps onto kernel
// helpers; keep it small — a member added here is API forever.

// Per-app state that survives app switches (resetTree wipes widgets and
// callbacks, never JS globals). Cleared by os.clearState / Settings.
var STATE = {};

var os = {
  // Content margin (kernel global M is the truth; getter keeps it live).
  get M() { return M; },
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
  // Brings the network up with saved credentials, then fn(ok).
  netUp: function (fn) {
    if (wifi.status() === 4) { fn(1); return; }
    if (wifi.savedCount() === 0) { fn(0); return; }
    wifi.joinSaved(function (k, ok, err) { fn(ok); });
  }
};
