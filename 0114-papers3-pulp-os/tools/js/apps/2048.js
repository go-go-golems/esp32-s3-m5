// --------------------------------------------------------------- 2048 ---

var GG = { g: null, score: 0, prev: null, pscore: 0, blits: 0 };

function g2048() {
  enter('2048');
  var st = GG;
  var cells = [];
  var scoreT = null;
  function spawn(g) {
    var free = [];
    var i;
    for (i = 0; i < 16; i++) { if (g[i] === 0) { free.push(i); } }
    if (free.length === 0) { return; }
    g[free[Math.floor(Math.random() * free.length)]] =
      Math.random() < 0.9 ? 2 : 4;
  }
  function fresh() {
    st.g = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    st.score = 0; st.prev = null;
    spawn(st.g); spawn(st.g);
  }
  if (!st.g) { fresh(); }
  var p = page('2048');
  // Diff update: .set() no-ops on unchanged text, so only tiles that
  // actually moved/merged produce damage rects. Repeated tile blits
  // accumulate ghosting, so every 12th present is a clean full re-blank.
  function refresh() {
    var i;
    for (i = 0; i < 16; i++) {
      cells[i].set(st.g[i] === 0 ? '.' : '' + st.g[i]);
    }
    scoreT.set('SCORE ' + st.score + '   BEST ' + storeGet('2048best', 0));
    st.blits = st.blits + 1;
    if (st.blits % 12 === 0) { p.show(true); } else { p.update(); }
  }
  function slideRow(c) {
    var out = [];
    var gained = 0;
    var i;
    for (i = 0; i < 4; i++) { if (c[i] !== 0) { out.push(c[i]); } }
    for (i = 0; i + 1 < out.length; i++) {
      if (out[i] === out[i + 1]) {
        out[i] *= 2; gained += out[i]; out.splice(i + 1, 1);
      }
    }
    while (out.length < 4) { out.push(0); }
    var moved = false;
    for (i = 0; i < 4; i++) { if (out[i] !== c[i]) { moved = true; } }
    return [out, gained, moved];
  }
  function move(dir) {
    var g = st.g;
    var before = g.slice(0);
    var gained = 0;
    var any = false;
    var r, c, line, res, i;
    for (r = 0; r < 4; r++) {
      line = [];
      for (c = 0; c < 4; c++) {
        if (dir === 0) { line.push(g[r * 4 + c]); }
        else if (dir === 1) { line.push(g[r * 4 + 3 - c]); }
        else if (dir === 2) { line.push(g[c * 4 + r]); }
        else { line.push(g[(3 - c) * 4 + r]); }
      }
      res = slideRow(line);
      gained += res[1];
      if (res[2]) { any = true; }
      for (i = 0; i < 4; i++) {
        if (dir === 0) { g[r * 4 + i] = res[0][i]; }
        else if (dir === 1) { g[r * 4 + 3 - i] = res[0][i]; }
        else if (dir === 2) { g[i * 4 + r] = res[0][i]; }
        else { g[(3 - i) * 4 + r] = res[0][i]; }
      }
    }
    if (any) {
      st.prev = before;
      st.pscore = st.score;
      st.score += gained;
      if (gained > 0) { buzzer.tone(Math.min(660 + gained * 2, 1760), 30); }
      spawn(g);
      if (st.score > storeGet('2048best', 0)) {
        storeSet('2048best', st.score);
      }
      refresh();
    }
  }
  var body = col().pad(10, 30, 4, 30).gap(0);
  body.add(divider(2, 0));
  var r2, c2;
  for (r2 = 0; r2 < 4; r2++) {
    var line2 = row().gap(0).mainAlign(1);
    for (c2 = 0; c2 < 4; c2++) {
      var cell = text('.').size('lg').center().width(118).height(64);
      cells.push(cell);
      line2.add(cell);
    }
    body.add(line2);
    body.add(divider(2, 0));
  }
  body.add(spacer(8, 0));
  scoreT = text(' ').size('lg').center();
  body.add(scoreT);
  var btns = row().pad(8, 0, 0, 0).gap(20).mainAlign(1);
  btns.add(text('[ new game ]').size('xs').center().width(160).height(56)
    .onTap(function () { fresh(); refresh(); }));
  btns.add(text('[ undo ]').size('xs').center().width(110).height(56)
    .onTap(function () {
      if (st.prev) {
        st.g = st.prev; st.score = st.pscore; st.prev = null; refresh();
      }
    }));
  btns.add(text('[ home ]').size('xs').center().width(110).height(56)
    .onTap(function () { home(); }));
  body.add(btns);
  p.header(chrome('2048 INK')).content(body)
    .footer(hintFooter('swipe to slide tiles - home via button'));
  // Registering G.DOWN traps swipe-down as a move (home via button).
  p.on(G.LEFT, function () { move(0); });
  p.on(G.RIGHT, function () { move(1); });
  p.on(G.UP, function () { move(2); });
  p.on(G.DOWN, function () { move(3); });
  announce('2048');
  p.show(true);
  refresh();
}

