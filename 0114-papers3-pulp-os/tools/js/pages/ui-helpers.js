// ------------------------------------------------------------ ui-helpers --
// Evaluated into every page context before the page script runs. This is
// the whole cosmetic vocabulary a page shares with PULP OS; everything
// else a page can do is the builder API plus nav.
var UIM = 40;
var ui = {
  M: UIM,
  chrome: function (title) {
    return col().pad(16, UIM, 6, UIM).gap(8)
      .add(text(title).size('lg'), divider(6, 0));
  },
  hintFooter: function (hint) {
    return col().pad(4, UIM, 10, UIM).gap(5)
      .add(divider(1, 0), text(hint).size('xs').gray(96));
  },
  row: function (label, sub, fn) {
    var line = row().pad(6, 0, 4, 0).gap(10).crossAlign(3)
      .add(text(label).size('lg'), spacer(0, 1),
           text(sub).size('xs').gray(112));
    var entry = col().pad(0, UIM, 0, UIM).add(line, divider(2, 0));
    if (fn) { entry.onTap(fn); }
    return entry;
  }
};
