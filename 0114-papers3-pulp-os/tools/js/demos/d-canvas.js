// Canvas: the five draw verbs, labeled. Tap redraws with new randomness.
({
  id: 'd-canvas',
  title: 'Canvas',
  subtitle: 'line disc ring box paint',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var W = 540 - 2 * os.M;
    var cv = canvas().height(700);
    function draw() {
      cv.wipe();
      var i;
      cv.line(0, 20, W, 20, 0, 2);
      for (i = 0; i < 5; i++) {
        cv.line(10 + i * 30, 40, 100 + i * 30,
                110, i * 48, 1 + i);
      }
      for (i = 0; i < 5; i++) {
        cv.disc(40 + i * 90, 190, 12 + i * 6, i * 48);
      }
      for (i = 0; i < 5; i++) {
        cv.ring(40 + i * 90, 300, 14 + i * 6, i * 40, 2 + i);
      }
      for (i = 0; i < 5; i++) {
        cv.box(16 + i * 88, 380, 64, 54, i * 48, 1 + i);
      }
      for (i = 0; i < 16; i++) {
        cv.paint(Math.floor(i * (W / 16)), 480,
                 Math.floor(W / 16), 44, i * 17);
      }
      cv.line(0, 560, W, 560, 0, 1);
      for (i = 0; i < 24; i++) {
        cv.disc(20 + Math.floor(Math.random() * (W - 40)),
                590 + Math.floor(Math.random() * 90),
                4 + Math.floor(Math.random() * 14),
                (i * 37) % 256);
      }
    }
    paper.refreshTurns(1);
    var body = os.body(6).gap(6).add(
      cv,
      text('line / disc / ring / box / paint(16 grays) - tap = redraw')
        .size('xs').gray(96).center());
    var p = page('d-canvas').header(os.chrome('CANVAS')).content(body)
      .footer(os.hintFooter('tap = new randomness - swipe down = home'));
    p.on(G.TAP, function () { draw(); p.show(1); });
    os.announce('d-canvas');
    draw();
    p.show(true);
  }
})
