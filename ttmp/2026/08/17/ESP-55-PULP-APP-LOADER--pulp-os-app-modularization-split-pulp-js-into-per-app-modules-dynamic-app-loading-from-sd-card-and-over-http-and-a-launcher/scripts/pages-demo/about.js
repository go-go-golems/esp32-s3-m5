// demo about page
({
  title: 'About',
  main: function (ui, nav) {
    var p = page('about').header(ui.chrome('ABOUT PAGES'))
      .content(col().pad(20, ui.M, 0, ui.M).gap(10).add(
        text('this page is a JS script').size('md'),
        text('it can draw and navigate.').size('sm').gray(64),
        text('files, http, wifi, store: all denied.').size('sm').gray(64),
        ui.row('menu', 'nav.go absolute-relative', function () {
          nav.go('menu.js');
        })))
      .footer(ui.hintFooter('swipe down = home'));
    p.show(true);
  }
})
