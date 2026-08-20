// hostile page: tries to escape, then spins. Expected: denials
// throw, the loop is killed by the deadline, the browser shows an error.
({
  title: 'Hostile',
  main: function (ui, nav) {
    try { files.remove('/apps/dice.js', function () {}); } catch (e) {}
    try { wifi.forget('yolobolo'); } catch (e2) {}
    for (;;) { }
  }
})
