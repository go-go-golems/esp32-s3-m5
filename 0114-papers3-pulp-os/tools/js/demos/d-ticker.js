// Ticker: page.every + dyn text + diff updates vs clean fulls.
({
  id: 'd-ticker',
  title: 'Ticker',
  subtitle: 'every(ms) + dyn text',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var st = { t0: millis(), ticks: 0, fulls: 0 };
    var p = page('d-ticker');
    var clock = text(function () {
      return os.fmtClock(millis() - st.t0);
    }).size('xl').center();
    var info = text(function () {
      return 'ticks ' + st.ticks + '   diff updates only';
    }).size('xs').gray(96).center();
    var body = os.body(24).gap(14).add(
      os.label('uptime on this screen'),
      clock,
      progressBar(0, 18).height(18),
      info,
      divider(1, 0),
      text('the tick re-evaluates dyn texts; unchanged strings cost')
        .size('sm').gray(64),
      text('nothing; one diff present per second at most.').size('sm')
        .gray(64));
    var bar = body;
    var btns = os.buttonRow();
    btns.add(os.button('clean full', function () {
      st.fulls = st.fulls + 1;
      p.show(true);
    }, { w: 150 }));
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 130 }));
    body.add(btns);
    p.header(os.chrome('TICKER')).content(body)
      .footer(os.hintFooter('one second, one diff blit'));
    p.on(G.TICK, function () { st.ticks = st.ticks + 1; });
    p.every(1000);
    os.announce('d-ticker');
    p.show(true);
  }
})
