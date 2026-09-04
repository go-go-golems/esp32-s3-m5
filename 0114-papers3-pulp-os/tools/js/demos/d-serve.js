// Web server: JS routes registered by an app, query parsing, a live hit
// counter — and the resetTree lifecycle stated on screen.
({
  id: 'd-serve',
  title: 'Web Server',
  subtitle: 'JS routes from an app',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var st = { hits: 0, echo: '(none yet)', url: serve.url() };
    var p = page('d-serve');
    function routes() {
      serve.get('/demo').handle(function (req) {
        st.hits = st.hits + 1;
        return serve.json('{"hits":' + st.hits + ',"app":"d-serve"}');
      });
      serve.get('/demo/echo').handle(function (req) {
        st.hits = st.hits + 1;
        var q = serve.query(req);
        st.echo = q === '' ? '(empty)' : q;
        return serve.text('echo: ' + q);
      });
    }
    var urlT = text(function () {
      return st.url === '' ? 'server is OFF - start it below'
                           : st.url + '/demo';
    }).size('sm');
    var hitsT = text(function () { return 'route hits: ' + st.hits; })
      .size('lg');
    var echoT = text(function () { return 'last echo query: ' + st.echo; })
      .size('sm').gray(64);
    var body = os.body(10).gap(10).add(
      os.label('routes this app registered'),
      text('GET /demo -> json hit counter').size('sm').gray(64),
      text('GET /demo/echo?msg=hi -> echoes the query').size('sm')
        .gray(64),
      divider(1, 0),
      urlT, hitsT, echoT,
      divider(1, 0),
      text('routes die at every app switch (resetTree); this app')
        .size('xs').gray(96),
      text('re-registers its own on entry - the OS does the same.')
        .size('xs').gray(96));
    var btns = os.buttonRow();
    btns.add(os.button('start server', function () {
      os.netUp(function (ok) {
        if (ok !== 1) { return; }
        serve.start(80);
        st.url = serve.url();
        routes();
        p.update();
      });
    }, { w: 180 }));
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 130 }));
    body.add(btns);
    p.header(os.chrome('WEB SERVER')).content(body)
      .footer(os.hintFooter('curl the urls above and watch the counter'));
    p.every(1000);
    if (st.url !== '') { routes(); }
    os.announce('d-serve');
    p.show(true);
  }
})
