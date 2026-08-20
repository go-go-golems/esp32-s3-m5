// ---------------------------------------------------------------- dice --
({
  id: 'dice',
  title: 'Dice Tray',
  subtitle: '2d6 coin d20 d%',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var DICE_PIPS = [
      ['       ', '   *   ', '       '],
      ['*      ', '       ', '      *'],
      ['*      ', '   *   ', '      *'],
      ['*     *', '       ', '*     *'],
      ['*     *', '   *   ', '*     *'],
      ['*     *', '*     *', '*     *']];
    var z = os.state('dice', function () {
      return { mode: '2d6', a: 3, b: 4, big: '', hist: [] };
    });
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
        ui.dieA[i].set(z.mode === '2d6' ? DICE_PIPS[z.a - 1][i] : ' ');
        ui.dieB[i].set(z.mode === '2d6' ? DICE_PIPS[z.b - 1][i] : ' ');
      }
      ui.big.set(z.big === '' ? 'ROLL' : z.big);
      ui.hist.set('history: ' + z.hist.join(' - '));
      p.update();
    }
    var body = col().pad(16, os.M, 8, os.M).gap(12);
    body.add(row().gap(24).mainAlign(1).add(die(ui.dieA), die(ui.dieB)));
    ui.big = text('ROLL').size('xl').center();
    body.add(ui.big);
    body.add(divider(1, 176));
    var btns = row().pad(10, 0, 10, 0).gap(8).mainAlign(1);
    var p = page('dice').header(os.chrome('DICE TRAY')).content(body)
      .footer(os.hintFooter('tap a roll - swipe down = home'));
    function roll(mode) {
      z.mode = mode;
      var t = '';
      if (mode === '2d6') {
        z.a = 1 + Math.floor(Math.random() * 6);
        z.b = 1 + Math.floor(Math.random() * 6);
        t = '' + (z.a + z.b);
      } else if (mode === 'd20') {
        t = '' + (1 + Math.floor(Math.random() * 20));
      } else if (mode === 'coin') {
        t = Math.random() < 0.5 ? 'heads' : 'tails';
      } else {
        t = '' + (1 + Math.floor(Math.random() * 100));
      }
      z.big = t;
      z.hist.unshift(t);
      if (z.hist.length > 5) { z.hist.pop(); }
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
    os.announce('dice');
    p.show(true);
    refresh(p);
  }
})
