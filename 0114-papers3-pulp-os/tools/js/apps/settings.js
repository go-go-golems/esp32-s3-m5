// ------------------------------------------------------------ settings --
// Multi-screen app: main / scan / pass. Screen changes cross the
// app-switch boundary via os.launch('settings', {screen, ssid}) so the
// loader owns every resetTree (enter is not part of the app contract).
// Transient status text lives in os.state so it survives the relaunches.
({
  id: 'settings',
  title: 'Settings',
  subtitle: 'wifi - serve - margins',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var WIFI_STATES = ['off', 'idle', 'scanning', 'joining', 'up'];
    var KB_ROWS = ['1234567890', 'qwertyuiop', 'asdfghjkl', 'zxcvbnm.-_'];
    var st = os.state('settings', function () { return { msg: '' }; });

    function setRow(menu, label, sub, fn) {
      var line = row().pad(6, 0, 4, 0).gap(10).crossAlign(3)
        .add(text(label).size('lg'), spacer(0, 1),
             text(sub).size('xs').gray(112));
      var entry = col().pad(0, os.M, 0, os.M).add(line, divider(2, 0));
      if (fn) { entry.onTap(fn); }
      menu.add(entry);
    }

    function mainScreen() {
      var w = wifi.status();
      var wifiSub = WIFI_STATES[w] +
        (w === 4 ? ' - ' + wifi.ssidCurrent() + ' ' + wifi.ip() : '');
      var menu = list().pad(4, 0, 0, 0);
      setRow(menu, 'Wifi', wifiSub, function () {
        os.launch('settings', { screen: 'scan' });
      });
      var i;
      var n = wifi.savedCount();
      for (i = 0; i < n; i++) {
        (function (ssid) {
          setRow(menu, '  ' + ssid, 'saved - tap to forget', function () {
            wifi.forget(ssid);
            st.msg = 'forgot ' + ssid;
            os.launch('settings');
          });
        })(wifi.savedSsid(i));
      }
      var url = serve.url();
      setRow(menu, 'Serve', serve.url() === '' ? 'off - tap to start' : url,
        function () {
          if (serve.url() !== '') {
            serve.stop();
            st.msg = 'server stopped';
            os.launch('settings');
            return;
          }
          // No rebuild until the callback lands: a relaunch would
          // resetTree and cancel the pending joinSaved completion.
          os.netUp(function (ok) {
            if (ok !== 1) { st.msg = 'no network';
                            os.launch('settings'); return; }
            serve.files('/', '/sdcard/www');
            serve.start(80);
            st.msg = 'serving';
            os.launch('settings');
          });
        });
      setRow(menu, 'Margins', M === 40 ? 'on (40px) - tap to remove' :
        'off - tap to restore', function () {
          M = M === 40 ? 0 : 40;
          storeSet('margin', M);
          os.launch('settings');
        });
      setRow(menu, 'Apps', 'installed apps - rescan - state', function () {
        os.launch('settings', { screen: 'apps' });
      });
      setRow(menu, 'Radio off', 'save power', function () {
        serve.stop();
        wifi.off();
        st.msg = 'radio down';
        os.launch('settings');
      });
      var p = page('settings').header(os.chrome('SETTINGS')).content(menu)
        .footer(os.hintFooter(st.msg === '' ? 'swipe down = home' : st.msg));
      st.msg = '';
      os.announce('settings');
      p.show(true);
    }

    function scanScreen() {
      var body = col().pad(20, os.M, 0, os.M).gap(12)
        .add(text('scanning...').size('sm').gray(96));
      var p = page('settings').header(os.chrome('NETWORKS')).content(body)
        .footer(os.hintFooter('swipe down = home'));
      os.announce('settings-scan');
      p.show(true);
      var rc = wifi.scan(function (k, n, err) {
        var menu = list().pad(4, 0, 0, 0);
        var i;
        for (i = 0; i < n; i++) {
          (function (ssid, rssi, sec) {
            setRow(menu, ssid, rssi + ' dBm' + (sec === 1 ? ' *' : ''),
              function () {
                os.launch('settings', { screen: 'pass', ssid: ssid });
              });
          })(wifi.ssid(i), wifi.rssi(i), wifi.secure(i));
        }
        if (n === 0) {
          menu.add(col().pad(20, os.M, 0, os.M)
            .add(text('nothing in the air').size('sm').gray(96)));
        }
        var p2 = page('settings').header(os.chrome('NETWORKS')).content(menu)
          .footer(os.hintFooter('tap to join - swipe down = home'));
        p2.show(true);
      });
      if (rc !== 0) {
        st.msg = 'scan failed (' + rc + ')';
        os.launch('settings');
      }
    }

    function passScreen(ssid) {
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
              st.msg = 'joined ' + ssid;
              os.launch('settings');
            } else {
              msgT.set('failed (reason ' + err + ') - try again');
              p.update();
            }
          });
          if (rc !== 0) { msgT.set('busy (' + rc + ')'); p.update(); }
        }));
      body.add(last);
      p.header(os.chrome('PASSWORD')).content(body)
        .footer(os.hintFooter('lowercase + digits - swipe down = home'));
      os.announce('settings-pass');
      p.show(true);
      refresh();
    }

    function appsScreen() {
      var menu = list().pad(4, 0, 0, 0);
      var c = catalog();
      var i;
      for (i = 0; i < c.length; i++) {
        (function (e) {
          if (e.hidden) { return; }
          var sub = e.broken ? ('! ' + e.broken) : e.source;
          var fn = null;
          if (e.source !== 'rom') {
            sub = sub + ' - tap to remove';
            fn = function () {
              files.remove('/apps/' + e.id + '.js', function () {
                files.remove('/apps/' + e.id + '.json', function () {
                  scanApps(function () {
                    st.msg = 'removed ' + e.id;
                    os.launch('settings', { screen: 'apps' });
                  });
                });
              });
            };
          }
          setRow(menu, e.title, sub, fn);
        })(c[i]);
      }
      setRow(menu, 'Rescan', 'reload /apps from the card', function () {
        scanApps(function () {
          st.msg = 'rescanned';
          os.launch('settings', { screen: 'apps' });
        });
      });
      setRow(menu, 'Clear app state', 'forget in-memory app state',
        function () {
          os.clearAllState();
          st.msg = 'state cleared';
          os.launch('settings', { screen: 'apps' });
        });
      var p = page('settings').header(os.chrome('APPS')).content(menu)
        .footer(os.hintFooter(st.msg === '' ? 'swipe down = home'
                                            : st.msg));
      st.msg = '';
      os.announce('settings-apps');
      p.show(true);
    }

    var a = arg || {};
    if (a.screen === 'scan') { scanScreen(); }
    else if (a.screen === 'pass') { passScreen(a.ssid); }
    else if (a.screen === 'apps') { appsScreen(); }
    else { mainScreen(); }
  }
})
