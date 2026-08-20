// ----------------------------------------------------------------- ink --
// The e-ink showcase: three canvas scenes on one page. Tap = next scene.
// Clock redraws once a minute (one canvas blit); field and ladder arrive
// on a deliberate clean full - the flash is the reveal.

var INK = { scene: 0, lastMin: -1 };

function ink() {
  enter('ink');
  // Scene geometry follows the live canvas width (540 minus the global
  // margins), so the margin toggle keeps compositions centered.
  var W = 540 - 2 * M;
  var CX = Math.floor(W / 2);
  var cv = canvas().height(760);
  var cap = text(' ').size('xs').gray(96).center();
  var p = page('ink')
    .header(col().pad(16, M, 6, M).gap(8)
      .add(text('INK').size('lg'), divider(6, 0)))
    .content(col().pad(6, M, 0, M).gap(8).add(cv, cap));

  function clockFace(min) {
    cv.wipe();
    var cx = CX;
    var cy = 340;
    var r = 190;
    cv.ring(cx, cy, r, 0, 4);
    var i;
    for (i = 0; i < 12; i++) {
      var a = i * Math.PI / 6;
      var ox = Math.round(Math.sin(a) * (r - 14));
      var oy = -Math.round(Math.cos(a) * (r - 14));
      var ix = Math.round(Math.sin(a) * (r - (i % 3 === 0 ? 34 : 24)));
      var iy = -Math.round(Math.cos(a) * (r - (i % 3 === 0 ? 34 : 24)));
      cv.line(cx + ix, cy + iy, cx + ox, cy + oy, 0,
              i % 3 === 0 ? 3 : 1);
    }
    var ma = (min % 60) * Math.PI / 30;
    var ha = ((min / 60) % 12) * Math.PI / 6;
    cv.line(cx, cy, cx + Math.round(Math.sin(ha) * (r - 90)),
            cy - Math.round(Math.cos(ha) * (r - 90)), 0, 5);
    cv.line(cx, cy, cx + Math.round(Math.sin(ma) * (r - 40)),
            cy - Math.round(Math.cos(ma) * (r - 40)), 0, 2);
    cv.disc(cx, cy, 8, 0);
    cv.ring(cx, cy, 12, 0, 2);
    cap.set('a quiet clock - one blit per minute - up ' + min + 'm');
  }

  function field() {
    cv.wipe();
    var i;
    for (i = 0; i < 4; i++) {
      cv.line(Math.floor(Math.random() * W), 0,
              Math.floor(Math.random() * W), 760,
              Math.floor(Math.random() * 160), 1);
    }
    for (i = 0; i < 30; i++) {
      var x = 30 + Math.floor(Math.random() * (W - 60));
      var y = 30 + Math.floor(Math.random() * 700);
      var rr = 6 + Math.floor(Math.random() * 44);
      var g = (i * 37) % 256;
      if (i % 3 === 0) { cv.ring(x, y, rr, g, 2 + (i % 3)); }
      else { cv.disc(x, y, rr, g); }
    }
    cap.set('a field of circles - no two alike');
  }

  function ladder() {
    cv.wipe();
    var cx = CX;
    var cy = 360;
    var i;
    for (i = 0; i < 16; i++) {
      cv.ring(cx, cy, 300 - i * 18, i * 17, 9);
    }
    cv.disc(cx, cy, 300 - 16 * 18, 255);
    var cell = Math.floor((W - 24) / 16);
    var strip = cell * 16;
    var sx = Math.floor((W - strip) / 2);
    cv.box(sx - 2, 700, strip + 4, 40, 0, 2);
    for (i = 0; i < 16; i++) {
      cv.paint(sx + i * cell, 702, cell, 36, i * 17);
    }
    cap.set('sixteen grays - the whole palette');
  }

  function show(scene, full) {
    INK.scene = scene;
    print('pulp screen: ink/' + scene);
    if (scene === 0) {
      INK.lastMin = Math.floor(millis() / 60000);
      clockFace(INK.lastMin);
      p.show(full ? 1 : 0);
    } else if (scene === 1) {
      field();
      p.show(1);
    } else {
      ladder();
      p.show(1);
    }
  }

  p.on(G.TAP, function () { show((INK.scene + 1) % 3, INK.scene === 2); });
  p.on(G.TICK, function () {
    if (INK.scene !== 0) { return; }
    var min = Math.floor(millis() / 60000);
    if (min !== INK.lastMin) {
      INK.lastMin = min;
      clockFace(min);
      p.update();
    }
  });
  p.every(1000);
  show(0, true);
}

