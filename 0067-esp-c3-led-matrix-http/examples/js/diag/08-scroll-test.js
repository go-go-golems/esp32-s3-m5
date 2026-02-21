(function () {
  if (globalThis.__matrix_anim && typeof globalThis.__matrix_anim.cancel === "function") {
    globalThis.__matrix_anim.cancel();
  }
  matrix.stop();
  matrix.startScroll("HELLO 123", { fps: 20, pauseMs: 200, repeat: 0, wave: false });
  return "diag 08: scroll HELLO 123";
})()
