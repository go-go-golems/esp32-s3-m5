(function () {
  var name = "plasma-ribbon";
  matrix.anim.unregister(name);

  matrix.anim.register(name, function (ctx) {
    ctx.matrix.clear();
    var w = ctx.matrix.width();
    var h = ctx.matrix.height();
    var t = 0;
    var frameMs = ((ctx.opts && ctx.opts.frameMs) | 0) || 10;

    function clampY(v) {
      if (v < 0) return 0;
      if (v >= h) return h - 1;
      return v;
    }

    ctx.every(frameMs, function () {
      if (ctx.shouldStop()) return;

      ctx.matrix.clear();

      var x;
      for (x = 0; x < w; x++) {
        var y1f = 3.5 + 2.2 * Math.sin((x * 0.24) + t) + 0.7 * Math.sin((x * 0.09) - (t * 1.9));
        var y2f = 3.5 + 2.0 * Math.sin((x * 0.18) - (t * 1.4)) + 0.8 * Math.cos((x * 0.13) + (t * 1.2));
        var y1 = clampY(y1f | 0);
        var y2 = clampY(y2f | 0);

        ctx.matrix.setPixel(x, y1, 1);
        ctx.matrix.setPixel(x, y2, 1);

        if (((x + (t * 10)) | 0) % 9 === 0) {
          var tail = clampY(((y1 + y2) / 2) | 0);
          ctx.matrix.setPixel(x, tail, 1);
        }
      }

      ctx.matrix.present();
      t += 0.13;
    });

    return function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    };
  });

  matrix.anim.start(name, { frameMs: 10 });
  return JSON.stringify(matrix.anim.status());
})()
