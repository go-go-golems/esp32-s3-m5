# Evidence: PULP OS v2 native-side map (collected 2026-08-17 for ESP-55)

Collected by a read-only survey of `0114-papers3-pulp-os/main/*` and the
shared components; all paths absolute under
`/home/manuel/code/wesen/go-go-golems/esp32-s3-m5/0114-papers3-pulp-os`.
Line numbers are as of commit 5effe4c9.

## Task / thread model

| Task | Core | Prio | Stack | Created at | Owns |
|---|---|---|---|---|---|
| `ui_owner` | 1 | 5 | 8192 | `main/app_owner.cpp:610-611` | ALL app state: JS context, widget arena, page table, storage, presents, book/library/settings, images catalog, serve route table, wifi/http builder slots. `AssertOwner()` aborts on violation (`app_owner.cpp:597-602`) |
| `touch_tick` | 0 | 4 | 3072 | `main/app_input.cpp:89-90` | GT911 polling at 20 ms; posts `TimerDue` events |
| `http_worker` (transient) | 0 | 4 | 6144 | `main/net_http.cpp:220-221` | one `esp_http_client_perform()`; writes PSRAM body mailbox + `PostModuleDone(Http)`; forbidden from touching JS/arena/storage (`net_http.cpp:111-112`) |
| `httpd` | any | 5 | 4096 | `main/net_serve.cpp:501-524` | request parsing; sanctioned off-owner SD read (static files `:209-238`) and write (POST upload `:118-195`) |
| `mdns` | 0 | 1 | 4096 | managed component | responder; init/add only from owner |
| `sys_evt` | – | – | 2304 | IDF | Wi-Fi/IP events → POD + `PostModuleDone(Wifi)` |
| console REPL | – | – | IDF | `main/app_console.cpp:599-607` | parses commands, posts `AppEvent`, blocks on 4-entry reply queue; cmdline cap 160 |
| `main` | 0 | – | 12288 | IDF | `OwnerStart()`, `ConsoleStart()`, returns (`app_main.cpp:39-53`) |

Owner loop (`app_owner.cpp:556-571`): `xQueueReceive(500 ms)`; timeout → `TickHooks()`; event → `HandleEvent()` + `TickHooks()`. `TickHooks()` (`:495-502`) = `StorageFlushIfDue`, `JsTimerTick`, `BuzzerTick`, `WifiTick`, `PowerAutoTick`. Boot order (`:504-555`): `RuntimeInit(320 KiB frame arena)` → storage mount + seed → `InputServiceInit()` → `JsRunPulp()` (native home only as fallback) → `TouchEnable()`. Event queue: 32 entries (`app_owner.h:14`), `PostEvent` never blocks.

## Event flows

`files.read(path, cb)`: `js_files_read` (`js_files.cpp:97`) → `PathOp` (`:25`): path[96], `RegisterModuleCb(Files)` (throws "module busy" if pending) → `FilesRead` (`app_files.cpp:183-221`): resolve under `/sdcard`, lazy 16 KiB PSRAM body, **synchronous** fopen/fread on the owner, line index (≤512 lines), `Post(kDoneFilesRead, line_count, 0)` → later loop pass: `HandleEvent` → `JsModuleDone(Files, 11, n, 0)` (`app_js.cpp:528-543`) clears the cb slot BEFORE calling → `CallCb` with 1 s deadline. Only callback delivery is deferred; the I/O is synchronous.

HTTP GET on a JS route: httpd `Handler` (`net_serve.cpp:289`) → uri[128], POST only `/images/upload` (`:299-304`) → `FindRoute` exact strcmp over 8 slots → `ServeJsRoute` (`:241`): single `RequestSlot` (busy CAS else 503), `PostModuleDone(Serve, kDoneServeRequest, route, gen)`, wait on semaphore ≤5000 ms → owner `ServeOwnerDispatch` (`:564-585`) → `JsServeInvokeRoute` → `CallCb(cb,1,0,0,1)` → handler calls `serve.text/json/status` → `ServeRespond` (`:587-604`, body clamped 4096) → semaphore give → httpd sends.

## Numeric limits

- JS host (`app_js_internal.h`): `kAbiVersion=2`, `kMaxJsHits=48`, `kMaxPages=12` (name[16]), `kMaxDynValues=48`; `kArenaBytes=192*1024` (`app_js.cpp:53`, PSRAM first); embedded bytecode `kJsBytecode_pulp` ≈45,332 B copied into `MALLOC_CAP_INTERNAL` (`app_js.cpp:111`); `s_last_error[48]`; deadlines: kernel eval 1000 ms, `JsRunPulp` 3 s, every `CallCb` 1 s, probes 3000 ms; `page.every` floor 250 ms.
- Widget core: `WidgetArena::kCapacity=128`, `TextProps::kCapacity=64`; frame arena 320 KiB.
- files (`app_files.h`): path 96, list 32 entries (names >39 chars skipped), body 16384 (PSRAM), lines 512; path charset `[A-Za-z0-9._-/]`, no `..`, no dot-segments, no `//`, no trailing `/`.
- http (`net_http.h`): url 256, headers 4 (key 32/value 64), body cap 32768 (PSRAM, lazy), default limit 16384, lines 1024, timeout 10 s, 3 redirects, worker stack 6144, TLS via cert bundle, chunked handled through `esp_http_client_perform` + `ON_DATA`.
- serve (`net_serve.h`): routes 8, path 64, query 256, response body 4096, `RequestSlot.uri[128]`, static_dir 64, handoff timeout 5000 ms, `max_uri_handlers=2` (one GET `/*`, one POST `/*`), static chunk 1024, upload chunk 1024, static prefix must be `/`, default index.html marker `<!--v5-->`.
- images (`app_images.h`): `kImageMaxBytes=280 KiB`, count 64, name 32, dir `/sdcard/images`; `kImagesIndex` declared but unused (catalog = directory scan of `*.g4`).
- wifi: scan max 16, join timeout 15 s, 2 retries.
- settings store (`s3paper_storage/src/storage.cpp:133-141`): 16 records of `{char key[16]; int32_t value}` → keys ≤15 chars, int32 values only; overflow overwrites record 0 (`:750-753`); atomic tmp/bak/primary to `/sdcard/.s3paper/settings.bin`; flushed by `StorageFlushIfDue`.
- console: reply queue 4, default timeout 500 ms; `js` command 15 s.

## JS-visible native functions (summary)

Engine globals (`app_js.cpp:549-598`): `print`, `gc`, `load` (**throws "load() not supported"**), `setTimeout`/`clearTimeout` (**throw**), `millis`, `Date.now`, `performance.now`, `abiVersion`, `resetTree` (`:603-635`: `Arena().Reset()`, bump page generations, `g_current_page=-1`, dyn=0, `g_next_cb=1`, clear home/sleep/module cbs, `ServeRoutesClear()`, fresh `__cbs=[null]`). **`eval(str)` is live** (`main/js_stdlib.h:4323` → mquickjs `js_global_eval`, `mquickjs.c:15330-15341`: `JS_Parse2(JS_EVAL_RETVAL)` + `JS_Run`), and so is `new Function(...)` (`mquickjs.c:12967-12994`).

Widget factories (`js_widgets.cpp`): `text(value|fn)` (dyn cb table 48), `row`, `col`, `spacer(fixed,flex)`, `divider(t,gray)`, `progressBar(permille,h)`, `list`, `region(id,interval,quiet)`, `canvas`, `page(name)` (12 slots retained by name). Widget proto: `pad gap mainAlign crossAlign width height flex font size gray center align invert dep hit add set progress onTap every quiet line disc ring box paint wipe`. Page proto: `header content footer overlay on(gesture,fn) every(ms) show(full) update()`. `paper`: `home(fn) sleepImage(fn) refreshTurns(n) version()`.

`files`: `exists list read write append remove name size isDir line lineCount` (completion `fn(kind, value, err)`, kinds 10..14). `http`: `get header limit done send abort status length body bodyLine bodyLineCount` (completion kind 3). `serve`: `get(path).handle(fn)`, `text json status query files(prefix,dir) start(port) stop url` — GET only, no POST route registration from JS. `images`: `count name display remove received(fn)`. `battery`: `level mv charging statusText`. `mdns`: `status host url`. `wifi`: `status ip ssidCurrent rssiCurrent scan join joinSaved save forget savedCount savedSsid off count ssid rssi secure`. `buzzer`: `tone beep stop melody`. Books/library/store: `bookOpen bookTitle bookLineCount bookLine bookNext bookPrev bookProgress libraryCount libraryLine libraryRescan storeGet storeSet appendPostcard batteryLevel`.

## Console commands (`app_console.cpp:609-654`)

`status heap events display ping touch sd sleep home serve http net buzz bat images js`. `js [status|probe N|pulp|tap X Y|swipe K|hits]` (`:311`; arg encoding in `app_owner.cpp:234-257`: 0=JsInit, 10=JsRunPulp, 11=tap, 12=swipe, 13=hits, 20..44=probe N). **No `eval` console command.** `home` shows the *native* home page, not the JS launcher.

Probes (`js_probes.cpp:463-512`): 22 embedded JS strings evaluated by `EvalBounded(code, 3000, "<probeN>")`; 13 = fault battery (deadline kill + throw storm), 14 = trace equivalence, 15 = files, 16 = http chain, 17 = serve routes, 18 = busy/limits, 19 = battery, 20 = mdns, 21/22 = images.

## Partition table & memory

`partitions.csv`: nvs 0x4000, otadata 0x2000, phy 0x1000, `factory app 4M` (single slot, no OTA), `storage spiffs 512K` (declared, never mounted). Flash 16 MB; image 1,835,600 B. PSRAM 8 MB octal (`SPIRAM_USE_MALLOC`, `MALLOC_ALWAYSINTERNAL=16384`, `RESERVE_INTERNAL=32768`). PSRAM consumers: JS arena 192 K, frame arena 320 K, http body 32 K, files body 16 K, image display ~253 K, probe traces 24 K, EPD framebuffers. Internal-RAM consumers: bytecode image ~45 K, task stacks.

## Runtime code loading today

- `jsi::EvalBounded(code, timeout_ms, name)` (`app_js.cpp:163-175`) is the only `JS_Eval` call site; used for the kernel and the probes; declared in `app_js_internal.h:60` (not exported).
- `LoadBytecodeApps()` (`app_js.cpp:109-129`) runs before the kernel eval because `JS_LoadBytecode` requires `unique_strings_len == 0` (`mquickjs.c:12949`) and `N_ROM_ATOM_TABLES_MAX = 2` (`mquickjs.c:182`): stdlib + one image, both consumed at boot.
- No per-callback ownership: `g_next_cb++` monotonic, reclaimed only by `resetTree()`.
- Downloaded bytes can only land on the SD card (`files.write` ≤16 KiB per call, or the hard-wired `POST /images/upload`).
- Incidental: `ImagesCatalogInvalidate()` is called from the httpd task (`net_serve.cpp:189`) despite the "owner-side" comment; `kImagesIndex` is dead.
