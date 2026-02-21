(function () {
  matrix.stop();
  matrix.clear();
  matrix.setPixel(0, 0, 1);
  matrix.present();
  return JSON.stringify({
    step: "03-single-pixel",
    pixel00: matrix.getPixel(0, 0)
  });
})()
