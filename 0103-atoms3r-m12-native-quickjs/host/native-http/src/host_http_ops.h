#pragma once

#include <string>
#include <vector>

#include "http_namespace_core.h"

namespace qjs_http_host {

struct HostState {
    bool running = false;
    bool fake_async_fetch = false;
    uint16_t port = 0;
    std::vector<qjs_http::StaticMountInfo> static_mounts;
};

qjs_http::HostOps make_host_ops(HostState *state);

}  // namespace qjs_http_host
