(function () {
  var name = "comet-trails";
  matrix.anim.unregister(name);

  matrix.anim.register(name, function (ctx) {
    ctx.matrix.clear();
    var w = ctx.matrix.width();
    var h = ctx.matrix.height();
    var n = w * h;
    var frameMs = ((ctx.opts && ctx.opts.frameMs) | 0) || 45;
    var kComets = ((ctx.opts && ctx.opts.comets) | 0) || 7;

    var trails = [];
    var i;
    for (i = 0; i < n; i++) trails[i] = 0;

    var comets = [];
    for (i = 0; i < kComets; i++) {
      comets.push({
        x: Math.random() * w,
        y: Math.random() * h,
        vx: (Math.random() * 1.4) - 0.7,
        vy: (Math.random() * 1.2) - 0.6
      });
    }

    function idx(x, y) {
      return y * w + x;
    }

    function wrapX(x) {
      while (x < 0) x += w;
      while (x >= w) x -= w;
      return x;
    }

    function wrapY(y) {
      while (y < 0) y += h;
      while (y >= h) y -= h;
      return y;
    }

    function addTrail(x, y, v) {
      var p = idx(x, y);
      var nv = trails[p] + v;
      trails[p] = (nv > 255) ? 255 : nv;
    }

    var tick = 0;
    ctx.every(frameMs, function () {
      if (ctx.shouldStop()) return;

      for (i = 0; i < n; i++) {
        var d = trails[i] - 16;
        trails[i] = (d > 0) ? d : 0;
      }

      var c;
      for (c = 0; c < kComets; c++) {
        var m = comets[c];
        if ((Math.random() < 0.08) || (Math.abs(m.vx) < 0.06)) m.vx += (Math.random() * 0.4) - 0.2;
        if ((Math.random() < 0.08) || (Math.abs(m.vy) < 0.06)) m.vy += (Math.random() * 0.3) - 0.15;
        if (m.vx > 1.2) m.vx = 1.2;
        if (m.vx < -1.2) m.vx = -1.2;
        if (m.vy > 0.9) m.vy = 0.9;
        if (m.vy < -0.9) m.vy = -0.9;

        m.x = wrapX(m.x + m.vx);
        m.y = wrapY(m.y + m.vy);

        var xi = m.x | 0;
        var yi = m.y | 0;
        addTrail(xi, yi, 160);
        addTrail(wrapX(xi - 1), yi, 90);
        addTrail(wrapX(xi + 1), yi, 90);
        addTrail(xi, wrapY(yi - 1), 70);
        addTrail(xi, wrapY(yi + 1), 70);
      }

      ctx.matrix.clear();
      for (var y = 0; y < h; y++) {
        for (var x = 0; x < w; x++) {
          var t = trails[idx(x, y)];
          if (t > 170) {
            ctx.matrix.setPixel(x, y, 1);
          } else if (t > 95 && ((x + y + tick) % 2 === 0)) {
            ctx.matrix.setPixel(x, y, 1);
          } else if (t > 45 && ((x + tick) % 3 === 0)) {
            ctx.matrix.setPixel(x, y, 1);
          }
        }
      }
      ctx.matrix.present();
      tick++;
    });

    return function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    };
  });

  matrix.anim.start(name, { frameMs: 45, comets: 7 });
  return JSON.stringify(matrix.anim.status());
})()
