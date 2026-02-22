(function () {
  var name = "superbowl";
  matrix.anim.unregister(name);

  matrix.anim.register(name, function (ctx) {
    var w = ctx.matrix.width(), h = ctx.matrix.height();
    var t = 0, phase = 0, pt = 0;

    // Phase durations in ticks (at 25ms each)
    // 0: countdown  1: "SUPER"+starburst  2: "BOWL"+sparkle rain
    // 3: stadium wave  4: "2026!"+confetti  5: grand finale strobe
    var pLen = [40, 55, 50, 55, 40, 60];
    var words = ["SUPER", " BOWL", "2026!", " GO!!"];
    var wIdx = 0;

    // Starburst explosion centers
    var bx = [], by = [];
    function newBurst(n) {
      bx = []; by = [];
      for (var i = 0; i < n; i++) {
        bx.push((Math.random() * w * 0.8 + w * 0.1) | 0);
        by.push((Math.random() * h) | 0);
      }
    }

    ctx.every(25, function () {
      if (ctx.shouldStop()) return;
      pt++;

      if (phase === 0) {
        // Countdown: "3" "2" "1" with expanding ring pulses
        var digit = 3 - ((pt / 13) | 0);
        if (digit < 1) digit = 1;
        ctx.matrix.clear();
        var cx = w / 2, cy = h / 2;
        var rad = (pt % 13) * 1.2;
        for (var a = 0; a < 20; a++) {
          var rx = (cx + rad * Math.cos(a * 0.314)) | 0;
          var ry = (cy + rad * Math.sin(a * 0.314)) | 0;
          if (rx >= 0 && rx < w && ry >= 0 && ry < h)
            ctx.matrix.setPixel(rx, ry, 1);
        }
        ctx.matrix.present();
        if (pt % 13 === 1) matrix.setText("     " + digit);
        if (pt % 13 === 7) { ctx.matrix.clear(); ctx.matrix.present(); }

      } else if (phase === 1) {
        // "SUPER" text flash then starburst explosions
        if (pt === 1) { matrix.setText("  " + words[wIdx]); newBurst(4); wIdx = (wIdx + 1) % 4; }
        matrix.setIntensity((pt % 3 === 0) ? 15 : 5);
        if (pt > 20) {
          ctx.matrix.clear();
          var dd = (pt - 20) * 2.0;
          for (var b = 0; b < bx.length; b++) {
            for (var a = 0; a < 16; a++) {
              var ang = a * 0.393;
              var co = Math.cos(ang), si = Math.sin(ang);
              var rx = (bx[b] + dd * co) | 0;
              var ry = (by[b] + dd * si) | 0;
              if (rx >= 0 && rx < w && ry >= 0 && ry < h)
                ctx.matrix.setPixel(rx, ry, 1);
              // Inner ring for thickness
              if (dd > 3) {
                var rx2 = (bx[b] + (dd - 2) * co) | 0;
                var ry2 = (by[b] + (dd - 2) * si) | 0;
                if (rx2 >= 0 && rx2 < w && ry2 >= 0 && ry2 < h)
                  ctx.matrix.setPixel(rx2, ry2, 1);
              }
            }
          }
          ctx.matrix.present();
        }

      } else if (phase === 2) {
        // Cascading sparkle rain with horizontal flash bars
        if (pt === 1) matrix.setText("  " + words[wIdx]);
        wIdx = (wIdx + 1) % 4;
        matrix.setIntensity((pt % 4 < 2) ? 14 : 4);
        if (pt > 18) {
          ctx.matrix.clear();
          for (var x = 0; x < w; x++) {
            // Falling sparkle columns at different speeds
            var speed = 1 + (x % 3);
            var yy = (pt * speed + x * 7) % (h + 4);
            if (yy < h) ctx.matrix.setPixel(x, yy, 1);
            // Secondary slower layer
            var yy2 = (pt + x * 13) % (h + 6);
            if (yy2 < h && (x + pt) % 3 === 0)
              ctx.matrix.setPixel(x, yy2, 1);
          }
          // Horizontal flash bar
          if (pt % 10 === 0) {
            var fy = (Math.random() * h) | 0;
            for (var fx = 0; fx < w; fx++) ctx.matrix.setPixel(fx, fy, 1);
          }
          ctx.matrix.present();
        }

      } else if (phase === 3) {
        // Stadium wave: bright bar sweeping left to right with sparkle wake
        ctx.matrix.clear();
        var barX = ((pt * 3) % (w + 20)) - 10;
        for (var x = 0; x < w; x++) {
          var dist = Math.abs(x - barX);
          if (dist < 5) {
            for (var y = 0; y < h; y++) {
              if (dist < 2)
                ctx.matrix.setPixel(x, y, 1);
              else if ((x + y + t) % 2 === 0)
                ctx.matrix.setPixel(x, y, 1);
            }
          }
          // Trailing sparkle wake
          var wake = x - barX;
          if (wake > 0 && wake < 20 && Math.random() < 0.06)
            ctx.matrix.setPixel(x, (Math.random() * h) | 0, 1);
        }
        ctx.matrix.present();

      } else if (phase === 4) {
        // Confetti burst with diagonal streaks
        if (pt === 1) matrix.setText("  " + words[wIdx]);
        wIdx = (wIdx + 1) % 4;
        matrix.setIntensity((pt % 5 < 3) ? 15 : 3);
        if (pt > 15) {
          ctx.matrix.clear();
          for (var c = 0; c < 30; c++) {
            var cx = ((c * 37 + pt * 5) % w);
            var cy = ((c * 13 + pt * 2) % h);
            ctx.matrix.setPixel(cx, cy, 1);
          }
          for (var s = 0; s < 6; s++) {
            var sx = ((s * 16 + pt * 3) % w);
            var sy = (pt + s * 2) % h;
            ctx.matrix.setPixel(sx, sy, 1);
            if (sx + 1 < w && sy + 1 < h)
              ctx.matrix.setPixel(sx + 1, sy, 1);
          }
          ctx.matrix.present();
        }

      } else if (phase === 5) {
        // Grand finale: rapid full-screen strobe then decay to scattered sparkle
        if (pt < 20) {
          ctx.matrix.fill((pt % 2 === 0) ? 1 : 0);
          ctx.matrix.present();
          matrix.setIntensity((pt % 2 === 0) ? 15 : 0);
        } else {
          ctx.matrix.clear();
          var density = 1.0 - ((pt - 20) / 40);
          if (density < 0) density = 0;
          for (var x = 0; x < w; x++) {
            for (var y = 0; y < h; y++) {
              if (Math.random() < density)
                ctx.matrix.setPixel(x, y, 1);
            }
          }
          ctx.matrix.present();
          matrix.setIntensity(8);
        }
      }

      t++;
      if (pt >= pLen[phase]) {
        phase = (phase + 1) % pLen.length;
        pt = 0;
        matrix.setIntensity(8);
      }
    });

    return function () {
      matrix.setIntensity(3);
      ctx.matrix.clear();
      ctx.matrix.present();
    };
  });

  matrix.anim.start(name, {});
  return JSON.stringify(matrix.anim.status());
})()
