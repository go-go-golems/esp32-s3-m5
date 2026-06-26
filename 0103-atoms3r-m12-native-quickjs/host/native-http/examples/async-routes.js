print('native-http async route example boot');

http.get('/api/promise', function(req) {
  return Promise.resolve({
    status: 200,
    json: {
      ok: true,
      kind: 'promise',
      path: req.path,
    },
  });
});

http.get('/api/async', async function(req) {
  const value = await Promise.resolve('async-value');
  return {
    status: 200,
    json: {
      ok: true,
      kind: value,
      path: req.path,
    },
  };
});

http.get('/api/reject', function(req) {
  return Promise.reject(new Error('route boom'));
});

const st = http.status();
print('routes=' + st.routes.length + ' mounts=' + st.staticMounts.length);
