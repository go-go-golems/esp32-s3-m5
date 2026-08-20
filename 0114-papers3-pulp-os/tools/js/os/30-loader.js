// ------------------------------------------------------------- loader --
// launch(id, arg): resolve the descriptor, validate, cross the app-switch
// boundary, run main. Failures land on the error page, never in a broken
// tree: enter() runs only after validation.

var RUN = { id: 'home', desc: null };

function launch(id, arg) {
  var desc = APPS.hasOwnProperty(id) ? APPS[id] : null;
  if (!desc) { errorPage(id, 'not in catalog'); return; }
  if (typeof desc.main !== 'function' || desc.id !== id ||
      desc.abi !== os.abi) {
    errorPage(id, 'bad descriptor (abi ' + desc.abi + ')');
    return;
  }
  RUN.id = id;
  RUN.desc = desc;
  enter(id);
  try {
    desc.main(os, arg);
  } catch (e) {
    errorPage(id, 'crashed: ' + e);
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
