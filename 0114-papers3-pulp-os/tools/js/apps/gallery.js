// ------------------------------------------------------------ gallery --
// ESP-54: display stored .g4 images full-screen, swipe left/right to
// browse, tap to delete. images.display() does a clean-full present of
// the bitmap; the chrome (counter + name + hint) is drawn in the SAME
// frame so there is exactly one present per navigation.
var GAL = { idx: 0 };

function galleryRefresh() {
  var n = images.count();
  if (n === 0) { gallery(); return; }
  if (GAL.idx >= n) { GAL.idx = n - 1; }
  if (GAL.idx < 0) { GAL.idx = 0; }
  // The image present (clean full) is the primary; the hint is overlaid.
  var ok = images.display(images.name(GAL.idx));
  print('gallery: ' + (GAL.idx + 1) + '/' + n + ' '
        + images.name(GAL.idx) + ' -> ' + ok);
}

function gallery() {
  enter('gallery');
  var n = images.count();
  if (n === 0) {
    var url = mdns.url() !== '' ? mdns.url()
             : (serve.url() !== '' ? serve.url() : 'pulp.local');
    var body0 = col().pad(20, M, 0, M).gap(8)
      .add(text('no images yet').size('md').gray(96),
           text('upload at ' + url).size('xs').gray(128));
    var p0 = page('gallery').header(chrome('GALLERY')).content(body0)
      .footer(hintFooter('upload a picture from your browser'));
    announce('gallery');
    p0.show(true);
    return;
  }
  paper.refreshTurns(1);
  // A page with gesture handlers but empty content; the image is the
  // present. p.show() draws the chrome once, then galleryRefresh()
  // draws the image over it as the final present of this navigation.
  var p = page('gallery')
    .header(col().pad(16, M, 6, M).gap(8)
      .add(text('GALLERY').size('lg'),
           text(function () { return (GAL.idx + 1) + ' / ' + n; })
             .size('xs').gray(96)))
    .content(spacer(1, 0))
    .footer(hintFooter('swipe = browse - tap = delete - home = menu'));
  p.on(G.LEFT, function () {
    if (GAL.idx > 0) { GAL.idx = GAL.idx - 1; galleryRefresh(); }
  });
  p.on(G.RIGHT, function () {
    if (GAL.idx + 1 < images.count()) { GAL.idx = GAL.idx + 1;
      galleryRefresh(); }
  });
  p.on(G.TAP, function () {
    var nm = images.name(GAL.idx);
    if (images.remove(nm) === 0) {
      GAL.idx = 0;
      galleryRefresh();
    }
  });
  announce('gallery');
  p.show(true);
  // The image present is the final, topmost blit for this view.
  galleryRefresh();
}

