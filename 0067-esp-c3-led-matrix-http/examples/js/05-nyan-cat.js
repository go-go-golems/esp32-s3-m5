(function () {
  var name = "nyan-cat";
  matrix.anim.unregister(name);

  matrix.anim.register(name, function (ctx) {
    var w = ctx.matrix.width(), h = ctx.matrix.height();

    // Cat sprite: 8 wide x 6 tall, 2 animation frames (legs spread / together)
    var sp = [
      ".X....X.", ".XXXXXX.", "XX.XX.XX",
      ".XXXXXX.", "..XXXX..", ".X....X.",
      ".X....X.", ".XXXXXX.", "XX.XX.XX",
      ".XXXXXX.", "..XXXX..", "..X..X.."
    ];
    var SW = 8, SH = 6;

    var catX = -SW;  // start offscreen left, run rightward
    var frame = 0, t = 0, magic = 0;

    ctx.every(30, function () {
      if (ctx.shouldStop()) return;

      // MAGIC text phase: strobe "MAGIC" using built-in text engine
      if (magic > 0) {
        magic--;
        if (magic === 28) matrix.setText("   MAGIC");
        // intensity strobe for flash effect
        matrix.setIntensity((magic % 6 < 3) ? 13 : 2);
        if (magic === 0) {
          matrix.setIntensity(3);
          catX = -SW;  // cat re-enters from left
        }
        return;
      }

      ctx.matrix.clear();

      // Rainbow trail: 5 sine-wobbled bands streaming behind the cat
      var trailEnd = catX > 0 ? catX : 0;
      for (var x = 0; x < trailEnd && x < w; x++) {
        for (var b = 0; b < 5; b++) {
          var by = ((b + 1.5) + Math.sin(x * 0.12 + t * 0.25 + b * 1.1) * 0.9) | 0;
          if (by >= 0 && by < h) {
            // alternating dither per band for pseudo-color layering
            if (b % 2 === 0 || (x + t) % 2 === 0)
              ctx.matrix.setPixel(x, by, 1);
          }
        }
      }

      // Sparkle stars near the cat (nyan-cat signature)
      for (var s = 0; s < 4; s++) {
        var sx = catX - 4 - ((t * 5 + s * 23) % 18);
        var sy = (s * 17 + t * 3) % h;
        if (sx >= 0 && sx < w) {
          ctx.matrix.setPixel(sx, sy, 1);
          // cross twinkle on alternating ticks
          if ((t + s) % 3 === 0) {
            if (sy > 0) ctx.matrix.setPixel(sx, sy - 1, 1);
            if (sy < h - 1) ctx.matrix.setPixel(sx, sy + 1, 1);
          }
        }
      }

      // Cat sprite with vertical bob
      var bob = (Math.sin(t * 0.22) * 1.2 + 0.5) | 0;
      var fo = (frame & 1) * SH;
      for (var sy = 0; sy < SH; sy++) {
        var row = sp[fo + sy];
        for (var sx = 0; sx < SW; sx++) {
          if (row.charAt(sx) === "X") {
            var px = catX + sx, py = sy + bob;
            if (px >= 0 && px < w && py >= 0 && py < h)
              ctx.matrix.setPixel(px, py, 1);
          }
        }
      }

      // Wagging tail behind cat
      var tailX = catX - 1;
      var tailY = 3 + bob + ((Math.sin(t * 0.5) > 0) ? -1 : 0);
      if (tailX >= 0 && tailX < w && tailY >= 0 && tailY < h)
        ctx.matrix.setPixel(tailX, tailY, 1);

      ctx.matrix.present();
      catX++;
      t++;
      if (t % 3 === 0) frame++;
      // cat exits right -> trigger MAGIC flash
      if (catX > w + 2) magic = 32;
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
