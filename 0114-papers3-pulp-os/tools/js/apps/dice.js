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
  var body = col().pad(16, M, 8, M).gap(12);
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

