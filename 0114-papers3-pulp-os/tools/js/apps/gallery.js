// ------------------------------------------------------------ gallery --
// ESP-54: display stored .g4 images full-screen, swipe left/right to
// browse, tap to delete. images.display() does a clean-full present of
// the bitmap; the chrome (counter + name + hint) is drawn in the SAME
// frame so there is exactly one present per navigation.
({
  id: 'gallery',
  title: 'Gallery',
  subtitle: 'your pictures',
  version: 1,
  abi: 2,
  main: function (os, arg) {
    var g = os.state('gallery', function () { return { idx: 0 }; });
    function galleryRefresh() {
      var n = images.count();
      if (n === 0) { os.launch('gallery'); return; }
      if (g.idx >= n) { g.idx = n - 1; }
      if (g.idx < 0) { g.idx = 0; }
      // The image present (clean full) is the primary; the hint overlays.
      var ok = images.display(images.name(g.idx));
      print('gallery: ' + (g.idx + 1) + '/' + n + ' '
            + images.name(g.idx) + ' -> ' + ok);
    }
    var n = images.count();
    if (n === 0) {
      var url = mdns.url() !== '' ? mdns.url()
               : (serve.url() !== '' ? serve.url() : 'pulp.local');
      var body0 = col().pad(20, os.M, 0, os.M).gap(8)
        .add(text('no images yet').size('md').gray(96),
             text('upload at ' + url).size('xs').gray(128));
      var p0 = page('gallery').header(os.chrome('GALLERY')).content(body0)
        .footer(os.hintFooter('upload a picture from your browser'));
      os.announce('gallery');
      p0.show(true);
      return;
    }
    paper.refreshTurns(1);
    // A page with gesture handlers but empty content; the image is the
    // present. p.show() draws the chrome once, then galleryRefresh()
    // draws the image over it as the final present of this navigation.
    var p = page('gallery')
      .header(col().pad(16, os.M, 6, os.M).gap(8)
        .add(text('GALLERY').size('lg'),
             text(function () { return (g.idx + 1) + ' / ' + n; })
               .size('xs').gray(96)))
      .content(spacer(1, 0))
      .footer(os.hintFooter('swipe = browse - tap = delete - home = menu'));
    p.on(G.LEFT, function () {
      if (g.idx > 0) { g.idx = g.idx - 1; galleryRefresh(); }
    });
    p.on(G.RIGHT, function () {
      if (g.idx + 1 < images.count()) { g.idx = g.idx + 1;
        galleryRefresh(); }
    });
    p.on(G.TAP, function () {
      var nm = images.name(g.idx);
      if (images.remove(nm) === 0) {
        g.idx = 0;
        galleryRefresh();
      }
    });
    os.announce('gallery');
    p.show(true);
    // The image present is the final, topmost blit for this view.
    galleryRefresh();
  }
})
