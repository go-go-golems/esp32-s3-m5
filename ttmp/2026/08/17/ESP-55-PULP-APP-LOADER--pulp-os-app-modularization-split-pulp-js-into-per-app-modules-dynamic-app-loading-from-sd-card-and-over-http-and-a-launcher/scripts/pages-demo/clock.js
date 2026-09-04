// demo clock page: dyn text + tick inside the sandbox
({
  title: 'Clock',
  main: function (ui, nav) {
    var t0 = millis();
    var p = page('clock').header(ui.chrome('PAGE CLOCK'))
      .content(col().pad(40, ui.M, 0, ui.M).gap(16).add(
        text(function () {
          var s = Math.floor((millis() - t0) / 1000);
          return 'on page ' + s + 's';
        }).size('xl').center(),
        ui.row('back to menu', 'nav.back()', function () { nav.back(); })))
      .footer(ui.hintFooter('ticks once a second - swipe down = home'));
    p.every(1000);
    p.show(true);
  }
})
