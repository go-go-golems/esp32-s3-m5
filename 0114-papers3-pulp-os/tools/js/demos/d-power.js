// Power & mDNS: the battery singleton, the legacy alias, and the mdns
// read-only surface — refreshed live.
({
  id: 'd-power',
  title: 'Power & mDNS',
  subtitle: 'battery + pulp.local',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var p = page('d-power');
    var lvl = text(function () {
      return battery.level() + '%';
    }).size('xl').center();
    var bar = progressBar(0, 20).height(20);
    var detail = text(function () {
      return battery.mv() + ' mV   charging=' + battery.charging() +
             '   statusText "' + battery.statusText() + '"';
    }).size('sm').center();
    var alias = text(function () {
      return 'legacy batteryLevel() = ' + batteryLevel();
    }).size('xs').gray(96).center();
    var mdnsT = text(function () {
      return 'mdns status=' + mdns.status() + ' host=' + mdns.host() +
             ' url=' + (mdns.url() === '' ? '(down)' : mdns.url());
    }).size('sm');
    var body = os.body(20).gap(12).add(
      os.label('battery'),
      lvl, bar, detail, alias,
      divider(1, 0),
      os.label('mdns'),
      mdnsT,
      text('announced by serve.start, withdrawn by stop/wifi.off')
        .size('xs').gray(96));
    var btns = os.buttonRow();
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 130 }));
    body.add(btns);
    p.header(os.chrome('POWER & MDNS')).content(body)
      .footer(os.hintFooter('dyn texts refresh every 2 s'));
    p.on(G.TICK, function () { bar.progress(battery.level() * 10); });
    p.every(2000);
    os.announce('d-power');
    p.show(true);
    bar.progress(battery.level() * 10);
  }
})
