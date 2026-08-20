// --------------------------------------------------------------- radio --

var RA = { q: '', a: '', msg: 'tap to tune in' };
var RA_LINES = 7;

// Poster typography: the quote fills the page in the 84px bold face,
// black on plain white, word-wrapped onto up to 7 short lines (the
// display is a poster, the shelf copy is the full text).
function radioShow(p, ui) {
  var words = (RA.q === '' ? 'RADIO' : RA.q).split(' ');
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
  ui.a.set(RA.a === '' ? ' ' : RA.a);
  ui.msg.set(RA.msg);
  p.update();
}

function radio() {
  enter('radio');
  var ui = {};
  var p = page('radio');
  function tune() {
    RA.msg = 'tuning...';
    radioShow(p, ui);
    netUp(function (ok) {
      if (ok !== 1) { RA.msg = 'no network - save one in settings';
                      radioShow(p, ui); return; }
      // adviceslip is chunked https: exercises the TLS bundle AND the
      // perform-based chunked decode in one demo.
      var rc = http.get('https://api.adviceslip.com/advice').limit(2048)
        .done(function (k, status, len) {
          if (status !== 200 || len <= 0) {
            RA.msg = 'static (http ' + status + ')';
          } else {
            RA.q = JSON.parse(http.body()).slip.advice;
            RA.a = '- the advice wire';
            RA.msg = 'tap for another - hold to save to shelf';
          }
          radioShow(p, ui);
        }).send();
      if (rc !== 0) { RA.msg = 'radio busy (' + rc + ')';
                      radioShow(p, ui); }
    });
  }
  // Poster layout: no header chrome, the words own the page.
  ui.q = [];
  var body = col().pad(48, M, 0, M).gap(2);
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
    .footer(hintFooter('radio - tap = tune - hold = save'));
  p.on(G.TAP, function () { tune(); });
  p.on(G.LONG, function () {
    if (RA.q === '') { return; }
    files.append('/books/radio.txt', RA.q + ' ' + RA.a + '\n',
      function (k, wrote, err) {
        RA.msg = err === 0 ? 'saved to the shelf' : 'no card?';
        if (err === 0) { libraryRescan(); }
        radioShow(p, ui);
      });
  });
  announce('radio');
  p.show(true);
  radioShow(p, ui);
}

