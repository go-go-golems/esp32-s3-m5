(function () {
  if (globalThis.__matrix_anim && typeof globalThis.__matrix_anim.cancel === "function") {
    globalThis.__matrix_anim.cancel();
  }
  matrix.stop();
  matrix.setText("TEST");
  return "diag 07: static text TEST";
})()
