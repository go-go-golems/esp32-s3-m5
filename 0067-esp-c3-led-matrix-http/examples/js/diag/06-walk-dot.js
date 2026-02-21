(function () {
  if (globalThis.__matrix_anim && typeof globalThis.__matrix_anim.cancel === "function") {
    globalThis.__matrix_anim.cancel();
  }
  matrix.stop();
  matrix.clear();
  matrix.present();

  var w = matrix.width();
  var x = 0;
  var y = 3;
  var handle = null;
  handle = every(70, function () {
    if (matrix.shouldStop()) {
      handle.cancel();
      return;
    }
    matrix.clear();
    matrix.setPixel(x, y, 1);
    matrix.present();
    x++;
    if (x >= w) x = 0;
  });

  globalThis.__matrix_anim = handle;
  globalThis.__matrix_anim_name = "diag-walk-dot";
  return "diag 06: walking dot started";
})()
