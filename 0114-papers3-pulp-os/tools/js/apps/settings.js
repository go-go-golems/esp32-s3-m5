// ------------------------------------------------------------ settings --

var WIFI_STATES = ['off', 'idle', 'scanning', 'joining', 'up'];

// Brings the network up if needed, then fn(ok). Uses stored credentials.
function netUp(fn) {
  if (wifi.status() === 4) { fn(1); return; }
  if (wifi.savedCount() === 0) { fn(0); return; }
  wifi.joinSaved(function (k, ok, err) { fn(ok); });
}

var SET = { msg: '' };

function setRow(menu, label, sub, fn) {
  var line = row().pad(6, 0, 4, 0).gap(10).crossAlign(3)
    .add(text(label).size('lg'), spacer(0, 1),
         text(sub).size('xs').gray(112));
  var entry = col().pad(0, M, 0, M).add(line, divider(2, 0));
  if (fn) { entry.onTap(fn); }
  menu.add(entry);
}

function settings() {
  enter('settings');
  var st = wifi.status();
  var wifiSub = WIFI_STATES[st] +
    (st === 4 ? ' - ' + wifi.ssidCurrent() + ' ' + wifi.ip() : '');
  var menu = list().pad(4, 0, 0, 0);
  setRow(menu, 'Wifi', wifiSub, settingsScan);
  var i;
  var n = wifi.savedCount();
  for (i = 0; i < n; i++) {
    (function (ssid) {
      setRow(menu, '  ' + ssid, 'saved - tap to forget', function () {
        wifi.forget(ssid);
        SET.msg = 'forgot ' + ssid;
        settings();
      });
    })(wifi.savedSsid(i));
  }
  var url = serve.url();
  setRow(menu, 'Serve', serve.url() === '' ? 'off - tap to start' : url,
    function () {
      if (serve.url() !== '') {
        serve.stop();
        SET.msg = 'server stopped';
        settings();
        return;
      }
      // No rebuild until the callback lands: settings() would resetTree
      // and cancel the pending joinSaved completion.
      netUp(function (ok) {
        if (ok !== 1) { SET.msg = 'no network'; settings(); return; }
        serve.files('/', '/sdcard/www');
        serve.start(80);
        SET.msg = 'serving';
        settings();
      });
    });
  setRow(menu, 'Margins', M === 40 ? 'on (40px) - tap to remove' :
    'off - tap to restore', function () {
      M = M === 40 ? 0 : 40;
      storeSet('margin', M);
      settings();
    });
  setRow(menu, 'Radio off', 'save power', function () {
    serve.stop();
    wifi.off();
    SET.msg = 'radio down';
    settings();
  });
  var p = page('settings').header(chrome('SETTINGS')).content(menu)
    .footer(hintFooter(SET.msg === '' ? 'swipe down = home' : SET.msg));
  SET.msg = '';
  announce('settings');
  p.show(true);
}

function settingsScan() {
  enter('settings');
  var body = col().pad(20, M, 0, M).gap(12)
    .add(text('scanning...').size('sm').gray(96));
  var p = page('settings').header(chrome('NETWORKS')).content(body)
    .footer(hintFooter('swipe down = home'));
  announce('settings-scan');
  p.show(true);
  var rc = wifi.scan(function (k, n, err) {
    var menu = list().pad(4, 0, 0, 0);
    var i;
    for (i = 0; i < n; i++) {
      (function (ssid, rssi, sec) {
        setRow(menu, ssid, rssi + ' dBm' + (sec === 1 ? ' *' : ''),
          function () { settingsPass(ssid); });
      })(wifi.ssid(i), wifi.rssi(i), wifi.secure(i));
    }
    if (n === 0) {
      menu.add(col().pad(20, M, 0, M)
        .add(text('nothing in the air').size('sm').gray(96)));
    }
    var p2 = page('settings').header(chrome('NETWORKS')).content(menu)
      .footer(hintFooter('tap to join - swipe down = home'));
    p2.show(true);
  });
  if (rc !== 0) {
    SET.msg = 'scan failed (' + rc + ')';
    settings();
  }
}

var KB_ROWS = ['1234567890', 'qwertyuiop', 'asdfghjkl', 'zxcvbnm.-_'];

function settingsPass(ssid) {
  enter('settings');
  var draft = '';
  var draftT = null;
  var msgT = null;
  var p = page('settings');
  function refresh() {
    draftT.set(draft === '' ? '(password)' : draft);
    p.update();
  }
  function put(ch) {
    if (draft.length < 63) { draft += ch; refresh(); }
  }
  var body = col().pad(10, 24, 0, 24).gap(8);
  body.add(text('join ' + ssid).size('sm'));
  draftT = text(' ').size('xs');
  body.add(draftT);
  msgT = text(' ').size('xs').gray(128);
  body.add(msgT);
  body.add(divider(1, 0));
  var r, i;
  for (r = 0; r < 4; r++) {
    var line = row().gap(2).mainAlign(1);
    for (i = 0; i < KB_ROWS[r].length; i++) {
      (function (ch) {
        line.add(text(ch).size('lg').center().width(48).height(56)
          .onTap(function () { put(ch); }));
      })(KB_ROWS[r].charAt(i));
    }
    if (r === 3) {
      line.add(text('<del>').size('xs').center().width(70).height(52)
        .onTap(function () { draft = draft.slice(0, -1); refresh(); }));
    }
    body.add(line);
    body.add(divider(1, 200));
  }
  var last = row().gap(2).mainAlign(1);
  last.add(text('space').size('xs').center().width(160).height(56)
    .onTap(function () { put(' '); }));
  last.add(text(' JOIN ').size('xs').invert().center().width(110)
    .height(56).onTap(function () {
      msgT.set('joining...');
      p.update();
      var rc = wifi.join(ssid, draft, function (k, ok, err) {
        if (ok === 1) {
          wifi.save(ssid, draft);
          buzzer.melody('880:80,1319:120');
          SET.msg = 'joined ' + ssid;
          settings();
        } else {
          msgT.set('failed (reason ' + err + ') - try again');
          p.update();
        }
      });
      if (rc !== 0) { msgT.set('busy (' + rc + ')'); p.update(); }
    }));
  body.add(last);
  p.header(chrome('PASSWORD')).content(body)
    .footer(hintFooter('lowercase + digits - swipe down = home'));
  announce('settings-pass');
  p.show(true);
  refresh();
}

