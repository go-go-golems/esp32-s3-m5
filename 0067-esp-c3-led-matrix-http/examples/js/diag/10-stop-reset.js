(function () {
  if (globalThis.__matrix_anim && typeof globalThis.__matrix_anim.cancel === "function") {
    globalThis.__matrix_anim.cancel();
  }
  matrix.stop();
  matrix.clear();
  matrix.present();
  return "diag 10: stopped and cleared";
})()
