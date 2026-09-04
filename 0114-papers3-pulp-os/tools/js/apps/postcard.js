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
    var body = os.body(10).gap(8);
    body.add(os.label('today, one line'));
    draftT = text(' ').size('sm');
    body.add(draftT);
    countT = text(' ').size('xs').gray(128);
    body.add(countT);
    body.add(divider(1, 0));
    os.keyboard(body, PC_ROWS, put);
    var last = row().pad(6, 0, 0, 0).gap(14).mainAlign(1);
    last.add(os.key(',', function () { put(','); }, 42));
    last.add(os.button('delete', function () {
      pc.draft = pc.draft.slice(0, -1); refresh(); },
      { w: 110, size: 'sm' }));
    last.add(os.button('space', function () { put(' '); },
                       { w: 140, size: 'sm' }));
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
