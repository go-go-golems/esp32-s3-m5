#pragma once

#include <stdint.h>

#include <string>
#include <vector>

extern "C" {
#include "quickjs.h"
}

namespace qjs_http {

struct HostStatus {
    bool running = false;
    uint16_t port = 0;
};

struct Header {
    std::string name;
    std::string value;
};

struct FetchRequest {
    std::string url;
    std::string method = "GET";
    std::vector<Header> headers;
    std::string body;
    uint32_t timeout_ms = 1000;
    size_t max_response_bytes = 16 * 1024;
};

struct FetchResult {
    int status = 0;
    std::string status_text;
    std::string final_url;
    std::vector<Header> headers;
    std::string body;
};

struct HostOps {
    void *user = nullptr;
    int (*start)(void *user, uint16_t port) = nullptr;
    int (*stop)(void *user) = nullptr;
    int (*add_static_mount)(void *user, const char *url_prefix, const char *virtual_root) = nullptr;
    int (*clear_static_mounts)(void *user) = nullptr;
    int (*status)(void *user, HostStatus *out) = nullptr;
    int (*fetch)(void *user, const FetchRequest *req, FetchResult *out, std::string *error) = nullptr;
};

struct HttpResponse {
    int status = 200;
    std::string content_type = "text/plain; charset=utf-8";
    std::string body;
    bool body_set = false;
};

struct StaticMountInfo {
    std::string url_prefix;
    std::string virtual_root;
};

struct DynamicRouteInfo {
    std::string method;
    std::string path;
};

class Runtime {
  public:
    Runtime(JSContext *ctx, const HostOps &ops);
    ~Runtime();

    Runtime(const Runtime &) = delete;
    Runtime &operator=(const Runtime &) = delete;

    JSContext *ctx() const { return ctx_; }
    const HostOps &ops() const { return ops_; }

    int install_global();
    void clear_routes();

    int start(uint16_t port);
    int stop();
    int add_static_mount(const char *url_prefix, const char *virtual_root);
    int clear_static_mounts();
    HostStatus status() const;

    int add_get_route(const char *path, JSValueConst handler);
    bool dispatch_get(const char *path, HttpResponse *out, std::string *error);

    std::vector<StaticMountInfo> static_mounts() const { return static_mounts_; }
    std::vector<DynamicRouteInfo> dynamic_routes() const;

  private:
    struct Route {
        std::string method;
        std::string path;
        JSValue handler = JS_UNDEFINED;
    };

    Route *find_route(const char *method, const char *path);

    JSContext *ctx_ = nullptr;
    HostOps ops_ = {};
    bool running_ = false;
    uint16_t port_ = 0;
    std::vector<StaticMountInfo> static_mounts_;
    std::vector<Route> routes_;
};

Runtime *active_runtime();
void set_active_runtime(Runtime *runtime);

}  // namespace qjs_http
