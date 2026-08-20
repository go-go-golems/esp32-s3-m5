// Books: the headless book service — open, page, progress.
({
  id: 'd-books',
  title: 'Books',
  subtitle: 'the reader API',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var n = libraryCount();
    if (n === 0) { n = libraryRescan(); }
    var p = page('d-books');
    if (n <= 0 || bookOpen(0) !== 0) {
      var body0 = os.body(20).gap(8).add(
        text('no books on the card').size('md').gray(96),
        os.button('demos', function () { os.launch('demos'); },
                  { w: 130 }));
      p.header(os.chrome('BOOKS')).content(body0)
        .footer(os.hintFooter('drop a .txt into /sdcard/books'));
      os.announce('d-books');
      p.show(true);
      return;
    }
    var lines = [];
    var head = text(' ').size('sm');
    var foot = text(' ').size('xs').gray(96);
    function refresh() {
      var c = bookLineCount();
      if (c > 10) { c = 10; }
      var i;
      for (i = 0; i < lines.length; i++) {
        lines[i].set(i < c ? bookLine(i) : ' ');
      }
      head.set(bookTitle() + '  (' + n + ' book(s) on the shelf)');
      foot.set('progress ' + Math.floor(bookProgress() / 10) + '%');
      p.update();
    }
    var body = os.body(8).gap(4);
    body.add(head);
    body.add(divider(1, 0));
    var i;
    for (i = 0; i < 10; i++) {
      var t = text(' ').size('md');
      lines.push(t);
      body.add(t);
    }
    body.add(divider(1, 0));
    body.add(foot);
    var btns = os.buttonRow();
    btns.add(os.button('prev', function () {
      if (bookPrev() === 0) { refresh(); } }, { w: 110 }));
    btns.add(os.button('next', function () {
      if (bookNext() === 0) { refresh(); } }, { w: 110 }));
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 130 }));
    body.add(btns);
    p.header(os.chrome('BOOKS')).content(body)
      .footer(os.hintFooter('bookOpen/Line/Next/Prev/Progress'));
    os.announce('d-books');
    p.show(true);
    refresh();
  }
})
