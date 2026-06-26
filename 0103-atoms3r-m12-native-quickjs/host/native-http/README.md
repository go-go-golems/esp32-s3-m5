# native-http host

`native-http` is a desktop QuickJS host for the 0103 AtomS3R HTTP namespace and
bounded `fetch()` API. It mirrors the 0102 native-host split:

- portable/firmware-oriented code lives in `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.*`,
- host-only code lives in `host/native-http/src/*`,
- the host compiles upstream QuickJS directly and runs scripts without ESP-IDF or device hardware.

## Build

```bash
make -C 0103-atoms3r-m12-native-quickjs/host/native-http all
```

## Run

```bash
0103-atoms3r-m12-native-quickjs/host/native-http/build/qjs-http-host \
  0103-atoms3r-m12-native-quickjs/host/native-http/examples/server.js \
  --dispatch /api/hello
```

## Smoke

```bash
0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh
```

The host currently supports direct route dispatch for `http.get()` and a simple
POSIX-socket `http://` fetch adapter. It is intentionally not a full browser or
full Express clone.
