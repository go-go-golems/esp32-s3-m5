// Sound: buzzer tone/beep/stop/melody.
({
  id: 'd-sound',
  title: 'Sound',
  subtitle: 'tone beep melody',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var st = { last: 'silent' };
    var lastT = text(function () { return st.last; }).size('lg');
    var p = page('d-sound');
    function play(what, fn) { st.last = what; fn(); p.update(); }
    var NOTES = [262, 330, 392, 523, 660, 784, 1047];
    var scale = os.buttonRow();
    var i;
    for (i = 0; i < NOTES.length; i++) {
      (function (f, n) {
        scale.add(os.key('' + (n + 1), function () {
          play('tone ' + f + ' Hz', function () { buzzer.tone(f, 120); });
        }, 56));
      })(NOTES[i], i);
    }
    var body = os.body(12).gap(12).add(
      os.label('last played'),
      lastT,
      divider(1, 0),
      text('a scale (buzzer.tone)').size('xs').gray(96),
      scale);
    var btns = os.buttonRow();
    btns.add(os.button('beep', function () {
      play('beep()', function () { buzzer.beep(); });
    }, { w: 110 }));
    btns.add(os.button('melody', function () {
      play('melody 5 notes', function () {
        buzzer.melody('880:120,988:120,1109:120,1319:200,880:200');
      });
    }, { w: 130 }));
    btns.add(os.button('stop', function () {
      play('stop()', function () { buzzer.stop(); });
    }, { w: 110 }));
    body.add(btns);
    var btns2 = os.buttonRow();
    btns2.add(os.button('demos', function () { os.launch('demos'); },
                        { w: 130 }));
    body.add(btns2);
    p.header(os.chrome('SOUND')).content(body)
      .footer(os.hintFooter('GPIO21 LEDC chimes - swipe down = home'));
    os.announce('d-sound');
    p.show(true);
  }
})
