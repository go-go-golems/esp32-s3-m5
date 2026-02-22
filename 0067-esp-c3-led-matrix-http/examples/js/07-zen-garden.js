(function () {
  var name = "zen-garden";
  matrix.anim.unregister(name);

  matrix.anim.register(name, function (ctx) {
    var w = ctx.matrix.width(), h = ctx.matrix.height();
    var t = 0;

    // Ripple pool: up to 4 concurrent ripples
    var rips = [];
    var maxRip = 4;

    // Seed first ripple from center
    rips.push({ x: (w / 2) | 0, y: (h / 2) | 0, age: 0 });

    // Raked sand lines: sparse horizontal dashes that drift very slowly
    var sandOff = 0;

    ctx.every(140, function () {
      if (ctx.shouldStop()) return;
      ctx.matrix.clear();

      // Layer 1: raked sand texture (very sparse, drifting)
      for (var y = 0; y < h; y += 3) {
        for (var x = (sandOff + y * 7) % 12; x < w; x += 12) {
          ctx.matrix.setPixel(x, y, 1);
          if (x + 1 < w) ctx.matrix.setPixel(x + 1, y, 1);
        }
      }
      if (t % 5 === 0) sandOff = (sandOff + 1) % 12;

      // Layer 2: breathing center stone (pulsing cross)
      var cx = (w / 2) | 0, cy = (h / 2) | 0;
      var breath = Math.sin(t * 0.06);
      ctx.matrix.setPixel(cx, cy, 1);
      if (breath > -0.3) {
        ctx.matrix.setPixel(cx - 1, cy, 1);
        ctx.matrix.setPixel(cx + 1, cy, 1);
        ctx.matrix.setPixel(cx, cy - 1, 1);
        ctx.matrix.setPixel(cx, cy + 1, 1);
      }
      if (breath > 0.5) {
        ctx.matrix.setPixel(cx - 2, cy, 1);
        ctx.matrix.setPixel(cx + 2, cy, 1);
      }

      // Layer 3: expanding ripple rings (raindrop in still water)
      for (var r = rips.length - 1; r >= 0; r--) {
        var rp = rips[r];
        var rad = rp.age * 0.7;
        rp.age++;
        // remove ripple once fully offscreen
        if (rad > 55) { rips.splice(r, 1); continue; }
        // draw ring with 28 sample points
        var fade = rp.age > 20;
        for (var a = 0; a < 28; a++) {
          var ang = a * 0.224;
          var rx = (rp.x + rad * Math.cos(ang) + 0.5) | 0;
          var ry = (rp.y + rad * Math.sin(ang) + 0.5) | 0;
          if (rx >= 0 && rx < w && ry >= 0 && ry < h) {
            // fade: skip more pixels as ripple ages
            if (!fade || (a + rp.age) % 2 === 0)
              ctx.matrix.setPixel(rx, ry, 1);
          }
        }
      }

      // Layer 4: slow drifting firefly
      var fx = ((w / 2) + Math.sin(t * 0.023) * 35 + Math.sin(t * 0.057) * 8) | 0;
      var fy = ((h / 2) + Math.sin(t * 0.031) * 2.8) | 0;
      if (fx >= 0 && fx < w && fy >= 0 && fy < h) {
        ctx.matrix.setPixel(fx, fy, 1);
        // gentle blink: dim cross every few ticks
        if (t % 7 < 4) {
          if (fx > 0) ctx.matrix.setPixel(fx - 1, fy, 1);
          if (fx < w - 1) ctx.matrix.setPixel(fx + 1, fy, 1);
        }
      }

      ctx.matrix.present();

      // Occasionally drop a new ripple (like a raindrop)
      if (t % 22 === 0 && rips.length < maxRip) {
        rips.push({
          x: (Math.random() * w * 0.7 + w * 0.15) | 0,
          y: (Math.random() * h * 0.5 + h * 0.25) | 0,
          age: 0
        });
      }

      t++;
    });

    return function () {
      ctx.matrix.clear();
      ctx.matrix.present();
    };
  });

  matrix.anim.start(name, {});
  return JSON.stringify(matrix.anim.status());
})()
