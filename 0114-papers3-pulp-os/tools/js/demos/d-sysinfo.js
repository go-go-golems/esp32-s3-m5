// System: clocks, ABI, gc, the ROM asset registry and the merged catalog.
({
  id: 'd-sysinfo',
  title: 'System',
  subtitle: 'abi, clocks, catalog',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var p = page('d-sysinfo');
    var up = text(function () {
      return 'millis ' + millis() + '   Date.now ' + Date.now();
    }).size('sm');
    var c = catalog();
    var rom = 0;
    var sd = 0;
    var hid = 0;
    var i;
    for (i = 0; i < c.length; i++) {
      if (c[i].source === 'rom') { rom = rom + 1; } else { sd = sd + 1; }
      if (c[i].hidden) { hid = hid + 1; }
    }
    var body = os.body(10).gap(8).add(
      os.label('engine'),
      text('abiVersion ' + abiVersion() + '   paper.version ' +
           paper.version()).size('sm'),
      up,
      divider(1, 0),
      os.label('rom assets'),
      text('apps.count() = ' + apps.count() + ' embedded modules')
        .size('sm'),
      text(function () {
        var names = [];
        var j;
        for (j = 0; j < apps.count() && j < 6; j++) {
          names.push(apps.name(j));
        }
        return names.join(' ') + ' ...';
      }).size('xs').gray(96),
      divider(1, 0),
      os.label('catalog'),
      text(c.length + ' entries: ' + rom + ' rom-sourced, ' + sd +
           ' from the card, ' + hid + ' hidden').size('sm'),
      text('gc() runs on demand; tap collect to force one').size('xs')
        .gray(96));
    var btns = os.buttonRow();
    btns.add(os.button('collect', function () { gc(); print('demo: gc'); },
                       { w: 130 }));
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 130 }));
    body.add(btns);
    p.header(os.chrome('SYSTEM')).content(body)
      .footer(os.hintFooter('the machine, introspected from JS'));
    p.every(1000);
    os.announce('d-sysinfo');
    p.show(true);
  }
})
