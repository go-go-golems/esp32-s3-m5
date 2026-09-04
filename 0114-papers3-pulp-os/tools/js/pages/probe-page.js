// Sandbox regression page (probe 28): every denied native must throw,
// drawing must work, nav must record.
({
  title: 'Probe Page',
  main: function (ui, nav) {
    function denied(fn) {
      try { fn(); return 'ALLOWED'; } catch (e) { return 'denied'; }
    }
    print('probe-page: files=' + denied(function () { files.exists('/x'); })
      + ' http=' + denied(function () { http.get('http://x'); })
      + ' serve=' + denied(function () { serve.url(); })
      + ' wifi=' + denied(function () { wifi.status(); })
      + ' load=' + denied(function () { load('rom:dice'); })
      + ' reset=' + denied(function () { resetTree(); })
      + ' paper=' + denied(function () { paper.home(function () {}); })
      + ' store=' + denied(function () { storeGet('x', 0); })
      + ' apps=' + denied(function () { apps.count(); })
      + ' browser=' + denied(function () { browser.close(); }));
    print('probe-page: url=' + nav.url() + ' abi=' + abiVersion());
    var p = page('probe').header(ui.chrome('PROBE PAGE'))
      .content(col().pad(20, ui.M, 0, ui.M).gap(10).add(
        ui.row('sandboxed', 'all a page can do is draw',
               function () { nav.go('probe://tapped'); })))
      .footer(ui.hintFooter('swipe down = home'));
    print('pulp screen: page/probe');
    p.show(true);
  }
})
