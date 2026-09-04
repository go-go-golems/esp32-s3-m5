// ------------------------------------------------------------- library --
({
  id: 'library',
  title: 'Reader',
  subtitle: 'books on the card',
  version: 1,
  abi: 2,
  main: function (os, arg) {
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
        menu.add(col().pad(0, os.M, 0, os.M).add(
          row().pad(8, 0, 6, 0).gap(10).crossAlign(3)
            .add(text(name).size('title'), spacer(0, 1),
                 text(kb).size('xs').gray(112)),
          divider(1, 176)).onTap(function () { os.launch('reader', idx); }));
      })(i);
    }
    if (n === 0) {
      menu.add(col().pad(20, os.M, 0, os.M)
        .add(text('no books on the card').size('sm').gray(96)));
    }
    var p = page('library').header(os.chrome('LIBRARY')).content(menu)
      .footer(os.hintFooter('tap a book - swipe down = home'));
    os.announce('library');
    p.show(true);
  }
})
