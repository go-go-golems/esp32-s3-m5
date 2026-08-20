// ------------------------------------------------------------ postcard --

var PC_ROWS = ['qwertyuio', 'asdfghjkl', 'pzxcvbnm'];
var PC = { draft: '', msg: '' };

function postcard() {
  enter('postcard');
  var pc = PC;
  var draftT = null;
  var countT = null;
  var p = page('postcard');
  function refresh() {
    draftT.set(pc.draft === '' ? '(empty)' : pc.draft);
    countT.set(pc.draft.length + '/63' +
               (pc.msg === '' ? '' : '   ' + pc.msg));
    p.update();
  }
  function put(ch) {
    if (pc.draft.length < 63) { pc.draft += ch; pc.msg = ''; refresh(); }
  }
  var body = col().pad(10, 24, 0, 24).gap(8);
  body.add(text('today, one line:').size('xs').gray(96));
  draftT = text(' ').size('xs');
  body.add(draftT);
  countT = text(' ').size('xs').gray(128);
  body.add(countT);
  body.add(divider(1, 0));
  var r, i;
  for (r = 0; r < 3; r++) {
    var line = row().gap(2).mainAlign(1);
    for (i = 0; i < PC_ROWS[r].length; i++) {
      (function (ch) {
        line.add(text(ch).size('lg').center().width(52).height(56)
          .onTap(function () { put(ch); }));
      })(PC_ROWS[r].charAt(i));
    }
    if (r === 2) {
      line.add(text('<del>').size('xs').center().width(70).height(52)
        .onTap(function () {
          pc.draft = pc.draft.slice(0, -1); refresh(); }));
    }
    body.add(line);
    body.add(divider(1, 200));
  }
  var last = row().gap(2).mainAlign(1);
  last.add(text(',').size('lg').center().width(52).height(56)
    .onTap(function () { put(','); }));
  last.add(text('space').size('xs').center().width(200).height(56)
    .onTap(function () { put(' '); }));
  last.add(text(' SEAL ').size('xs').invert().center().width(90).height(56)
    .onTap(function () {
      if (pc.draft === '') { pc.msg = 'nothing to seal'; }
      else if (appendPostcard(pc.draft) === 0) {
        pc.msg = 'sealed.'; pc.draft = '';
        buzzer.melody('1319:40,880:80');
      } else { pc.msg = 'no card?'; }
      refresh();
    }));
  body.add(last);
  p.header(chrome('POSTCARD')).content(body)
    .footer(hintFooter('seal = save to postcard.txt - no edits'));
  announce('postcard');
  p.show(true);
  refresh();
}

