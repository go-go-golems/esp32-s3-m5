// ------------------------------------------------------------- library --

function library() {
  enter('library');
  var menu = list().pad(4, 0, 0, 0);
  var n = libraryCount();
  if (n === 0) { n = libraryRescan(); }
  var i;
  for (i = 0; i < n; i++) {
    (function (idx) {
      // Separators share the header rule's 40px margins; titles render
      // at app-row size in the serif face, sizes small at the right.
      var s = libraryLine(idx);
      var cut = s.lastIndexOf('  ');
      var name = cut > 0 ? s.slice(0, cut) : s;
      var kb = cut > 0 ? s.slice(cut + 2) : '';
      menu.add(col().pad(0, M, 0, M).add(
        row().pad(8, 0, 6, 0).gap(10).crossAlign(3)
          .add(text(name).size('title'), spacer(0, 1),
               text(kb).size('xs').gray(112)),
        divider(1, 176)).onTap(function () { reader(idx); }));
    })(i);
  }
  if (n === 0) {
    menu.add(col().pad(20, M, 0, M)
      .add(text('no books on the card').size('sm').gray(96)));
  }
  var p = page('library').header(chrome('LIBRARY')).content(menu)
    .footer(hintFooter('tap a book - swipe down = home'));
  announce('library');
  p.show(true);
}

