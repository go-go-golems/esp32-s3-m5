// demo menu page
({
  title: 'Menu',
  main: function (ui, nav) {
    var p = page('menu').header(ui.chrome('PULP PAGES'))
      .content(col().pad(4, 0, 0, 0).gap(0).add(
        ui.row('Clock', 'a live dyn-text page', function () {
          nav.go('clock.js');
        }),
        ui.row('About', 'what pages are', function () {
          nav.go('about.js');
        }),
        ui.row('Reload', 'fetch this menu again', function () {
          nav.reload();
        })))
      .footer(ui.hintFooter('served over http - swipe down = home'));
    p.show(true);
  }
})
