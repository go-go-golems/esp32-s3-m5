// -------------------------------------------------------------- reader --
({
  id: 'reader',
  title: 'Reader page',
  subtitle: 'the open book',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var idx = typeof arg === 'number' ? arg : 0;
    if (bookOpen(idx) !== 0) { os.launch('library'); return; }
    var rd = { lines: [], title: null, foot: null, turns: 0 };
    function readerUpdate(p) {
      var n = bookLineCount();
      var i;
      for (i = 0; i < rd.lines.length; i++) {
        rd.lines[i].set(i < n ? bookLine(i) : '');
      }
      rd.title.set(bookTitle() + '  [pulp reader]');
      rd.foot.set(Math.floor(bookProgress() / 10) + '%  turns ' + rd.turns);
      p.update();
    }
    rd.title = text('').size('xs');
    var header = col().pad(14, os.M, 4, os.M).gap(6)
      .add(rd.title, divider(1, 0));
    var body = col().pad(4, os.M, 0, os.M).gap(2);
    var i;
    for (i = 0; i < 24; i++) {
      var line = text(' ');
      rd.lines.push(line);
      body.add(line);
    }
    rd.foot = text('').size('xs').gray(96);
    var footer = col().pad(4, os.M, 10, os.M).gap(6)
      .add(divider(1, 0), rd.foot);
    var p = page('reader').header(header).content(body).footer(footer);
    function turn(fwd) {
      var moved = fwd ? bookNext() : bookPrev();
      if (moved === 0) { rd.turns = rd.turns + 1; readerUpdate(p); }
    }
    p.on(G.LEFT, function () { turn(true); });
    p.on(G.RIGHT, function () { turn(false); });
    p.on(G.TAP, function (k, x, y) { turn(x >= 270); });
    os.announce('reader');
    p.show(true);
    readerUpdate(p);
  }
})
