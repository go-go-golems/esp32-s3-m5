// --------------------------------------------------------------- radio --
({
  id: 'radio',
  title: 'Radio',
  subtitle: 'words from the ether',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var RA_LINES = 7;
    var ra = os.state('radio', function () {
      return { q: '', a: '', msg: 'tap to tune in' };
    });
    var ui = {};
    var p = page('radio');
    // Poster typography: the quote fills the page in the 84px bold face,
    // black on plain white, word-wrapped onto up to 7 short lines (the
    // display is a poster, the shelf copy is the full text).
    function radioShow() {
      var words = (ra.q === '' ? 'RADIO' : ra.q).split(' ');
      var lines = [];
      var i;
      for (i = 0; i < RA_LINES; i++) { lines.push(''); }
      var li = 0;
      for (i = 0; i < words.length && li < RA_LINES; i++) {
        if (lines[li] !== '' && lines[li].length + words[i].length + 1 > 10) {
          li++;
          if (li === RA_LINES) { lines[RA_LINES - 1] += '...'; break; }
        }
        lines[li] += (lines[li] === '' ? '' : ' ') + words[i];
      }
      for (i = 0; i < RA_LINES; i++) {
        ui.q[i].set(lines[i] === '' ? ' ' : lines[i]);
      }
      ui.a.set(ra.a === '' ? ' ' : ra.a);
      ui.msg.set(ra.msg);
      p.update();
    }
    function tune() {
      ra.msg = 'tuning...';
      radioShow();
      os.netUp(function (ok) {
        if (ok !== 1) { ra.msg = 'no network - save one in settings';
                        radioShow(); return; }
        // adviceslip is chunked https: exercises the TLS bundle AND the
        // perform-based chunked decode in one demo.
        var rc = http.get('https://api.adviceslip.com/advice').limit(2048)
          .done(function (k, status, len) {
            if (status !== 200 || len <= 0) {
              ra.msg = 'static (http ' + status + ')';
            } else {
              ra.q = JSON.parse(http.body()).slip.advice;
              ra.a = '- the advice wire';
              ra.msg = 'tap for another - hold to save to shelf';
            }
            radioShow();
          }).send();
        if (rc !== 0) { ra.msg = 'radio busy (' + rc + ')';
                        radioShow(); }
      });
    }
    // Poster layout: no header chrome, the words own the page.
    ui.q = [];
    var body = col().pad(48, os.M, 0, os.M).gap(2);
    var qi;
    for (qi = 0; qi < RA_LINES; qi++) {
      ui.q.push(text(' ').size('xl'));
      body.add(ui.q[qi]);
    }
    ui.a = text(' ').size('sm').gray(96);
    ui.msg = text(' ').size('xs').gray(128);
    body.add(spacer(0, 1), ui.a, ui.msg);
    body.flex(1);
    p.content(body)
      .footer(os.hintFooter('radio - tap = tune - hold = save'));
    p.on(G.TAP, function () { tune(); });
    p.on(G.LONG, function () {
      if (ra.q === '') { return; }
      files.append('/books/radio.txt', ra.q + ' ' + ra.a + '\n',
        function (k, wrote, err) {
          ra.msg = err === 0 ? 'saved to the shelf' : 'no card?';
          if (err === 0) { libraryRescan(); }
          radioShow();
        });
    });
    os.announce('radio');
    p.show(true);
    radioShow();
  }
})
