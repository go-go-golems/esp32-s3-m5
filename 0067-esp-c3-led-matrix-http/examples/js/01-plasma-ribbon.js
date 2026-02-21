(function () {
  if (globalThis.__matrix_anim && typeof globalThis.__matrix_anim.cancel === "function") {
    globalThis.__matrix_anim.cancel();
  }

  matrix.stop();
  matrix.clear();

  var w = matrix.width();
  var h = matrix.height();
  var t = 0;
  var frameMs = 40;

  function clampY(v) {
    if (v < 0) return 0;
    if (v >= h) return h - 1;
    return v;
  }

  var handle = null;
  handle = every(frameMs, function () {
    if (matrix.shouldStop()) {
      if (handle) handle.cancel();
      return;
    }

    matrix.clear();

    var x;
    for (x = 0; x < w; x++) {
      var y1f = 3.5 + 2.2 * Math.sin((x * 0.24) + t) + 0.7 * Math.sin((x * 0.09) - (t * 1.9));
      var y2f = 3.5 + 2.0 * Math.sin((x * 0.18) - (t * 1.4)) + 0.8 * Math.cos((x * 0.13) + (t * 1.2));
      var y1 = clampY(y1f | 0);
      var y2 = clampY(y2f | 0);

      matrix.setPixel(x, y1, 1);
      matrix.setPixel(x, y2, 1);

      if (((x + (t * 10)) | 0) % 9 === 0) {
        var tail = clampY(((y1 + y2) / 2) | 0);
        matrix.setPixel(x, tail, 1);
      }
    }

    matrix.present();
    t += 0.13;
  });

  globalThis.__matrix_anim = handle;
  globalThis.__matrix_anim_name = "plasma-ribbon";
  return "plasma-ribbon started";
})()
