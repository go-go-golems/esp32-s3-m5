// async-routes.js — Promise-returning route examples for AtomS3R QuickJS.
// Upload to /scripts/async-routes.js, then run:
//   js run /scripts/async-routes.js
// Requires the native HTTP server to be running: http start 80

http.get('/async-promise', function(req) {
  return Promise.resolve({
    status: 200,
    json: {
      ok: true,
      kind: 'promise',
      path: req.path,
    },
  });
});

http.get('/async-await', async function(req) {
  var value = await Promise.resolve('async-value');
  return {
    status: 200,
    json: {
      ok: true,
      kind: value,
      path: req.path,
    },
  };
});

http.get('/async-reject', function(req) {
  return Promise.reject(new Error('route boom'));
});

print('installed async routes: /async-promise /async-await /async-reject');
