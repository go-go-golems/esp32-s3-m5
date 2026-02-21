(function () {
  if (globalThis.__matrix_anim && typeof globalThis.__matrix_anim.cancel === "function") {
    globalThis.__matrix_anim.cancel();
  }
  matrix.stop();
  matrix.startScroll("WAVE TEST", { fps: 20, pauseMs: 200, repeat: 0, wave: true });
  return "diag 09: wave scroll";
})()
