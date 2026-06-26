// http-static-and-routes.js — AtomS3R QuickJS HTTP route examples.
// Upload to /scripts/http-static-and-routes.js, then run:
//   js run /scripts/http-static-and-routes.js
// Requires the native HTTP server to be running: http start 80

http.static('/static', '/data');

http.get('/api/hello', function(req) {
  return {
    status: 200,
    json: {
      ok: true,
      route: '/api/hello',
      method: req.method,
      path: req.path,
      board: system.board,
    },
  };
});

http.get('/api/status', function(req) {
  return {
    status: 200,
    json: {
      ok: true,
      http: http.status(),
      wifi: wifi.status(),
      storage: storage.status(),
    },
  };
});

print('installed routes: /api/hello /api/status and static /static -> /data');
