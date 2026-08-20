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
  var zb = col().pad(20, M, 20, M).gap(6).add(ui.bt, ui.bl)
    .onTap(function () { hit(2); });
  var zw = col().pad(20, M, 20, M).gap(6).add(ui.wt, ui.wl)
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

