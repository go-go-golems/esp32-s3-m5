(function () {
  matrix.stop();
  matrix.clear();
  matrix.present();
  matrix.setIntensity(15);
  return JSON.stringify({
    step: "00-env-status",
    width: matrix.width(),
    height: matrix.height(),
    status: matrix.status()
  });
})()
