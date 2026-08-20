// ------------------------------------------------------------ catalog --
// App catalog (ESP-55 P3). Built-in apps live as flash assets
// (load('rom:<id>')); the entry carries the launcher metadata so listing
// never evaluates a module. `hidden` entries (reader) are launchable via
// os.launch but not shown. Keep ROM_APPS in launcher row order.
// ROM subtitles may be functions (evaluated at row-build time); SD
// manifest subtitles (Phase 4) are strings only.
var ROM_APPS = [
  { id: 'library', title: 'Reader', subtitle: 'books on the card',
    src: 'rom:library' },
  { id: 'reader', title: 'Reader page', subtitle: 'the open book',
    src: 'rom:reader', hidden: true },
  { id: 'dice', title: 'Dice Tray', subtitle: '2d6 coin d20 d%',
    src: 'rom:dice' },
  { id: 'blitz', title: 'Blitz Ink', subtitle: 'chess clock 5+3',
    src: 'rom:blitz' },
  { id: '2048', title: '2048 INK',
    subtitle: function () { return 'best ' + storeGet('2048best', 0); },
    src: 'rom:2048' },
  { id: 'tea', title: 'Tea Timer', subtitle: 'steep watch',
    src: 'rom:tea' },
  { id: 'postcard', title: 'Postcard', subtitle: 'one line a day',
    src: 'rom:postcard' },
  { id: 'daily', title: 'Daily Pulp', subtitle: 'a page at random',
    src: 'rom:daily' },
  { id: 'ink', title: 'Ink', subtitle: 'the beauty of e-ink',
    src: 'rom:ink' },
  { id: 'gallery', title: 'Gallery', subtitle: 'your pictures',
    src: 'rom:gallery' },
  { id: 'radio', title: 'Radio', subtitle: 'words from the ether',
    src: 'rom:radio' },
  { id: 'settings', title: 'Settings', subtitle: 'wifi - serve - margins',
    src: 'rom:settings' }
];

function catalogFind(id) {
  var i;
  for (i = 0; i < ROM_APPS.length; i++) {
    if (ROM_APPS[i].id === id) { return ROM_APPS[i]; }
  }
  return null;
}
