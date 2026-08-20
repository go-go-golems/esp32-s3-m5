// Network: wifi status/saved networks + an https fetch with the whole
// builder chain (header, limit, done) on screen.
({
  id: 'd-net',
  title: 'Network',
  subtitle: 'wifi + http.get',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var st = { msg: 'tap fetch', body: ' ' };
    var p = page('d-net');
    var wifiT = text(function () {
      var w = wifi.status();
      var names = ['off', 'idle', 'scanning', 'joining', 'up'];
      return 'wifi ' + names[w] +
             (w === 4 ? ' ' + wifi.ssidCurrent() + ' ' + wifi.ip() +
                        ' ' + wifi.rssiCurrent() + 'dBm'
                      : '') +
             '   saved ' + wifi.savedCount();
    }).size('sm');
    var msgT = text(function () { return st.msg; }).size('sm').gray(64);
    var bodyT = text(function () { return st.body; }).size('sm');
    function fetch() {
      st.msg = 'net up...';
      p.update();
      os.netUp(function (ok) {
        if (ok !== 1) { st.msg = 'no network (save one in Settings)';
                        p.update(); return; }
        st.msg = 'GET adviceslip.com (https, chunked)...';
        p.update();
        var rc = http.get('https://api.adviceslip.com/advice')
          .header('Accept', 'application/json')
          .limit(2048)
          .done(function (k, status, len) {
            st.msg = 'http ' + status + ', ' + len + ' bytes, ' +
                     http.bodyLineCount() + ' line(s)';
            if (status === 200) {
              st.body = '"' + JSON.parse(http.body()).slip.advice + '"';
            }
            print('demo: http status=' + status + ' len=' + len);
            p.update();
          }).send();
        if (rc !== 0) { st.msg = 'busy (' + rc + ')'; p.update(); }
      });
    }
    var body = os.body(10).gap(10).add(
      os.label('station status'),
      wifiT,
      divider(1, 0),
      os.label('fetch'),
      msgT, bodyT);
    var btns = os.buttonRow();
    btns.add(os.button('fetch', function () { fetch(); },
                       { w: 130, primary: true }));
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 130 }));
    body.add(btns);
    p.header(os.chrome('NETWORK')).content(body)
      .footer(os.hintFooter('TLS bundle + chunked decode, 2 KiB cap'));
    p.every(2000);
    os.announce('d-net');
    p.show(true);
  }
})
