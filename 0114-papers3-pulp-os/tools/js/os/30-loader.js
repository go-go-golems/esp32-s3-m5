// ------------------------------------------------------------- loader --
// launch(id, arg): catalog lookup -> drop the previous module + gc (the
// parser competes with garbage for arena headroom) -> load() -> validate
// -> app-switch boundary -> main. Failures land on the error page; the
// previous page stays presented until validation has passed, so a broken
// module never leaves a half-torn tree.

var RUN = { id: 'home', desc: null };

function launch(id, arg) {
  var e = catalogFind(id);
  if (!e) { errorPage(id, 'not in catalog'); return; }
  RUN.desc = null;
  gc();
  var desc = null;
  try {
    desc = load(e.src);
  } catch (ex) {
    errorPage(id, 'load failed: ' + ex);
    return;
  }
  if (!desc || typeof desc.main !== 'function' || desc.id !== id ||
      desc.abi !== os.abi) {
    errorPage(id, 'bad descriptor (abi ' + (desc && desc.abi) + ')');
    return;
  }
  RUN.id = id;
  RUN.desc = desc;
  enter(id);
  try {
    desc.main(os, arg);
  } catch (ex2) {
    errorPage(id, 'crashed: ' + ex2);
  }
}

function errorPage(id, why) {
  enter('error');
  print('pulp screen: error/' + id + ' ' + why);
  var p = page('error').header(chrome('APP ERROR')).content(
    col().pad(20, M, 0, M).gap(10).add(
      text(id).size('lg'),
      text('' + why).size('xs').gray(96),
      text('[ back ]').size('xs').center().width(160).height(56)
        .onTap(function () { home(); })))
    .footer(hintFooter('swipe down = home'));
  p.show(true);
}
