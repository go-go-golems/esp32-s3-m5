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
            text('[ back ]').size('xs').center().width(120).height(56)
              .onTap(function () { back(); }),
            text('[ url ]').size('xs').center().width(120).height(56)
              .onTap(function () { urlScreen(); }))))
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
      var body = col().pad(10, 24, 0, 24).gap(8);
      body.add(text('open page url:').size('sm'));
      draftT = text(' ').size('xs');
      body.add(draftT);
      body.add(divider(1, 0));
      var r, i;
      for (r = 0; r < 4; r++) {
        var line = row().gap(2).mainAlign(1);
        for (i = 0; i < URL_ROWS[r].length; i++) {
          (function (ch) {
            line.add(text(ch).size('lg').center().width(48).height(56)
              .onTap(function () { put(ch); }));
          })(URL_ROWS[r].charAt(i));
        }
        if (r === 3) {
          line.add(text('<del>').size('xs').center().width(70).height(52)
            .onTap(function () { draft = draft.slice(0, -1); refresh(); }));
        }
        body.add(line);
        body.add(divider(1, 200));
      }
      var extra = row().gap(2).mainAlign(1);
      var EXTRA = [':', '/', '.', '-', '_'];
      for (i = 0; i < EXTRA.length; i++) {
        (function (ch) {
          extra.add(text(ch).size('lg').center().width(64).height(56)
            .onTap(function () { put(ch); }));
        })(EXTRA[i]);
      }
      extra.add(text(' GO ').size('xs').invert().center().width(90)
        .height(56).onTap(function () { go(draft); }));
      body.add(extra);
      var hist = col().pad(8, 24, 0, 24).gap(4);
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
