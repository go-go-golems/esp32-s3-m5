(function () {
  matrix.stop();
  matrix.clear();
  var w = matrix.width();
  var h = matrix.height();
  var x, y;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      if (((x + y) % 2) === 0) matrix.setPixel(x, y, 1);
    }
  }
  matrix.present();
  return "diag 05: checkerboard";
})()
