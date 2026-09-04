// ----------------------------------------------------------------- tea --
({
  id: 'tea',
  title: 'Tea Timer',
  subtitle: 'steep watch',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var TEA_KINDS = [['green', 120], ['black', 240], ['herbal', 360]];
    var t = os.state('tea', function () {
      var k0 = storeGet('teakind', 1);
      return { kind: k0, total: TEA_KINDS[k0][1], left: TEA_KINDS[k0][1],
               run: false, done: false };
    });
    var ui = {};
    var p = page('tea');
    function refresh(full) {
      ui.kind.set(TEA_KINDS[t.kind][0].toUpperCase());
      ui.time.set(t.done ? 'READY' : os.fmtClock(t.left * 1000));
      ui.bar.progress(Math.floor(1000 - (t.left * 1000 / t.total)));
      ui.pause.set(t.run ? 'pause' : 'start');
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
    var body = col().pad(18, os.M, 8, os.M).gap(14)
      .add(ui.kind, ui.time, ui.bar, divider(1, 176));
    var kinds = os.buttonRow();
    var i;
    for (i = 0; i < 3; i++) {
      (function (idx) {
        kinds.add(os.button(TEA_KINDS[idx][0] + ' ' +
                            os.fmtClock(TEA_KINDS[idx][1] * 1000),
                            function () { pick(idx); },
                            { w: 142, size: 'sm' }));
      })(i);
    }
    body.add(kinds);
    var btns = os.buttonRow();
    btns.add(os.button('+30s', function () {
      t.left += 30; t.done = false; refresh(false); }, { w: 130 }));
    ui.pause = os.button(' ', function () {
      t.run = !t.run && t.left > 0; refresh(false); }, { w: 130 });
    btns.add(ui.pause);
    btns.add(os.button('reset', function () {
      t.left = t.total; t.run = false; t.done = false; refresh(false);
    }, { w: 130 }));
    body.add(btns);
    p.header(os.chrome('TEA TIMER')).content(body)
      .footer(os.hintFooter('page blinks when the tea is ready'));
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
    os.announce('tea');
    p.show(true);
    refresh(false);
  }
})
