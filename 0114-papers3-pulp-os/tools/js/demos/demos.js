// Demo suite index (ESP-57): the one visible row. Lists every installed
// d-* demo from the merged catalog and launches them by id.
({
  id: 'demos',
  title: 'Demos',
  subtitle: 'the JS API, live',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var menu = list().pad(4, 0, 0, 0);
    var c = catalog();
    var n = 0;
    var i;
    for (i = 0; i < c.length; i++) {
      (function (e) {
        if (e.id.indexOf('d-') !== 0) { return; }
        n = n + 1;
        os.menuRow(menu, e.title, e.subtitle || '', function () {
          os.launch(e.id);
        });
      })(c[i]);
    }
    if (n === 0) {
      menu.add(os.body(20).add(
        text('no demos installed').size('md').gray(96),
        text('push them: scripts/02-push-demos.sh').size('xs').gray(128)));
    }
    var p = page('demos').header(os.chrome('DEMOS')).content(menu)
      .footer(os.hintFooter(n + ' demos - tap to open - swipe down = home'));
    os.announce('demos');
    p.show(true);
  }
})
