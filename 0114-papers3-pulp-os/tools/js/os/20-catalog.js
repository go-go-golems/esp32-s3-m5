// ------------------------------------------------------------ catalog --
// App registry. In the one-image phases the build script wraps every
// tools/js/apps/<id>.js (a bare descriptor expression) as
//   APPS['<id>'] = (<file>);
// ROM_ORDER is the launcher row order; entries not listed (reader) are
// launchable via os.launch but not shown.
var APPS = {};
var ROM_ORDER = ['library', 'dice', 'blitz', '2048', 'tea', 'postcard',
                 'daily', 'ink', 'gallery', 'radio', 'settings'];
