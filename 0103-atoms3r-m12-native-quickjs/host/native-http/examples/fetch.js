fetch('http://127.0.0.1:18080/healthz', { timeoutMs: 1000 })
  .then(async (r) => {
    print('fetch status=' + r.status + ' ok=' + r.ok);
    print('fetch body=' + (await r.text()));
  })
  .catch((err) => {
    print('fetch error=' + err.message);
    throw err;
  });
