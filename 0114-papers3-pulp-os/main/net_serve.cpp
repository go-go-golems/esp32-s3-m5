#include "net_serve.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "app_images.h"
#include "app_owner.h"
#include "net_mdns.h"
#include "net_wifi.h"

namespace pulp {
namespace {

const char *kTag = "serve";

constexpr TickType_t kHandoffTimeout = pdMS_TO_TICKS(5000);

struct RouteEntry {
    bool in_use = false;
    char path[kServeMaxPath] = {};
    uint8_t method = 0;  // 0 = GET (default), 1 = POST  [ESP-54]
    int32_t cb_id = 0;
};

// The single request/response slot. `busy` is the httpd-side claim;
// `gen` invalidates late owner writes after an httpd-side timeout.
struct RequestSlot {
    std::atomic<bool> busy{false};
    std::atomic<int32_t> gen{0};
    char uri[128];
    char query[kServeMaxQuery];
    // Response (owner writes during dispatch, httpd reads after the give).
    bool resp_filled;
    int32_t resp_status;
    uint8_t resp_type;  // 0 text, 1 json, 2 html
    char resp_body[kServeMaxBody];
    uint32_t resp_len;
};

struct ServeState {
    httpd_handle_t server = nullptr;
    uint16_t port = 0;
    RouteEntry routes[kServeMaxRoutes];
    char static_dir[64] = {};
    bool static_mounted = false;
    RequestSlot slot;
    SemaphoreHandle_t resp_sem = nullptr;
    // Owner-side dispatch context (which generation may respond).
    int32_t dispatch_gen = -1;
    // Counters (owner + httpd; approximate is fine for diagnostics).
    uint32_t requests = 0;
    uint32_t busy_503 = 0;
    uint32_t timeout_503 = 0;
};

ServeState s_state;

const char *ContentTypeName(uint8_t type) {
    switch (type) {
        case 1: return "application/json";
        case 2: return "text/html";
    }
    return "text/plain";
}

const char *MimeFor(const char *path) {
    const char *dot = strrchr(path, '.');
    if (dot == nullptr) {
        return "application/octet-stream";
    }
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
        return "text/html";
    }
    if (strcmp(dot, ".txt") == 0) return "text/plain";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) {
        return "image/jpeg";
    }
    return "application/octet-stream";
}

// HTTPD TASK. Streams a POST body to /sdcard/images/<ts>.g4, validates
// the .g4 header on the first chunk, caps the size, appends the catalog
// index, stages the result mailbox, and posts ModuleDone{Images}. This is
// the one sanctioned off-owner SD WRITE (to /sdcard/images/ only, never
// state files), mirroring ESP-53's off-owner static-file READ.
//
// The upload slot is single: a concurrent POST gets 503 (a PaperS3 is not
// a web farm).
static std::atomic<bool> s_upload_busy{false};

static esp_err_t SendStatus(httpd_req_t *req, int code, const char *why) {
    char status_line[16];
    snprintf(status_line, sizeof(status_line), "%d", code);
    httpd_resp_set_status(req, status_line);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, why, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// IDF's httpd_resp_send_err has no 503 variant; hand-rolled (ESP-53 legacy).
static esp_err_t Send503(httpd_req_t *req, const char *why) {
    return SendStatus(req, 503, why);
}

static esp_err_t ServeUpload(httpd_req_t *req) {
    // Reject oversized uploads before touching the card.
    const size_t total = req->content_len;
    if (total > kImageMaxBytes) {
        return SendStatus(req, 413, "too large");
    }
    bool expected = false;
    if (!s_upload_busy.compare_exchange_strong(expected, true)) {
        s_state.busy_503++;
        return SendStatus(req, 503, "busy");
    }
    // Generate a timestamped name and open the file for streaming write.
    ImagesUploadResult res{};
    const int64_t now_ms = esp_timer_get_time() / 1000;
    snprintf(res.name, sizeof(res.name), "%lld.g4",
             static_cast<long long>(now_ms));
    char path[64];
    snprintf(path, sizeof(path), "%s/%s", kImagesDir, res.name);
    mkdir(kImagesDir, 0775);
    FILE *f = fopen(path, "wb");
    if (f == nullptr) {
        s_upload_busy.store(false);
        res.err = 1;
        ImagesSetUploadResult(res);
        (void)PostModuleDone(ModuleId::Images, kDoneImagesUpload, 0, 1);
        return SendStatus(req, 503, "no card");
    }
    static char chunk[1024];
    size_t received = 0;
    bool header_ok = false;
    while (received < total) {
        const size_t want = total - received < sizeof(chunk)
                                ? total - received
                                : sizeof(chunk);
        const int n = httpd_req_recv(req, chunk, want);
        if (n <= 0) {
            if (n == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            break;  // connection dropped
        }
        if (!header_ok) {
            // The first chunk must contain the full 12-byte .g4 header.
            if (static_cast<size_t>(n) < sizeof(G4Header) ||
                !G4ValidateHeader(
                    reinterpret_cast<const G4Header *>(chunk))) {
                fclose(f);
                (void)unlink(path);
                s_upload_busy.store(false);
                ESP_LOGW(kTag, "upload: bad .g4 header");
                return SendStatus(req, 400, "bad frame");
            }
            header_ok = true;
        }
        fwrite(chunk, 1, static_cast<size_t>(n), f);
        received += static_cast<size_t>(n);
    }
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    s_upload_busy.store(false);
    if (received != total) {
        (void)unlink(path);
        res.err = 2;
        ImagesSetUploadResult(res);
        (void)PostModuleDone(ModuleId::Images, kDoneImagesUpload, 0, 2);
        return SendStatus(req, 499, "short");
    }
    res.bytes = static_cast<uint32_t>(received);
    res.err = 0;
    ImagesSetUploadResult(res);
    ImagesCatalogInvalidate();
    (void)PostModuleDone(ModuleId::Images, kDoneImagesUpload,
                         static_cast<int32_t>(res.bytes), 0);
    ESP_LOGI(kTag, "upload %s: %u bytes", res.name,
             static_cast<unsigned>(res.bytes));
    return SendStatus(req, 200, res.name);
}

int FindRoute(const char *uri) {
    for (uint32_t i = 0; i < kServeMaxRoutes; ++i) {
        if (s_state.routes[i].in_use &&
            strcmp(s_state.routes[i].path, uri) == 0) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// HTTPD TASK: streams a file from the static mount. The one sanctioned
// off-owner storage access (plain VFS reads).
esp_err_t ServeStatic(httpd_req_t *req, const char *uri) {
    if (strstr(uri, "..") != nullptr) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "no");
        return ESP_OK;
    }
    char path[160];
    snprintf(path, sizeof(path), "%s%s%s", s_state.static_dir, uri,
             uri[strlen(uri) - 1] == '/' ? "index.html" : "");
    struct stat st;
    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }
    FILE *f = fopen(path, "rb");
    if (f == nullptr) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
        return ESP_OK;
    }
    httpd_resp_set_type(req, MimeFor(path));
    static char chunk[1024];  // httpd is single-worker: one buffer is safe
    size_t got;
    while ((got = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (httpd_resp_send_chunk(req, chunk, got) != ESP_OK) {
            break;
        }
    }
    fclose(f);
    httpd_resp_send_chunk(req, nullptr, 0);
    return ESP_OK;
}

// HTTPD TASK: the JS-route handoff.
esp_err_t ServeJsRoute(httpd_req_t *req, int route_index) {
    RequestSlot &slot = s_state.slot;
    bool expected = false;
    if (!slot.busy.compare_exchange_strong(expected, true)) {
        s_state.busy_503++;
        return Send503(req, "busy");
    }
    snprintf(slot.uri, sizeof(slot.uri), "%.127s", req->uri);
    slot.query[0] = '\0';
    (void)httpd_req_get_url_query_str(req, slot.query,
                                      sizeof(slot.query));
    slot.resp_filled = false;
    slot.resp_status = 500;
    slot.resp_type = 0;
    slot.resp_len = 0;
    const int32_t gen = slot.gen.fetch_add(1) + 1;
    // Drain a stale give (an earlier owner response that lost its race
    // against our timeout) so the take below waits for THIS request.
    (void)xSemaphoreTake(s_state.resp_sem, 0);
    if (PostModuleDone(ModuleId::Serve, kDoneServeRequest, route_index,
                       gen) != StatusCode::Ok) {
        slot.busy.store(false);
        return Send503(req, "queue");
    }
    if (xSemaphoreTake(s_state.resp_sem, kHandoffTimeout) != pdTRUE) {
        // Owner wedged: invalidate the generation so a late ServeRespond
        // is dropped, then fail this request.
        slot.gen.fetch_add(1);
        s_state.timeout_503++;
        slot.busy.store(false);
        return Send503(req, "owner timeout");
    }
    if (!slot.resp_filled) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                            "route returned nothing");
        slot.busy.store(false);
        return ESP_OK;
    }
    char status_line[16];
    snprintf(status_line, sizeof(status_line), "%d",
             static_cast<int>(slot.resp_status));
    httpd_resp_set_status(req, status_line);
    httpd_resp_set_type(req, ContentTypeName(slot.resp_type));
    httpd_resp_send(req, slot.resp_body, slot.resp_len);
    slot.busy.store(false);
    return ESP_OK;
}

// ---- ESP-55 P5: POST /apps/upload?name=<id> ----------------------------
// Streams an app module to /sdcard/apps/<id>.js (.part + rename). The
// THIRD sanctioned off-owner SD write: plain files under /sdcard/apps
// only, never state files. Completion: ModuleDone{Apps, kDoneAppsUpload,
// bytes, err} -> the OS core's apps.received callback rescans.
constexpr uint32_t kAppUploadMax = 32 * 1024;
constexpr const char *kAppsDir = "/sdcard/apps";
static char s_apps_upload_name[32] = "";

static bool AppNameOk(const char *name) {
    const size_t n = strlen(name);
    if (n == 0 || n > 24) {
        return false;
    }
    for (size_t i = 0; i < n; i++) {
        const char c = name[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                        c == '_' || c == '-';
        if (!ok) {
            return false;
        }
    }
    return true;
}

static esp_err_t ServeAppsUpload(httpd_req_t *req) {
    if (req->content_len == 0 || req->content_len > kAppUploadMax) {
        return SendStatus(req, 413, "too large");
    }
    char query[224] = "";
    char name[32] = "";
    (void)httpd_req_get_url_query_str(req, query, sizeof(query));
    if (httpd_query_key_value(query, "name", name, sizeof(name)) != ESP_OK ||
        !AppNameOk(name)) {
        return SendStatus(req, 400, "bad name");
    }
    bool expected = false;
    if (!s_upload_busy.compare_exchange_strong(expected, true)) {
        s_state.busy_503++;
        return SendStatus(req, 503, "busy");
    }
    mkdir(kAppsDir, 0775);
    char part[80];
    char path[80];
    snprintf(part, sizeof(part), "%s/upload.part", kAppsDir);
    snprintf(path, sizeof(path), "%s/%s.js", kAppsDir, name);
    FILE *f = fopen(part, "wb");
    if (f == nullptr) {
        s_upload_busy.store(false);
        (void)PostModuleDone(ModuleId::Apps, kDoneAppsUpload, 0, 1);
        return SendStatus(req, 503, "no card");
    }
    static char achunk[1024];
    const size_t total = req->content_len;
    size_t received = 0;
    while (received < total) {
        const size_t want = total - received < sizeof(achunk)
                                ? total - received
                                : sizeof(achunk);
        const int n = httpd_req_recv(req, achunk, want);
        if (n <= 0) {
            if (n == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            break;  // connection dropped
        }
        fwrite(achunk, 1, static_cast<size_t>(n), f);
        received += static_cast<size_t>(n);
    }
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (received != total) {
        (void)unlink(part);
        s_upload_busy.store(false);
        (void)PostModuleDone(ModuleId::Apps, kDoneAppsUpload, 0, 2);
        return SendStatus(req, 499, "short");
    }
    (void)unlink(path);  // FATFS rename does not overwrite
    if (rename(part, path) != 0) {
        (void)unlink(part);
        s_upload_busy.store(false);
        (void)PostModuleDone(ModuleId::Apps, kDoneAppsUpload, 0, 1);
        return SendStatus(req, 503, "rename failed");
    }
    // Manifest when none exists; never clobbers an operator-authored one.
    // ESP-57: optional query fields fill it (title/subtitle sanitized to
    // [A-Za-z0-9 _.&-]; '+' decodes to space; hidden=1 marks suite apps
    // that are launchable by id but get no launcher row).
    char mpath[80];
    snprintf(mpath, sizeof(mpath), "%s/%s.json", kAppsDir, name);
    struct stat st;
    char title[48] = "";
    (void)httpd_query_key_value(query, "title", title, sizeof(title));
    // ESP-57: metadata in the query REWRITES the manifest (a repeated
    // push with fields is an intentional update); a bare push still never
    // clobbers an existing manifest.
    if (stat(mpath, &st) != 0 || title[0] != '\0') {
        char subtitle[64] = "";
        char hidden[8] = "";
        (void)httpd_query_key_value(query, "subtitle", subtitle,
                                    sizeof(subtitle));
        (void)httpd_query_key_value(query, "hidden", hidden,
                                    sizeof(hidden));
        auto sanitize = [](char *sv) {
            for (size_t i = 0; sv[i] != '\0'; ++i) {
                const char c = sv[i];
                if (c == '+') {
                    sv[i] = ' ';
                    continue;
                }
                const bool ok = (c >= 'A' && c <= 'Z') ||
                                (c >= 'a' && c <= 'z') ||
                                (c >= '0' && c <= '9') || c == ' ' ||
                                c == '_' || c == '.' || c == '&' ||
                                c == '-';
                if (!ok) {
                    sv[i] = ' ';
                }
            }
        };
        sanitize(title);
        sanitize(subtitle);
        FILE *m = fopen(mpath, "wb");
        if (m != nullptr) {
            fprintf(m,
                    "{\"id\":\"%s\",\"title\":\"%s\",\"subtitle\":"
                    "\"%s\",\"version\":1,\"abi\":2,"
                    "\"src\":\"/apps/%s.js\"%s}\n",
                    name, title[0] != '\0' ? title : name,
                    subtitle[0] != '\0' ? subtitle
                                         : "installed over http",
                    name,
                    hidden[0] == '1' ? ",\"hidden\":true" : "");
            fflush(m);
            fsync(fileno(m));
            fclose(m);
        }
    }
    snprintf(s_apps_upload_name, sizeof(s_apps_upload_name), "%s", name);
    s_upload_busy.store(false);
    (void)PostModuleDone(ModuleId::Apps, kDoneAppsUpload,
                         static_cast<int32_t>(received), 0);
    ESP_LOGI(kTag, "apps upload %s: %u bytes", name,
             static_cast<unsigned>(received));
    return SendStatus(req, 200, name);
}

esp_err_t Handler(httpd_req_t *req) {
    // HTTPD TASK entry point for every request.
    s_state.requests++;
    char uri[128];
    snprintf(uri, sizeof(uri), "%.127s", req->uri);
    char *qmark = strchr(uri, '?');
    if (qmark != nullptr) {
        *qmark = '\0';
    }
    // ESP-54: POST /images/upload streams the body to the SD card.
    // ESP-55: POST or PUT (curl -T) /apps/upload streams an app module.
    if (req->method == HTTP_POST || req->method == HTTP_PUT) {
        if (req->method == HTTP_POST && strcmp(uri, "/images/upload") == 0) {
            return ServeUpload(req);
        }
        if (strcmp(uri, "/apps/upload") == 0) {
            return ServeAppsUpload(req);
        }
        return SendStatus(req, 404, "not found");
    }
    const int route = FindRoute(uri);
    if (route >= 0) {
        return ServeJsRoute(req, route);
    }
    if (s_state.static_mounted) {
        return ServeStatic(req, uri);
    }
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
    return ESP_OK;
}

// ESP-54: the default upload page. Self-contained HTML+JS that lets the
// operator pick a PNG/JPG, crop/scale it to 540x960 in the browser,
// quantize to 4-bit grayscale (+ optional Floyd-Steinberg dither), pack a
// .g4 frame, and POST it to /images/upload. No device-side decode.
const char kDefaultIndex[] =
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>PULP Gallery</title><!--v5-->"
    "<style>body{font-family:monospace;margin:16px;"
    "background:#222;color:#ddd}"
    "h1{margin:0 0 8px}button,select{font:inherit;padding:6px 10px;"
    "margin:4px 4px 4px 0;background:#444;color:#eee;border:1px solid #666}"
    "canvas{background:#fff;image-rendering:pixelated;"
    "border:1px solid #555;max-width:100%}"
    ".row{margin:8px 0}#out{color:#9cf;word-break:break-all}</style>"
    "</head><body><h1>PULP Gallery</h1>"
    "<div class='row'><input type='file' id='f' accept='image/png,image/jpeg'>"
    "</div><div class='row'>"
    "<button id='fit'>Fit</button><button id='fill'>Fill</button>"
    "<label><input type='checkbox' id='rot'> rotate</label>"
    "<label><input type='checkbox' id='dither'> dither</label>"
    "<button id='up' disabled>Upload</button></div>"
    "<div class='row'><canvas id='c' width='540' height='960'></canvas></div>"
    "<div class='row'><button id='list'>Refresh list</button>"
    "<span id='count'></span></div><div id='out'></div>"
    "<script>"
    "var W=540,H=960;var img=null;var cv=document.getElementById('c');"
    "var ctx=cv.getContext('2d');var fit=true;var rot=false;var out=document.getElementById('out');"
    "function log(s){out.innerHTML=s+'<br>'+out.innerHTML;}"
    "document.getElementById('fit').onclick=function(){fit=true;draw();};"
    "document.getElementById('fill').onclick=function(){fit=false;draw();};"
    "document.getElementById('rot').onchange=function(){rot=document.getElementById('rot').checked;draw();};"
    "document.getElementById('f').onchange=function(e){"
    "  var fr=new FileReader();"
    "  fr.onload=function(){"
    "    img=new Image();"
    "    img.onload=function(){"
    "      rot=(img.width>img.height);"
    "      document.getElementById('rot').checked=rot;"
    "      if(rot){fit=false;document.getElementById('fill').focus();}"
    "      draw();document.getElementById('up').disabled=false;};"
    "    img.src=fr.result;};"
    "  fr.readAsDataURL(e.target.files[0]);};"
    "function draw(){"
    "  if(!img)return;"
    "  ctx.fillStyle='#fff';ctx.fillRect(0,0,W,H);"
    "  if(rot){"
    "    var s=fit?Math.min(W/img.height,H/img.width)"
    "            :Math.max(W/img.height,H/img.width);"
    "    var w=img.width*s,h=img.height*s;"
    "    ctx.save();ctx.translate(W/2,H/2);ctx.rotate(Math.PI/2);"
    "    ctx.drawImage(img,-w/2,-h/2,w,h);ctx.restore();"
    "    log('landscape -> rotated (turn tablet sideways) '+(fit?'fit':'fill'));"
    "    return;}"
    "  var s=fit?Math.min(W/img.width,H/img.height)"
    "          :Math.max(W/img.width,H/img.height);"
    "  var w=img.width*s,h=img.height*s;"
    "  ctx.drawImage(img,(W-w)/2,(H-h)/2,w,h);}"
    "function buildG4(dither){"
    "  var d=ctx.getImageData(0,0,W,H).data;"
    "  var rowB=(W+1)>>1;var px=new Uint8Array(rowB*H);"
    "  var err=new Float32Array(W);var nextErr=new Float32Array(W);"
    "  for(var y=0;y<H;y++){"
    "    var t=nextErr;nextErr=err;err=t;for(var x=0;x<W;x++)nextErr[x]=0;"
    "    for(var x=0;x<W;x++){"
    "      var i=(y*W+x)*4;"
    "      var y_=(d[i]*299+d[i+1]*587+d[i+2]*114)/1000;"
    "      if(dither)y_+=err[x];"
    "      var g=Math.max(0,Math.min(15,Math.round(y_/17)));"
    "      if(dither){var q=g*17;var e=y_-q;"
    "        if(x+1<W)nextErr[x+1]+=e*7/16;"
    "        if(x>0)nextErr[x-1]+=e*3/16;"
    "        nextErr[x]+=e*5/16;"
    "        if(x+1<W)err[x+1]+=e*1/16;}"
    "      var off=y*rowB+(x>>1);"
    "      if((x&1)===0)px[off]=g<<4;else px[off]|=g;}}"
    "  var hdr=new Uint8Array([0x47,0x34,0x49,0x4d,W&0xff,(W>>8)&0xff,"
    "    H&0xff,(H>>8)&0xff,4,1,0,0]);"
    "  var r=new Uint8Array(hdr.length+px.length);"
    "  r.set(hdr,0);r.set(px,hdr.length);return r;}"
    "document.getElementById('up').onclick=function(){"
    "  if(!img)return;"
    "  var d=document.getElementById('dither').checked;"
    "  var body=buildG4(d);"
    "  log('uploading '+body.length+' bytes...');"
    "  fetch('/images/upload',{method:'POST',body:body})"
    "    .then(function(r){return r.text();})"
    "    .then(function(t){log('saved: '+t);loadList();})"
    "    .catch(function(e){log('err: '+e);});};"
    "function loadList(){"
    "  fetch('/images/list').then(function(r){return r.json();})"
    "    .then(function(j){"
    "      document.getElementById('count').textContent="
    "        j.count+' image(s)';"
    "      var s=j.count+' image(s): '+j.images.join(', ');"
    "      log(s);})"
    "    .catch(function(e){log('list err: '+e);});}"
    "document.getElementById('list').onclick=loadList;"
    "loadList();"
    "</script></body></html>\n";

}  // namespace

StatusCode ServeRouteAdd(const char *path, int32_t cb_id) {
    AssertOwner();
    if (path == nullptr || path[0] != '/' ||
        strlen(path) >= kServeMaxPath || cb_id <= 0) {
        return StatusCode::InvalidArgument;
    }
    const int existing = FindRoute(path);
    if (existing >= 0) {
        s_state.routes[existing].cb_id = cb_id;  // re-register
        return StatusCode::Ok;
    }
    for (uint32_t i = 0; i < kServeMaxRoutes; ++i) {
        if (!s_state.routes[i].in_use) {
            snprintf(s_state.routes[i].path, sizeof(s_state.routes[i].path),
                     "%s", path);
            s_state.routes[i].cb_id = cb_id;
            s_state.routes[i].in_use = true;
            return StatusCode::Ok;
        }
    }
    return StatusCode::CapacityExceeded;
}

void ServeRoutesClear() {
    for (uint32_t i = 0; i < kServeMaxRoutes; ++i) {
        s_state.routes[i].in_use = false;
        s_state.routes[i].cb_id = 0;
    }
}

StatusCode ServeFilesMount(const char *url_prefix, const char *dir) {
    AssertOwner();
    if (url_prefix == nullptr || strcmp(url_prefix, "/") != 0 ||
        dir == nullptr || strlen(dir) >= sizeof(s_state.static_dir)) {
        return StatusCode::InvalidArgument;
    }
    snprintf(s_state.static_dir, sizeof(s_state.static_dir), "%s", dir);
    mkdir(dir, 0775);
    char index_path[96];
    snprintf(index_path, sizeof(index_path), "%s/index.html", dir);
    struct stat st;
    bool write_default = (stat(index_path, &st) != 0);
    if (!write_default) {
        // ESP-54: ship the upload UI. Overwrite any stale default (the
        // original ESP-53 placeholder OR a previous ESP-54 page version)
        // unless the operator has customized it. The marker <title>PULP
        // Gallery</title> identifies our own page; anything else is treated
        // as a customization and preserved.
        FILE *probe = fopen(index_path, "rb");
        if (probe != nullptr) {
            char head[256] = {};
            const size_t got = fread(head, 1, sizeof(head) - 1, probe);
            fclose(probe);
            head[got < sizeof(head) - 1 ? got : sizeof(head) - 1] = '\0';
            if (got > 0 && strstr(head, "<!--v5-->") == nullptr) {
                write_default = true;
            }
        }
    }
    if (write_default) {
        FILE *f = fopen(index_path, "wb");
        if (f != nullptr) {
            fwrite(kDefaultIndex, 1, sizeof(kDefaultIndex) - 1, f);
            fclose(f);
            ESP_LOGI(kTag, "wrote default %s", index_path);
        }
    }
    s_state.static_mounted = true;
    return StatusCode::Ok;
}

StatusCode ServeStart(uint16_t port) {
    AssertOwner();
    if (s_state.server != nullptr) {
        return StatusCode::Busy;
    }
    if (s_state.resp_sem == nullptr) {
        s_state.resp_sem = xSemaphoreCreateBinary();
        if (s_state.resp_sem == nullptr) {
            return StatusCode::OutOfMemory;
        }
    }
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port = port == 0 ? 80 : port;
    // ESP-57: the manifest-writing upload path (query + field buffers +
    // FATFS fprintf) overflowed the 4 KiB default — detected by the
    // FreeRTOS stack watchdog on the first suite push.
    cfg.stack_size = 6144;
    cfg.max_uri_handlers = 3;  // ESP-54: GET+POST; ESP-55: +PUT (curl -T)
    cfg.uri_match_fn = httpd_uri_match_wildcard;
    cfg.lru_purge_enable = true;
    if (httpd_start(&s_state.server, &cfg) != ESP_OK) {
        s_state.server = nullptr;
        return StatusCode::Busy;
    }
    static const httpd_uri_t all_get = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = &Handler,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_state.server, &all_get);
    static const httpd_uri_t all_post = {
        .uri = "/*",
        .method = HTTP_POST,
        .handler = &Handler,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_state.server, &all_post);
    // ESP-55 P5: curl -T uses PUT; same dispatch as POST (uploads only).
    static const httpd_uri_t all_put = {
        .uri = "/*",
        .method = HTTP_PUT,
        .handler = &Handler,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(s_state.server, &all_put);
    s_state.port = cfg.server_port;
    ESP_LOGI(kTag, "listening on :%u",
             static_cast<unsigned>(s_state.port));
    // ESP-54: advertise pulp.local now that the listener is up. mDNS
    // defers if WiFi is not yet up; the Settings app re-arms on join.
    (void)MdnsAnnounce(s_state.port);
    return StatusCode::Ok;
}

StatusCode ServeStop() {
    AssertOwner();
    if (s_state.server == nullptr) {
        return StatusCode::Ok;
    }
    httpd_stop(s_state.server);
    s_state.server = nullptr;
    s_state.port = 0;
    // ESP-54: withdraw pulp.local when the server stops.
    (void)MdnsStop();
    ESP_LOGI(kTag, "stopped");
    return StatusCode::Ok;
}

bool ServeRunning() { return s_state.server != nullptr; }

void ServeUrl(char *out, size_t cap) {
    char ip[16];
    WifiIp(ip, sizeof(ip));
    if (!ServeRunning() || ip[0] == '\0') {
        snprintf(out, cap, "%s", "");
        return;
    }
    if (s_state.port == 80) {
        snprintf(out, cap, "http://%s", ip);
    } else {
        snprintf(out, cap, "http://%s:%u", ip,
                 static_cast<unsigned>(s_state.port));
    }
}

void ServeOwnerDispatch(int32_t route_index, int32_t gen) {
    AssertOwner();
    if (route_index < 0 ||
        route_index >= static_cast<int32_t>(kServeMaxRoutes) ||
        !s_state.routes[route_index].in_use) {
        // Route vanished (resetTree between claim and dispatch): release
        // the waiter with an unfilled slot -> it answers 500.
        if (s_state.slot.gen.load() == gen) {
            xSemaphoreGive(s_state.resp_sem);
        }
        return;
    }
    s_state.dispatch_gen = gen;
    extern void JsServeInvokeRoute(int32_t cb_id);  // js_serve.cpp
    JsServeInvokeRoute(s_state.routes[route_index].cb_id);
    s_state.dispatch_gen = -1;
    if (s_state.slot.gen.load() == gen) {
        xSemaphoreGive(s_state.resp_sem);
    }
    // else: httpd timed out and moved on; the give is skipped so the
    // semaphore cannot carry a stale count into the next request.
}

bool ServeRespond(int32_t status, uint8_t content_type, const char *body,
                  uint32_t len) {
    AssertOwner();
    RequestSlot &slot = s_state.slot;
    if (s_state.dispatch_gen < 0 ||
        slot.gen.load() != s_state.dispatch_gen) {
        return false;  // late write after httpd timeout: dropped
    }
    if (len > kServeMaxBody) {
        len = kServeMaxBody;
    }
    std::memcpy(slot.resp_body, body, len);
    slot.resp_len = len;
    slot.resp_status = status;
    slot.resp_type = content_type;
    slot.resp_filled = true;
    return true;
}

const char *ServeRequestQuery() { return s_state.slot.query; }

const char *ServeRequestUri() { return s_state.slot.uri; }

void FillServeSnapshot(ServeSnapshot *out) {
    std::memset(out, 0, sizeof(*out));
    out->running = ServeRunning() ? 1 : 0;
    out->port = s_state.port;
    uint32_t routes = 0;
    for (uint32_t i = 0; i < kServeMaxRoutes; ++i) {
        routes += s_state.routes[i].in_use ? 1 : 0;
    }
    out->routes = routes;
    out->static_mounted = s_state.static_mounted ? 1 : 0;
    out->requests = s_state.requests;
    out->busy_503 = s_state.busy_503;
    out->timeout_503 = s_state.timeout_503;
    ServeUrl(out->url, sizeof(out->url));
}

const char *ServeAppsUploadName() { return s_apps_upload_name; }

}  // namespace pulp
