// ------------------------------------------------------------ postcard --
({
  id: 'postcard',
  title: 'Postcard',
  subtitle: 'one line a day',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var PC_ROWS = ['qwertyuio', 'asdfghjkl', 'pzxcvbnm'];
    var pc = os.state('postcard', function () {
      return { draft: '', msg: '' };
    });
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
    os.keyboard(body, PC_ROWS, put, {
      del: function () { pc.draft = pc.draft.slice(0, -1); refresh(); }
    });
    var last = row().gap(2).mainAlign(1);
    last.add(text(',').size('lg').center().width(48).height(56)
      .onTap(function () { put(','); }));
    last.add(os.button('space', function () { put(' '); },
                       { w: 200, size: 'sm' }));
    last.add(os.button('SEAL', function () {
        if (pc.draft === '') { pc.msg = 'nothing to seal'; }
        else if (appendPostcard(pc.draft) === 0) {
          pc.msg = 'sealed.'; pc.draft = '';
          buzzer.melody('1319:40,880:80');
        } else { pc.msg = 'no card?'; }
        refresh();
      }, { w: 100, primary: true, size: 'sm' }));
    body.add(last);
    p.header(os.chrome('POSTCARD')).content(body)
      .footer(os.hintFooter('seal = save to postcard.txt - no edits'));
    os.announce('postcard');
    p.show(true);
    refresh();
  }
})
