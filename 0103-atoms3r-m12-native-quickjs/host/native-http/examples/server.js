print('native-http example boot');

http.static('/static', '/data');
http.get('/api/hello', (req) => ({
  status: 200,
  json: {
    ok: true,
    method: req.method,
    path: req.path,
  },
}));

const st = http.status();
print('routes=' + st.routes.length + ' mounts=' + st.staticMounts.length);
