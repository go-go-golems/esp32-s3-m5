// --------------------------------------------------------------- daily --

var DP = { idx: 0, revealed: false };

function daily() {
  enter('daily');
  var dp = DP;
  var ui = { lines: [] };
  var p = page('daily');
  function shuffle() {
    var n = libraryCount();
    if (n === 0) { return false; }
    dp.idx = Math.floor(Math.random() * n);
    dp.revealed = false;
    if (bookOpen(dp.idx) !== 0) { return false; }
    var skips = Math.floor(Math.random() * 12);
    var i;
    for (i = 0; i < skips; i++) { bookNext(); }
    return true;
  }
  function refresh() {
    var n = bookLineCount();
    if (n > 13) { n = 13; }
    var i;
    for (i = 0; i < ui.lines.length; i++) {
      ui.lines[i].set(i < n ? bookLine(i) : '');
    }
    ui.title.set(dp.revealed ? bookTitle() : 'which book is this?');
    p.update();
  }
  if (!shuffle()) { library(); return; }
  var body = col().pad(10, M, 0, M).gap(2);
  var i;
  for (i = 0; i < 13; i++) {
    var line = text(' ');
    ui.lines.push(line);
    body.add(line);
  }
  body.add(spacer(10, 0));
  body.add(divider(1, 176));
  ui.title = text(' ').size('xs').center();
  body.add(ui.title);
  var btns = row().pad(6, 0, 0, 0).gap(16).mainAlign(1);
  btns.add(text('[ reveal ]').size('xs').center().width(120).height(56)
    .onTap(function () { dp.revealed = true; refresh(); }));
  btns.add(text('[ another ]').size('xs').center().width(130).height(56)
    .onTap(function () { shuffle(); refresh(); }));
  btns.add(text('[ keep reading ]').size('xs').center().width(170)
    .height(56).onTap(function () { reader(dp.idx); }));
  body.add(btns);
  p.header(chrome('DAILY PULP')).content(body)
    .footer(hintFooter('a page at random from your shelf'));
  announce('daily');
  p.show(true);
  refresh();
}

