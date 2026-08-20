// -------------------------------------------------------------- reader --

var RD = { lines: [], title: null, foot: null, turns: 0 };

function readerUpdate(p) {
  var n = bookLineCount();
  var i;
  for (i = 0; i < RD.lines.length; i++) {
    RD.lines[i].set(i < n ? bookLine(i) : '');
  }
  RD.title.set(bookTitle() + '  [pulp reader]');
  RD.foot.set(Math.floor(bookProgress() / 10) + '%  turns ' + RD.turns);
  p.update();
}

function reader(idx) {
  if (bookOpen(idx) !== 0) { library(); return; }
  enter('reader');
  RD.lines = [];
  RD.turns = 0;
  RD.title = text('').size('xs');
  var header = col().pad(14, M, 4, M).gap(6)
    .add(RD.title, divider(1, 0));
  var body = col().pad(4, M, 0, M).gap(2);
  var i;
  for (i = 0; i < 24; i++) {
    var line = text(' ');
    RD.lines.push(line);
    body.add(line);
  }
  RD.foot = text('').size('xs').gray(96);
  var footer = col().pad(4, M, 10, M).gap(6)
    .add(divider(1, 0), RD.foot);
  var p = page('reader').header(header).content(body).footer(footer);
  function turn(fwd) {
    var moved = fwd ? bookNext() : bookPrev();
    if (moved === 0) { RD.turns = RD.turns + 1; readerUpdate(p); }
  }
  p.on(G.LEFT, function () { turn(true); });
  p.on(G.RIGHT, function () { turn(false); });
  p.on(G.TAP, function (k, x, y) { turn(x >= 270); });
  announce('reader');
  p.show(true);
  readerUpdate(p);
}

