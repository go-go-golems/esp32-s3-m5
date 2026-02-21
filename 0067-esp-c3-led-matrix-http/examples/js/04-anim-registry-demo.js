(function () {
  // Register a pulse effect with explicit cleanup.
  matrix.anim.register("pulse", function (ctx) {
    var on = false;
    var h = ctx.every(120, function () {
      if (ctx.shouldStop()) {
        h.cancel();
        return;
      }
      on = !on;
      ctx.matrix.fill(on ? 1 : 0);
      ctx.matrix.present();
    });

    ctx.onCleanup(function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    });
  });

  // Register a moving dot effect.
  matrix.anim.register("runner", function (ctx) {
    var x = 0;
    var y = 3;
    var h = ctx.every(50, function () {
      if (ctx.shouldStop()) {
        h.cancel();
        return;
      }
      ctx.matrix.clear();
      ctx.matrix.setPixel(x, y, 1);
      ctx.matrix.present();
      x = (x + 1) % ctx.matrix.width();
    });

    ctx.onCleanup(function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    });
  });

  matrix.anim.start("runner", {});
  return JSON.stringify(matrix.anim.status());
})()
