# AtomS3R QuickJS example scripts

These scripts are source examples for the on-device `/scripts` storage root.
They are not embedded automatically and are not auto-run at boot.

The intended recovery-safe workflow is:

1. Flash/monitor over USB Serial/JTAG.
2. Start native services from the console, for example `http start 80`.
3. Copy a script into `/scripts/...` through the storage namespace.
4. Run it explicitly with `js run /scripts/name.js`.
5. If a script is bad, run `js reset` to clear QuickJS-owned route handlers.

Current examples:

- `http-static-and-routes.js` registers `/api/hello`, `/api/status`, and `/static -> /data`.
- `async-routes.js` registers Promise-returning `/async-promise`, async-function `/async-await`, and rejecting `/async-reject` routes.
- `fetch-healthz.js` calls worker-backed `fetch('http://<device-ip>/healthz')` and prints the response after the Promise settles.

## Console upload examples

For compact one-line scripts, the console command is enough:

```text
storage write /scripts/server.js http.get('/run/hello',function(req){return{json:{ok:true,path:req.path}};})
js run /scripts/server.js
```

For multiline scripts, use JavaScript-side `storage.writeText()` with an escaped string, or paste a minified version. A larger upload helper can be added later; boot-time autoload should remain disabled unless there is a recovery switch.

Promise-returning dynamic routes are supported for Promises that settle during the route dispatch job. A route that returns a never-settling Promise returns `504 Gateway Timeout`; a route that returns a rejected Promise returns `500 Internal Server Error`.

Firmware `fetch()` runs HTTP I/O on a single native worker task and settles the JavaScript Promise back on the QuickJS owner task. Pending fetches survive after `js eval` returns, so output from `.then()` and `.catch()` callbacks may appear at the prompt later. `js reset` cancels pending fetch Promises and ignores stale worker completions.
