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
  var body = col().pad(18, M, 8, M).gap(14)
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
      if (t.left <= 0) {
        t.run = false; t.done = true;
        buzzer.melody('880:150,0:60,1109:150,0:60,1319:300');
        refresh(true);
      }
      else { refresh(false); }
    }
  });
  p.every(1000);
  announce('tea');
  p.show(true);
  refresh(false);
}

