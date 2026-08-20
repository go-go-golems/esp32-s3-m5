// Files & store: the whole files chain with an on-screen log, a JSON
// round trip, and a storeGet/storeSet counter that survives reboots.
({
  id: 'd-storage',
  title: 'Files & Store',
  subtitle: 'write read append list remove',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var runs = storeGet('demoruns', 0) + 1;
    storeSet('demoruns', runs);
    var LOG = [];
    var logT = [];
    var p = page('d-storage');
    function log(line) {
      print('demo: ' + line);
      LOG.push(line);
      if (LOG.length > 10) { LOG.shift(); }
      var i;
      for (i = 0; i < logT.length; i++) {
        logT[i].set(i < LOG.length ? LOG[i] : ' ');
      }
      p.update();
    }
    function chain() {
      log('write /demo/log.txt ...');
      files.write('/demo/log.txt', 'hello from run ' + runs,
        function (k, wrote, err) {
          log('  wrote ' + wrote + 'B err=' + err);
          files.append('/demo/log.txt', ' + appended line',
            function (k2, w2, e2) {
              log('  appended ' + w2 + 'B');
              files.read('/demo/log.txt', function (k3, lines, e3) {
                log('  read ' + lines + ' line(s): "' +
                    files.line(0).slice(0, 24) + '..."');
                files.list('/demo', function (k4, count, e4) {
                  log('  /demo has ' + count + ' entries');
                  files.remove('/demo/log.txt', function (k5, okv, e5) {
                    log('  removed, err=' + e5);
                    var obj = JSON.parse('{"x":' + runs + ',"y":[1,2]}');
                    log('JSON round trip x=' + obj.x + ' y1=' + obj.y[1]);
                    log('chain done.');
                  });
                });
              });
            });
        });
    }
    var body = os.body(8).gap(6);
    body.add(os.label('run ' + runs + ' (storeGet survives reboots)'));
    body.add(divider(1, 0));
    var i;
    for (i = 0; i < 10; i++) {
      var t = text(' ').size('sm');
      logT.push(t);
      body.add(t);
    }
    body.add(divider(1, 0));
    var btns = os.buttonRow();
    btns.add(os.button('run chain', function () { chain(); },
                       { w: 150, primary: true }));
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 130 }));
    body.add(btns);
    p.header(os.chrome('FILES & STORE')).content(body)
      .footer(os.hintFooter('one op in flight - completions chain'));
    os.announce('d-storage');
    p.show(true);
    chain();
  }
})
