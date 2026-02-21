(function () {
  matrix.stop();
  matrix.clear();
  var w = matrix.width();
  var h = matrix.height();
  var x, y;
  for (x = 0; x < w; x++) {
    matrix.setPixel(x, 0, 1);
    matrix.setPixel(x, h - 1, 1);
  }
  for (y = 0; y < h; y++) {
    matrix.setPixel(0, y, 1);
    matrix.setPixel(w - 1, y, 1);
  }
  matrix.present();
  return "diag 04: border";
})()
