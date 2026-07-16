// PULP OS - "the paperback of computers".
// One bytecode image: launcher + apps, ES5-stricter dialect, compiled by
// tools/js/s3jsc.c. Gesture kinds: 0 tap, 1 long-press, 2 swipe-left,
// 3 swipe-right, 4 swipe-up, 5 swipe-down (home), 100 interval timer.

var P = { app: 'home', g: null, trapDown: false };

function pad2(n) { return (n < 10 ? '0' : '') + n; }
function fmtClock(ms) {
  if (ms < 0) { ms = 0; }
  var s = Math.floor(ms / 1000);
  return pad2(Math.floor(s / 60)) + ':' + pad2(s % 60);
}

// Every present re-registers the OS gesture wrapper (s3.reset clears it):
// swipe-down goes home unless the app traps it; kind 100 is the timer.
function present(slots) {
  s3._onGesture = function (k, x, y, hit) {
    if (k === 5 && !P.trapDown && P.app !== 'home') { home(); return true; }
    if (P.g) { return P.g(k, x, y, hit); }
    return false;
  };
  print('pulp screen: ' + P.app);
  s3.render(slots);
}

function chrome(title) {
  return s3.col().pad(16, 40, 6, 40).gap(8)
    .add(s3.text(title).font(s3.FONT_DISPLAY), s3.divider(6, 0));
}

function hintFooter(hint) {
  return s3.col().pad(4, 40, 10, 40).gap(5)
    .add(s3.divider(1, 0), s3.text(hint).font(s3.FONT_UI).gray(96));
}

// ---------------------------------------------------------------- home --

function home() {
  P.app = 'home'; P.g = null; P.trapDown = false;
  s3TimerStop();
  s3.reset();
  var header = s3.col().pad(16, 40, 6, 40).gap(4).add(
    s3.text('PULP').font(s3.FONT_XL),
    s3.text('THE PAPERBACK OF COMPUTERS').font(s3.FONT_UI).gray(96),
    s3.divider(8, 0));
  var list = s3.list().pad(4, 0, 0, 0);
  function row(label, sub, fn) {
    var line = s3.row().pad(8, 40, 6, 40).gap(10).crossAlign(3)
      .add(s3.text(label).font(s3.FONT_DISPLAY), s3.spacer(0, 1),
           s3.text(sub).font(s3.FONT_UI).gray(112));
    var r = s3.col().add(line, s3.divider(2, 0)).onTap(fn);
    list.add(r);
  }
  row('Reader', 'your last book', function () { s3OpenBook(-1); });
  row('Dice Tray', '2d6 coin d20 d%', dice);
  row('Blitz Ink', 'chess clock 5+3', blitz);
  row('2048 INK', 'best ' + s3StoreGet('2048best', 0), g2048);
  row('Tea Timer', 'steep watch', tea);
  row('Postcard', 'one line a day', postcard);
  row('Daily Pulp', 'a page at random', daily);
  present({ header: header, content: list,
            footer: hintFooter('tap to open - swipe down = home anywhere'),
            full: true });
}

// ---------------------------------------------------------------- dice --

var DICE_PIPS = [
  ['       ', '   *   ', '       '],
  ['*      ', '       ', '      *'],
  ['*      ', '   *   ', '      *'],
  ['*     *', '       ', '*     *'],
  ['*     *', '   *   ', '*     *'],
  ['*     *', '*     *', '*     *']];

function dice() {
  P.app = 'dice'; P.g = null; P.trapDown = false;
  if (!P.dice) { P.dice = { mode: '2d6', a: 3, b: 4, big: '', hist: [] }; }
  var d = P.dice;
  function roll(mode) {
    d.mode = mode;
    var text = '';
    if (mode === '2d6') {
      d.a = 1 + Math.floor(Math.random() * 6);
      d.b = 1 + Math.floor(Math.random() * 6);
      text = '' + (d.a + d.b);
    } else if (mode === 'd20') {
      text = '' + (1 + Math.floor(Math.random() * 20));
    } else if (mode === 'coin') {
      text = Math.random() < 0.5 ? 'heads' : 'tails';
    } else {
      text = '' + (1 + Math.floor(Math.random() * 100));
    }
    d.big = text;
    d.hist.unshift(text);
    if (d.hist.length > 5) { d.hist.pop(); }
    render(false);
  }
  function die(v) {
    var c = s3.col().pad(6, 10, 6, 10).gap(2);
    var i;
    for (i = 0; i < 3; i++) { c.add(s3.text(DICE_PIPS[v - 1][i]).center()); }
    return c;
  }
  function render(full) {
    s3.reset();
    var body = s3.col().pad(16, 40, 8, 40).gap(12);
    if (d.mode === '2d6') {
      body.add(s3.row().gap(24).mainAlign(1).add(die(d.a), die(d.b)));
      body.add(s3.text('' + (d.a + d.b)).font(s3.FONT_XL).center());
    } else {
      body.add(s3.spacer(30, 0));
      body.add(s3.text(d.big === '' ? 'ROLL' : d.big)
        .font(s3.FONT_XL).center());
      body.add(s3.spacer(30, 0));
    }
    body.add(s3.divider(1, 176));
    var btns = s3.row().pad(10, 0, 10, 0).gap(8).mainAlign(1);
    function btn(label, fn) {
      btns.add(s3.text(label).font(s3.FONT_DISPLAY).onTap(fn));
    }
    btn('2d6', function () { roll('2d6'); });
    btn('d20', function () { roll('d20'); });
    btn('coin', function () { roll('coin'); });
    btn('d%', function () { roll('d%'); });
    body.add(btns);
    body.add(s3.text('history: ' + d.hist.join(' - '))
      .font(s3.FONT_UI).gray(96));
    present({ header: chrome('DICE TRAY'), content: body,
              footer: hintFooter('tap a roll - swipe down = home'),
              full: full });
  }
  render(true);
}

// --------------------------------------------------------------- blitz --

function blitz() {
  P.app = 'blitz'; P.trapDown = false;
  if (!P.bz) {
    P.bz = { w: 300000, b: 300000, inc: 3000, run: 0, last: 0, moves: 0 };
  }
  var z = P.bz;
  function settle() {
    if (z.run !== 0) {
      var now = millis();
      var elapsed = now - z.last;
      z.last = now;
      if (z.run === 1) { z.w -= elapsed; } else { z.b -= elapsed; }
    }
  }
  function hit(side) {
    settle();
    if (z.run === side) {
      if (side === 1) { z.w += z.inc; } else { z.b += z.inc; }
      z.run = side === 1 ? 2 : 1;
      z.moves++;
    } else if (z.run === 0) {
      z.run = side === 1 ? 2 : 1;
      z.last = millis();
    }
    render(false);
  }
  function zone(name, ms, side) {
    var mine = z.run === side;
    return s3.col().pad(20, 40, 20, 40).gap(6)
      .add(s3.text(fmtClock(ms)).font(s3.FONT_XL).center()
             .gray(mine ? 0 : 144),
           s3.text((mine ? '> ' : '') + name.toUpperCase())
             .font(s3.FONT_UI).center().gray(mine ? 0 : 144))
      .onTap(function () { hit(side); });
  }
  P.g = function (k) {
    if (k === 1) { settle(); z.run = 0; render(false); return true; }
    if (k === 100) {
      settle();
      if (z.w <= 0 || z.b <= 0) { z.run = 0; s3TimerStop(); render(true); }
      else { render(false); }
      return true;
    }
    return false;
  };
  function render(full) {
    s3.reset();
    var flag = z.w <= 0 ? 'WHITE FLAGS' : (z.b <= 0 ? 'BLACK FLAGS' : '');
    var body = s3.col().pad(8, 0, 0, 0).gap(4).add(
      zone('black', z.b, 2),
      s3.divider(6, 0),
      s3.text(flag === '' ? 'MOVES ' + z.moves : flag)
        .font(s3.FONT_DISPLAY).center(),
      s3.divider(6, 0),
      zone('white', z.w, 1));
    present({ header: chrome('BLITZ INK 5+3'), content: body,
              footer: hintFooter('tap your side - hold = pause'),
              full: full });
  }
  s3TimerStart(1000);
  render(true);
}

// --------------------------------------------------------------- 2048 ---

function g2048() {
  P.app = '2048'; P.trapDown = true;
  if (!P.gg) { P.gg = { g: null, score: 0, prev: null, pscore: 0 }; }
  var st = P.gg;
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
  // Slides one row of 4 leftward; returns [cells, gained, moved].
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
  function move(dir) {  // 0 left, 1 right, 2 up, 3 down
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
      if (st.score > s3StoreGet('2048best', 0)) {
        s3StoreSet('2048best', st.score);
      }
      render(false);
    }
  }
  P.g = function (k) {
    if (k === 2) { move(0); return true; }
    if (k === 3) { move(1); return true; }
    if (k === 4) { move(2); return true; }
    if (k === 5) { move(3); return true; }
    return false;
  };
  function render(full) {
    s3.reset();
    var body = s3.col().pad(10, 30, 4, 30).gap(0);
    body.add(s3.divider(2, 0));
    var r, c, v;
    for (r = 0; r < 4; r++) {
      var line = s3.row().gap(0).mainAlign(1);
      for (c = 0; c < 4; c++) {
        v = st.g[r * 4 + c];
        line.add(s3.text(v === 0 ? '.' : '' + v).font(s3.FONT_DISPLAY)
          .center().width(118).height(64).gray(v >= 128 ? 0 : 112));
      }
      body.add(line);
      body.add(s3.divider(2, 0));
    }
    body.add(s3.spacer(8, 0));
    body.add(s3.text('SCORE ' + st.score + '   BEST ' +
                     s3StoreGet('2048best', 0))
      .font(s3.FONT_DISPLAY).center());
    var btns = s3.row().pad(8, 0, 0, 0).gap(20).mainAlign(1);
    btns.add(s3.text('[ new game ]').font(s3.FONT_UI)
      .onTap(function () { fresh(); render(true); }));
    btns.add(s3.text('[ undo ]').font(s3.FONT_UI)
      .onTap(function () {
        if (st.prev) { st.g = st.prev; st.score = st.pscore;
                       st.prev = null; render(false); }
      }));
    btns.add(s3.text('[ home ]').font(s3.FONT_UI).onTap(home));
    body.add(btns);
    present({ header: chrome('2048 INK'), content: body,
              footer: hintFooter('swipe to slide tiles - home via button'),
              full: full });
  }
  render(true);
}

// ----------------------------------------------------------------- tea --

var TEA_KINDS = [['green', 120], ['black', 240], ['herbal', 360]];

function tea() {
  P.app = 'tea'; P.trapDown = false;
  if (!P.tea) {
    var k = s3StoreGet('teakind', 1);
    P.tea = { kind: k, total: TEA_KINDS[k][1], left: TEA_KINDS[k][1],
              run: false, done: false };
  }
  var t = P.tea;
  function pick(i) {
    t.kind = i; t.total = TEA_KINDS[i][1]; t.left = t.total;
    t.run = true; t.done = false;
    s3StoreSet('teakind', i);
    render(false);
  }
  P.g = function (k) {
    if (k !== 100) { return false; }
    if (t.run && t.left > 0) {
      t.left -= 1;
      if (t.left <= 0) { t.run = false; t.done = true; render(true); }
      else if (t.left % 5 === 0) { render(false); }
      return true;
    }
    return true;
  };
  function render(full) {
    s3.reset();
    var body = s3.col().pad(18, 40, 8, 40).gap(14);
    body.add(s3.text(TEA_KINDS[t.kind][0].toUpperCase())
      .font(s3.FONT_DISPLAY).center());
    body.add(s3.text(t.done ? 'READY' : fmtClock(t.left * 1000))
      .font(s3.FONT_XL).center());
    var bar = s3.progressBar(
      Math.floor(1000 - (t.left * 1000 / t.total)), 24).height(24);
    body.add(bar);
    body.add(s3.divider(1, 176));
    var kinds = s3.row().pad(6, 0, 0, 0).gap(18).mainAlign(1);
    var i;
    for (i = 0; i < 3; i++) {
      (function (idx) {
        kinds.add(s3.text(TEA_KINDS[idx][0] + ' ' +
                          fmtClock(TEA_KINDS[idx][1] * 1000))
          .font(s3.FONT_UI).onTap(function () { pick(idx); }));
      })(i);
    }
    body.add(kinds);
    var btns = s3.row().pad(6, 0, 0, 0).gap(24).mainAlign(1);
    btns.add(s3.text('[ +30s ]').font(s3.FONT_UI).onTap(function () {
      t.left += 30; t.done = false; render(false); }));
    btns.add(s3.text(t.run ? '[ pause ]' : '[ start ]').font(s3.FONT_UI)
      .onTap(function () { t.run = !t.run && t.left > 0; render(false); }));
    btns.add(s3.text('[ reset ]').font(s3.FONT_UI).onTap(function () {
      t.left = t.total; t.run = false; t.done = false; render(false); }));
    body.add(btns);
    present({ header: chrome('TEA TIMER'), content: body,
              footer: hintFooter('page blinks when the tea is ready'),
              full: full });
  }
  s3TimerStart(1000);
  render(true);
}

// ------------------------------------------------------------ postcard --

var PC_ROWS = ['qwertyuio', 'asdfghjkl', 'pzxcvbnm'];

function postcard() {
  P.app = 'postcard'; P.g = null; P.trapDown = false;
  if (!P.pc) { P.pc = { draft: '', msg: '' }; }
  var pc = P.pc;
  function put(ch) {
    if (pc.draft.length < 63) { pc.draft += ch; pc.msg = ''; render(false); }
  }
  function render(full) {
    s3.reset();
    var body = s3.col().pad(10, 24, 0, 24).gap(8);
    body.add(s3.text('today, one line:').font(s3.FONT_UI).gray(96));
    body.add(s3.text(pc.draft === '' ? '(empty)' : pc.draft)
      .font(s3.FONT_UI));
    body.add(s3.text(pc.draft.length + '/63' +
                     (pc.msg === '' ? '' : '   ' + pc.msg))
      .font(s3.FONT_UI).gray(128));
    body.add(s3.divider(1, 0));
    var r, i;
    for (r = 0; r < 3; r++) {
      var line = s3.row().gap(2).mainAlign(1);
      for (i = 0; i < PC_ROWS[r].length; i++) {
        (function (ch) {
          line.add(s3.text(ch).font(s3.FONT_DISPLAY).center()
            .width(52).height(56).onTap(function () { put(ch); }));
        })(PC_ROWS[r].charAt(i));
      }
      if (r === 2) {
        line.add(s3.text('<del>').font(s3.FONT_UI).center()
          .width(70).height(52).onTap(function () {
            pc.draft = pc.draft.slice(0, -1); render(false); }));
      }
      body.add(line);
      body.add(s3.divider(1, 200));
    }
    var last = s3.row().gap(2).mainAlign(1);
    last.add(s3.text(',').center().width(50).height(52)
      .onTap(function () { put(','); }));
    last.add(s3.text('space').font(s3.FONT_UI).center().width(200).height(52)
      .onTap(function () { put(' '); }));
    last.add(s3.text('SEAL').font(s3.FONT_UI).center().width(90).height(52)
      .onTap(function () {
        if (pc.draft === '') { pc.msg = 'nothing to seal'; }
        else if (s3AppendPostcard(pc.draft) === 0) {
          pc.msg = 'sealed.'; pc.draft = '';
        } else { pc.msg = 'no card?'; }
        render(false);
      }));
    body.add(last);
    present({ header: chrome('POSTCARD'), content: body,
              footer: hintFooter('seal = save to postcard.txt - no edits'),
              full: full });
  }
  render(true);
}

// --------------------------------------------------------------- daily --

function daily() {
  P.app = 'daily'; P.g = null; P.trapDown = false;
  if (!P.dp) { P.dp = { idx: -1, revealed: false }; }
  var dp = P.dp;
  function shuffle() {
    var n = s3LibraryCount();
    dp.idx = Math.floor(Math.random() * (n + 1)) - 1;
    dp.revealed = false;
    if (s3BookOpen(dp.idx) !== 0) { dp.idx = -1; s3BookOpen(-1); }
    var skips = Math.floor(Math.random() * 12);
    var i;
    for (i = 0; i < skips; i++) { s3BookNext(); }
  }
  function render(full) {
    s3.reset();
    var body = s3.col().pad(10, 40, 0, 40).gap(2);
    var n = s3BookLineCount();
    if (n > 13) { n = 13; }
    var i;
    for (i = 0; i < n; i++) { body.add(s3.text(s3BookLine(i))); }
    body.add(s3.spacer(10, 0));
    body.add(s3.divider(1, 176));
    body.add(s3.text(dp.revealed ? s3BookTitle() : 'which book is this?')
      .font(s3.FONT_UI).center());
    var btns = s3.row().pad(6, 0, 0, 0).gap(16).mainAlign(1);
    btns.add(s3.text('[ reveal ]').font(s3.FONT_UI).onTap(function () {
      dp.revealed = true; render(false); }));
    btns.add(s3.text('[ another ]').font(s3.FONT_UI).onTap(function () {
      shuffle(); render(false); }));
    btns.add(s3.text('[ keep reading ]').font(s3.FONT_UI)
      .onTap(function () { s3OpenBook(dp.idx); }));
    body.add(btns);
    present({ header: chrome('DAILY PULP'), content: body,
              footer: hintFooter('a page at random from your shelf'),
              full: full });
  }
  shuffle();
  render(true);
}

// ---------------------------------------------------------------- boot --

print('PULP OS booting, abi v' + s3Version());
home();
