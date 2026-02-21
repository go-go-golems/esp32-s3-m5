(function () {
  if (globalThis.__matrix_anim && typeof globalThis.__matrix_anim.cancel === "function") {
    globalThis.__matrix_anim.cancel();
  }

  matrix.stop();
  matrix.clear();

  var w = matrix.width();
  var h = matrix.height();
  var n = w * h;
  var frameMs = 90;

  var a = [];
  var b = [];
  var i;
  for (i = 0; i < n; i++) {
    a[i] = (Math.random() < 0.28) ? 1 : 0;
    b[i] = 0;
  }

  function idx(x, y) {
    return y * w + x;
  }

  function wrapX(x) {
    if (x < 0) return w - 1;
    if (x >= w) return 0;
    return x;
  }

  function wrapY(y) {
    if (y < 0) return h - 1;
    if (y >= h) return 0;
    return y;
  }

  function countNeighbors(x, y) {
    var sum = 0;
    var dx;
    var dy;
    for (dy = -1; dy <= 1; dy++) {
      for (dx = -1; dx <= 1; dx++) {
        if (dx === 0 && dy === 0) continue;
        var xx = wrapX(x + dx);
        var yy = wrapY(y + dy);
        sum += a[idx(xx, yy)];
      }
    }
    return sum;
  }

  var gen = 0;
  var handle = null;
  handle = every(frameMs, function () {
    if (matrix.shouldStop()) {
      if (handle) handle.cancel();
      return;
    }

    var x;
    var y;
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
        var p = idx(x, y);
        var alive = a[p];
        var nb = countNeighbors(x, y);
        if (alive) {
          b[p] = (nb === 2 || nb === 3) ? 1 : 0;
        } else {
          b[p] = (nb === 3) ? 1 : 0;
        }
      }
    }

    matrix.clear();
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
        var q = idx(x, y);
        a[q] = b[q];
        if (a[q]) matrix.setPixel(x, y, 1);
      }
    }
    matrix.present();

    gen++;
    if ((gen % 45) === 0) {
      for (i = 0; i < 24; i++) {
        var rx = (Math.random() * w) | 0;
        var ry = (Math.random() * h) | 0;
        a[idx(rx, ry)] = 1;
      }
    }
  });

  globalThis.__matrix_anim = handle;
  globalThis.__matrix_anim_name = "life-torus";
  return "life-torus started";
})()
