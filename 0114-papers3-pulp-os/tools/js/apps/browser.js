// ------------------------------------------------------------- browser --
// The PULP browser (ESP-55 P10): fetches pages — JS descriptor scripts
// ({title, main(ui, nav)}) — over http(s) and runs them in a sandboxed
// page context (UI stdlib: drawing + nav only). This app stays resident
// in the OS context while a page owns the panel; nav requests arrive
// through browser.watch and are re-armed on every delivery.
({
  id: 'browser',
  title: 'Browser',
  subtitle: 'pages from the ether',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var st = os.state('browser', function () {
      return { url: '', hist: [] };
    });

    function resolve(u) {
      if (u.indexOf('http://') === 0 || u.indexOf('https://') === 0) {
        return u;
      }
      var base = st.url;
      if (base === '') { return u; }
      var scheme = base.indexOf('//') + 2;
      if (u.charAt(0) === '/') {
        var host = base.indexOf('/', scheme);
        return host < 0 ? base + u : base.slice(0, host) + u;
      }
      var dir = base.lastIndexOf('/');
      return dir < scheme ? base + '/' + u : base.slice(0, dir + 1) + u;
    }

    function watch() {
      browser.watch(function (k, kind, err) {
        watch();  // the slot clears before delivery; re-arm first
        if (kind === 1) { go(resolve(browser.navUrl())); }
        else if (kind === 2) { back(); }
        else if (kind === 3 && st.url !== '') { fetchPage(st.url); }
      });
    }

    function fail(why) {
      browser.close();
      var p = page('bfail').header(os.chrome('BROWSER'))
        .content(col().pad(20, os.M, 0, os.M).gap(10).add(
          text(why).size('md'),
          text(st.url === '' ? ' ' : st.url).size('xs').gray(96),
          row().gap(16).mainAlign(1).add(
            os.button('back', function () { back(); }, { w: 120 }),
            os.button('url', function () { urlScreen(); }, { w: 120 }))))
        .footer(os.hintFooter('swipe down = home'));
      print('pulp screen: browser/error ' + why);
      p.show(true);
    }

    function fetchPage(url) {
      os.netUp(function (ok) {
        if (ok !== 1) { fail('no network'); return; }
        var rc = http.get(url).limit(32768).done(function (k, status, n) {
          if (status !== 200 || n <= 0) { fail('http ' + status); return; }
          var w = apps.writeText('/web/page.js', http.body());
          if (w !== 0) { fail('cache write (' + w + ')'); return; }
          var rc2 = browser.run('/web/page.js', url);
          if (rc2 !== 0) { fail('bad page (' + rc2 + ')'); return; }
          print('pulp screen: browser/' + url);
        }).send();
        if (rc !== 0) { fail('http busy (' + rc + ')'); }
      });
    }

    function go(url) {
      if (st.url !== '' && st.url !== url) { st.hist.push(st.url); }
      if (st.hist.length > 8) { st.hist.shift(); }
      st.url = url;
      fetchPage(url);
    }

    function back() {
      if (st.hist.length === 0) { urlScreen(); return; }
      st.url = st.hist.pop();
      fetchPage(st.url);
    }

    function urlScreen() {
      browser.close();
      var URL_ROWS = ['1234567890', 'qwertyuiop', 'asdfghjkl', 'zxcvbnm'];
      var draft = st.url === '' ? 'http://' : st.url;
      var draftT = null;
      var p = page('burl');
      function refresh() { draftT.set(draft); p.update(); }
      function put(ch) { if (draft.length < 120) { draft += ch; refresh(); } }
      var body = os.body(10).gap(8);
      body.add(os.label('open page url'));
      draftT = text(' ').size('sm');
      body.add(draftT);
      body.add(divider(1, 0));
      os.keyboard(body, URL_ROWS, put);
      var extra = row().pad(6, 0, 0, 0).gap(10).mainAlign(1);
      var EXTRA = [':', '/', '.', '-', '_'];
      var i;
      for (i = 0; i < EXTRA.length; i++) {
        (function (ch) {
          extra.add(os.key(ch, function () { put(ch); }, 42));
        })(EXTRA[i]);
      }
      extra.add(os.button('del', function () {
        draft = draft.slice(0, -1); refresh(); }, { w: 80, size: 'sm' }));
      extra.add(os.button('GO', function () { go(draft); },
                          { w: 90, primary: true, size: 'sm' }));
      body.add(extra);
      var hist = col().pad(8, 0, 0, 0).gap(4);
      for (i = st.hist.length - 1; i >= 0; i--) {
        (function (u) {
          hist.add(text(u).size('xs').gray(96).height(40)
            .onTap(function () { go(u); }));
        })(st.hist[i]);
      }
      body.add(hist);
      p.header(os.chrome('BROWSER')).content(body)
        .footer(os.hintFooter('a page is a script - GO = fetch'));
      os.announce('browser');
      p.show(true);
      refresh();
    }

    watch();
    var a = arg || {};
    if (a.url) { st.url = ''; go(a.url); }
    else { urlScreen(); }
  }
})
