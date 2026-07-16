// PULP OS v2 - "the paperback of computers".
// One bytecode image: launcher + apps on the native builder API (ESP-51).
// Widgets/Pages are native class instances; closures live in __cbs; the
// kernel provides G = gesture constants. Swipe-down goes home via
// paper.home unless an app registers its own G.DOWN handler.

var P = { app: 'home' };

function pad2(n) { return (n < 10 ? '0' : '') + n; }
function fmtClock(ms) {
  if (ms < 0) { ms = 0; }
  var s = Math.floor(ms / 1000);
  return pad2(Math.floor(s / 60)) + ':' + pad2(s % 60);
}

// App-switch boundary: drop the whole tree/page/callback state, then
// re-register the OS chrome callbacks that resetTree cleared.
function enter(name) {
  P.app = name;
  resetTree();
  paper.home(function () { home(); });
  paper.sleepImage(function () {
    return col().pad(0, 40, 0, 40).gap(24).mainAlign(1)
      .add(text('PULP').size('xl').center(),
           text('asleep - press the side button').size('xs').center());
  });
}

function announce(name) { print('pulp screen: ' + name); }

function chrome(title) {
  return col().pad(16, 40, 6, 40).gap(8)
    .add(text(title).size('lg'), divider(6, 0));
}

function hintFooter(hint) {
  return col().pad(4, 40, 10, 40).gap(5)
    .add(divider(1, 0), text(hint).size('xs').gray(96));
}

// ---------------------------------------------------------------- home --

function home() {
  enter('home');
  var header = col().pad(16, 40, 6, 40).gap(4).add(
    text('PULP').size('xl'),
    text('THE PAPERBACK OF COMPUTERS').size('xs').gray(96),
    divider(8, 0));
  var menu = list().pad(4, 0, 0, 0);
  function entryRow(label, sub, fn) {
    // Separators carry the same 40px margins as the header rule.
    var line = row().pad(8, 0, 6, 0).gap(10).crossAlign(3)
      .add(text(label).size('lg'), spacer(0, 1),
           text(sub).size('xs').gray(112));
    menu.add(col().pad(0, 40, 0, 40).add(line, divider(2, 0)).onTap(fn));
  }
  entryRow('Reader', 'books on the card', library);
  entryRow('Dice Tray', '2d6 coin d20 d%', dice);
  entryRow('Blitz Ink', 'chess clock 5+3', blitz);
  entryRow('2048 INK', 'best ' + storeGet('2048best', 0), g2048);
  entryRow('Tea Timer', 'steep watch', tea);
  entryRow('Postcard', 'one line a day', postcard);
  entryRow('Daily Pulp', 'a page at random', daily);
  var p = page('home').header(header).content(menu)
    .footer(hintFooter('tap to open - swipe down = home anywhere'));
  announce('home');
  p.show(true);
}

// ------------------------------------------------------------- library --

function library() {
  enter('library');
  var menu = list().pad(4, 0, 0, 0);
  var n = libraryCount();
  if (n === 0) { n = libraryRescan(); }
  var i;
  for (i = 0; i < n; i++) {
    (function (idx) {
      // Separators share the header rule's 40px margins.
      menu.add(col().pad(0, 40, 0, 40).add(
        row().pad(10, 0, 8, 0).add(text(libraryLine(idx)).size('sm')),
        divider(1, 176)).onTap(function () { reader(idx); }));
    })(i);
  }
  if (n === 0) {
    menu.add(col().pad(20, 40, 0, 40)
      .add(text('no books on the card').size('sm').gray(96)));
  }
  var p = page('library').header(chrome('LIBRARY')).content(menu)
    .footer(hintFooter('tap a book - swipe down = home'));
  announce('library');
  p.show(true);
}

// -------------------------------------------------------------- reader --

var RD = { lines: [], title: null, foot: null, turns: 0 };

function readerUpdate(p) {
  var n = bookLineCount();
  var i;
  for (i = 0; i < RD.lines.length; i++) {
    RD.lines[i].set(i < n ? bookLine(i) : '');
  }
  RD.title.set(bookTitle() + '  [pulp reader]');
  RD.foot.set(Math.floor(bookProgress() / 10) + '%  turns ' + RD.turns);
  p.update();
}

function reader(idx) {
  if (bookOpen(idx) !== 0) { library(); return; }
  enter('reader');
  RD.lines = [];
  RD.turns = 0;
  RD.title = text('').size('xs');
  var header = col().pad(14, 40, 4, 40).gap(6)
    .add(RD.title, divider(1, 0));
  var body = col().pad(4, 40, 0, 40).gap(2);
  var i;
  for (i = 0; i < 24; i++) {
    var line = text(' ');
    RD.lines.push(line);
    body.add(line);
  }
  RD.foot = text('').size('xs').gray(96);
  var footer = col().pad(4, 40, 10, 40).gap(6)
    .add(divider(1, 0), RD.foot);
  var p = page('reader').header(header).content(body).footer(footer);
  function turn(fwd) {
    var moved = fwd ? bookNext() : bookPrev();
    if (moved === 0) { RD.turns = RD.turns + 1; readerUpdate(p); }
  }
  p.on(G.LEFT, function () { turn(true); });
  p.on(G.RIGHT, function () { turn(false); });
  p.on(G.TAP, function (k, x, y) { turn(x >= 270); });
  announce('reader');
  p.show(true);
  readerUpdate(p);
}

// ---------------------------------------------------------------- dice --

var DICE_PIPS = [
  ['       ', '   *   ', '       '],
  ['*      ', '       ', '      *'],
  ['*      ', '   *   ', '      *'],
  ['*     *', '       ', '*     *'],
  ['*     *', '   *   ', '*     *'],
  ['*     *', '*     *', '*     *']];

var DZ = { mode: '2d6', a: 3, b: 4, big: '', hist: [] };

function dice() {
  enter('dice');
  var ui = { dieA: [], dieB: [] };
  function die(rows) {
    var c = col().pad(6, 10, 6, 10).gap(2);
    var i;
    for (i = 0; i < 3; i++) {
      var t = text(' ').size('md').center();
      rows.push(t);
      c.add(t);
    }
    return c;
  }
  function refresh(p) {
    var i;
    for (i = 0; i < 3; i++) {
      ui.dieA[i].set(DZ.mode === '2d6' ? DICE_PIPS[DZ.a - 1][i] : ' ');
      ui.dieB[i].set(DZ.mode === '2d6' ? DICE_PIPS[DZ.b - 1][i] : ' ');
    }
    ui.big.set(DZ.big === '' ? 'ROLL' : DZ.big);
    ui.hist.set('history: ' + DZ.hist.join(' - '));
    p.update();
  }
  var body = col().pad(16, 40, 8, 40).gap(12);
  body.add(row().gap(24).mainAlign(1).add(die(ui.dieA), die(ui.dieB)));
  ui.big = text('ROLL').size('xl').center();
  body.add(ui.big);
  body.add(divider(1, 176));
  var btns = row().pad(10, 0, 10, 0).gap(8).mainAlign(1);
  var p = page('dice').header(chrome('DICE TRAY')).content(body)
    .footer(hintFooter('tap a roll - swipe down = home'));
  function roll(mode) {
    DZ.mode = mode;
    var t = '';
    if (mode === '2d6') {
      DZ.a = 1 + Math.floor(Math.random() * 6);
      DZ.b = 1 + Math.floor(Math.random() * 6);
      t = '' + (DZ.a + DZ.b);
    } else if (mode === 'd20') {
      t = '' + (1 + Math.floor(Math.random() * 20));
    } else if (mode === 'coin') {
      t = Math.random() < 0.5 ? 'heads' : 'tails';
    } else {
      t = '' + (1 + Math.floor(Math.random() * 100));
    }
    DZ.big = t;
    DZ.hist.unshift(t);
    if (DZ.hist.length > 5) { DZ.hist.pop(); }
    refresh(p);
  }
  function btn(label, mode) {
    // Fat tap targets: a finger needs more than the glyph rect.
    btns.add(text(label).size('lg').center().width(108).height(72)
      .onTap(function () { roll(mode); }));
  }
  btn('2d6', '2d6');
  btn('d20', 'd20');
  btn('coin', 'coin');
  btn('d%', 'd%');
  body.add(btns);
  ui.hist = text('history:').size('xs').gray(96);
  body.add(ui.hist);
  announce('dice');
  p.show(true);
  refresh(p);
}

// --------------------------------------------------------------- blitz --

var BZ = { w: 300000, b: 300000, inc: 3000, run: 0, last: 0, moves: 0 };

function blitz() {
  enter('blitz');
  var z = BZ;
  var ui = {};
  function settle() {
    if (z.run !== 0) {
      var now = millis();
      var elapsed = now - z.last;
      z.last = now;
      if (z.run === 1) { z.w -= elapsed; } else { z.b -= elapsed; }
    }
  }
  var p = page('blitz');
  // Dynamic values: the tick re-evaluates these; only changed text blits.
  ui.bt = text(function () { return fmtClock(z.b); }).size('xl').center();
  ui.bl = text(function () {
    return (z.run === 2 ? '> ' : '') + 'BLACK';
  }).size('xs').center();
  ui.wt = text(function () { return fmtClock(z.w); }).size('xl').center();
  ui.wl = text(function () {
    return (z.run === 1 ? '> ' : '') + 'WHITE';
  }).size('xs').center();
  ui.mid = text(function () {
    if (z.w <= 0) { return 'WHITE FLAGS'; }
    if (z.b <= 0) { return 'BLACK FLAGS'; }
    return 'MOVES ' + z.moves;
  }).size('lg').center();
  function hit(side) {
    settle();
    if (z.run === side) {
      if (side === 1) { z.w += z.inc; } else { z.b += z.inc; }
      z.run = side === 1 ? 2 : 1;
      z.moves = z.moves + 1;
    } else if (z.run === 0) {
      z.run = side === 1 ? 2 : 1;
      z.last = millis();
    }
    p.update();
  }
  var zb = col().pad(20, 40, 20, 40).gap(6).add(ui.bt, ui.bl)
    .onTap(function () { hit(2); });
  var zw = col().pad(20, 40, 20, 40).gap(6).add(ui.wt, ui.wl)
    .onTap(function () { hit(1); });
  var body = col().pad(8, 0, 0, 0).gap(4)
    .add(zb, divider(6, 0), ui.mid, divider(6, 0), zw);
  p.header(chrome('BLITZ INK 5+3')).content(body)
    .footer(hintFooter('tap your side - hold = pause'));
  p.on(G.LONG, function () { settle(); z.run = 0; p.update(); });
  p.on(G.TICK, function () { settle(); });
  p.every(1000);
  announce('blitz');
  p.show(true);
}

// --------------------------------------------------------------- 2048 ---

var GG = { g: null, score: 0, prev: null, pscore: 0 };

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
  // actually moved/merged produce damage rects.
  function refresh() {
    var i;
    for (i = 0; i < 16; i++) {
      cells[i].set(st.g[i] === 0 ? '.' : '' + st.g[i]);
    }
    scoreT.set('SCORE ' + st.score + '   BEST ' + storeGet('2048best', 0));
    p.update();
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

// ----------------------------------------------------------------- tea --

var TEA_KINDS = [['green', 120], ['black', 240], ['herbal', 360]];
var TT = null;

function tea() {
  enter('tea');
  if (!TT) {
    var k0 = storeGet('teakind', 1);
    TT = { kind: k0, total: TEA_KINDS[k0][1], left: TEA_KINDS[k0][1],
           run: false, done: false };
  }
  var t = TT;
  var ui = {};
  var p = page('tea');
  function refresh(full) {
    ui.kind.set(TEA_KINDS[t.kind][0].toUpperCase());
    ui.time.set(t.done ? 'READY' : fmtClock(t.left * 1000));
    ui.bar.progress(Math.floor(1000 - (t.left * 1000 / t.total)));
    ui.pause.set(t.run ? '[ pause ]' : '[ start ]');
    if (full) { p.show(true); } else { p.update(); }
  }
  function pick(i) {
    t.kind = i; t.total = TEA_KINDS[i][1]; t.left = t.total;
    t.run = true; t.done = false;
    storeSet('teakind', i);
    refresh(false);
  }
  ui.kind = text(' ').size('lg').center();
  ui.time = text(' ').size('xl').center();
  ui.bar = progressBar(0, 24).height(24);
  var body = col().pad(18, 40, 8, 40).gap(14)
    .add(ui.kind, ui.time, ui.bar, divider(1, 176));
  var kinds = row().pad(6, 0, 0, 0).gap(18).mainAlign(1);
  var i;
  for (i = 0; i < 3; i++) {
    (function (idx) {
      kinds.add(text(TEA_KINDS[idx][0] + ' ' +
                     fmtClock(TEA_KINDS[idx][1] * 1000))
        .size('xs').center().width(140).height(56)
        .onTap(function () { pick(idx); }));
    })(i);
  }
  body.add(kinds);
  var btns = row().pad(6, 0, 0, 0).gap(24).mainAlign(1);
  btns.add(text('[ +30s ]').size('xs').center().width(120).height(56)
    .onTap(function () {
      t.left += 30; t.done = false; refresh(false); }));
  ui.pause = text(' ').size('xs').center().width(120).height(56)
    .onTap(function () { t.run = !t.run && t.left > 0; refresh(false); });
  btns.add(ui.pause);
  btns.add(text('[ reset ]').size('xs').center().width(120).height(56)
    .onTap(function () {
      t.left = t.total; t.run = false; t.done = false; refresh(false); }));
  body.add(btns);
  p.header(chrome('TEA TIMER')).content(body)
    .footer(hintFooter('page blinks when the tea is ready'));
  p.on(G.TICK, function () {
    if (t.run && t.left > 0) {
      t.left -= 1;
      if (t.left <= 0) { t.run = false; t.done = true; refresh(true); }
      else { refresh(false); }
    }
  });
  p.every(1000);
  announce('tea');
  p.show(true);
  refresh(false);
}

// ------------------------------------------------------------ postcard --

var PC_ROWS = ['qwertyuio', 'asdfghjkl', 'pzxcvbnm'];
var PC = { draft: '', msg: '' };

function postcard() {
  enter('postcard');
  var pc = PC;
  var draftT = null;
  var countT = null;
  var p = page('postcard');
  function refresh() {
    draftT.set(pc.draft === '' ? '(empty)' : pc.draft);
    countT.set(pc.draft.length + '/63' +
               (pc.msg === '' ? '' : '   ' + pc.msg));
    p.update();
  }
  function put(ch) {
    if (pc.draft.length < 63) { pc.draft += ch; pc.msg = ''; refresh(); }
  }
  var body = col().pad(10, 24, 0, 24).gap(8);
  body.add(text('today, one line:').size('xs').gray(96));
  draftT = text(' ').size('xs');
  body.add(draftT);
  countT = text(' ').size('xs').gray(128);
  body.add(countT);
  body.add(divider(1, 0));
  var r, i;
  for (r = 0; r < 3; r++) {
    var line = row().gap(2).mainAlign(1);
    for (i = 0; i < PC_ROWS[r].length; i++) {
      (function (ch) {
        line.add(text(ch).size('lg').center().width(52).height(56)
          .onTap(function () { put(ch); }));
      })(PC_ROWS[r].charAt(i));
    }
    if (r === 2) {
      line.add(text('<del>').size('xs').center().width(70).height(52)
        .onTap(function () {
          pc.draft = pc.draft.slice(0, -1); refresh(); }));
    }
    body.add(line);
    body.add(divider(1, 200));
  }
  var last = row().gap(2).mainAlign(1);
  last.add(text(',').size('lg').center().width(52).height(56)
    .onTap(function () { put(','); }));
  last.add(text('space').size('xs').center().width(200).height(56)
    .onTap(function () { put(' '); }));
  last.add(text(' SEAL ').size('xs').invert().center().width(90).height(56)
    .onTap(function () {
      if (pc.draft === '') { pc.msg = 'nothing to seal'; }
      else if (appendPostcard(pc.draft) === 0) {
        pc.msg = 'sealed.'; pc.draft = '';
      } else { pc.msg = 'no card?'; }
      refresh();
    }));
  body.add(last);
  p.header(chrome('POSTCARD')).content(body)
    .footer(hintFooter('seal = save to postcard.txt - no edits'));
  announce('postcard');
  p.show(true);
  refresh();
}

// --------------------------------------------------------------- daily --

var DP = { idx: 0, revealed: false };

function daily() {
  enter('daily');
  var dp = DP;
  var ui = { lines: [] };
  var p = page('daily');
  function shuffle() {
    var n = libraryCount();
    if (n === 0) { return false; }
    dp.idx = Math.floor(Math.random() * n);
    dp.revealed = false;
    if (bookOpen(dp.idx) !== 0) { return false; }
    var skips = Math.floor(Math.random() * 12);
    var i;
    for (i = 0; i < skips; i++) { bookNext(); }
    return true;
  }
  function refresh() {
    var n = bookLineCount();
    if (n > 13) { n = 13; }
    var i;
    for (i = 0; i < ui.lines.length; i++) {
      ui.lines[i].set(i < n ? bookLine(i) : '');
    }
    ui.title.set(dp.revealed ? bookTitle() : 'which book is this?');
    p.update();
  }
  if (!shuffle()) { library(); return; }
  var body = col().pad(10, 40, 0, 40).gap(2);
  var i;
  for (i = 0; i < 13; i++) {
    var line = text(' ');
    ui.lines.push(line);
    body.add(line);
  }
  body.add(spacer(10, 0));
  body.add(divider(1, 176));
  ui.title = text(' ').size('xs').center();
  body.add(ui.title);
  var btns = row().pad(6, 0, 0, 0).gap(16).mainAlign(1);
  btns.add(text('[ reveal ]').size('xs').center().width(120).height(56)
    .onTap(function () { dp.revealed = true; refresh(); }));
  btns.add(text('[ another ]').size('xs').center().width(130).height(56)
    .onTap(function () { shuffle(); refresh(); }));
  btns.add(text('[ keep reading ]').size('xs').center().width(170)
    .height(56).onTap(function () { reader(dp.idx); }));
  body.add(btns);
  p.header(chrome('DAILY PULP')).content(body)
    .footer(hintFooter('a page at random from your shelf'));
  announce('daily');
  p.show(true);
  refresh();
}

// ---------------------------------------------------------------- boot --

print('PULP OS v2 booting, abi v' + abiVersion());
home();
