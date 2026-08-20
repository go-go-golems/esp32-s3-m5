// Type & widget specimen: every text size/face, gray steps, invert,
// progress bars, and the layout primitives — the design system as a
// screen.
({
  id: 'd-widgets',
  title: 'Type & Widgets',
  subtitle: 'the builder specimen',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var body = os.body(8).gap(6);
    body.add(text('xl grotesque 84').size('xl'));
    body.add(text('lg grotesque display').size('lg'));
    body.add(text('title serif display').size('title'));
    body.add(text('md serif body — reading text').size('md'));
    body.add(text('sm serif ui, gray 0/64/96').size('sm'));
    body.add(row().gap(14).add(
      text('gray 64').size('sm').gray(64),
      text('gray 96').size('sm').gray(96),
      text('gray 128').size('sm').gray(128),
      text('gray 176').size('sm').gray(176)));
    body.add(text(' INVERT CHIP ').size('sm').invert());
    body.add(divider(1, 0));
    body.add(text('progressBar 250 / 500 / 750').size('xs').gray(96));
    body.add(progressBar(250, 16).height(16));
    body.add(progressBar(500, 16).height(16));
    body.add(progressBar(750, 16).height(16));
    body.add(divider(1, 0));
    body.add(text('row mainAlign: start center end').size('xs').gray(96));
    body.add(row().gap(8).mainAlign(0).add(
      text('A').size('lg'), text('start').size('xs').gray(112)));
    body.add(row().gap(8).mainAlign(1).add(
      text('B').size('lg'), text('center').size('xs').gray(112)));
    body.add(row().gap(8).mainAlign(2).add(
      text('C').size('lg'), text('end').size('xs').gray(112)));
    body.add(divider(2, 0));
    var btns = os.buttonRow();
    btns.add(os.button('primary', function () {}, { w: 140, primary: true }));
    btns.add(os.button('demos', function () { os.launch('demos'); },
                       { w: 140 }));
    body.add(btns);
    var p = page('d-widgets').header(os.chrome('TYPE & WIDGETS'))
      .content(body)
      .footer(os.hintFooter('every face, gray and layout primitive'));
    os.announce('d-widgets');
    p.show(true);
  }
})
