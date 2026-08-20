// Touch lab: all six gestures + hit-region routing on a 3x3 grid.
// Traps G.DOWN deliberately (documented on-screen; home via button).
({
  id: 'd-touch',
  title: 'Touch Lab',
  subtitle: 'gestures + hit regions',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var st = { last: 'touch me', hits: 0, gestures: 0 };
    var lastT = text(function () { return st.last; }).size('lg');
    var statT = text(function () {
      return 'hits ' + st.hits + '   gestures ' + st.gestures;
    }).size('xs').gray(96);
    var p = page('d-touch');
    function note(what) {
      st.gestures = st.gestures + 1;
      st.last = what;
      p.update();
    }
    var grid = col().gap(4);
    var r, c;
    for (r = 0; r < 3; r++) {
      var line = row().gap(4).mainAlign(1);
      for (c = 0; c < 3; c++) {
        (function (n) {
          line.add(text('' + n).size('lg').center().width(140).height(84)
            .onTap(function (k, x, y) {
              st.hits = st.hits + 1;
              note('cell ' + n + ' @ ' + x + ',' + y);
            }));
        })(r * 3 + c + 1);
      }
      grid.add(line);
      grid.add(divider(1, 200));
    }
    var body = os.body(8).gap(10).add(
      lastT, statT, divider(1, 0),
      text('tap a cell - swipe any direction - long press')
        .size('xs').gray(96),
      grid);
    var btns = os.buttonRow();
    btns.add(os.button('home', function () { os.home(); }, { w: 120 }));
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 120 }));
    body.add(btns);
    p.header(os.chrome('TOUCH LAB')).content(body)
      .footer(os.hintFooter('G.DOWN is trapped here - home via button'));
    p.on(G.LEFT, function (k, x, y) { note('swipe LEFT @ ' + x + ',' + y); });
    p.on(G.RIGHT, function (k, x, y) { note('swipe RIGHT'); });
    p.on(G.UP, function (k, x, y) { note('swipe UP'); });
    p.on(G.DOWN, function (k, x, y) { note('swipe DOWN (trapped)'); });
    p.on(G.LONG, function (k, x, y) { note('LONG press @ ' + x + ',' + y); });
    os.announce('d-touch');
    p.show(true);
  }
})
