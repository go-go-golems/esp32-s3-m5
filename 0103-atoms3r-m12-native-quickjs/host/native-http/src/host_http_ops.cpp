#include "host_http_ops.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace qjs_http_host {
namespace {

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

int host_start(void *user, uint16_t port)
{
    auto *st = static_cast<HostState *>(user);
    if (!st) return -1;
    if (st->running && st->port != port) return -2;
    st->running = true;
    st->port = port;
    return 0;
}

int host_stop(void *user)
{
    auto *st = static_cast<HostState *>(user);
    if (!st) return -1;
    st->running = false;
    st->port = 0;
    return 0;
}

int host_static(void *user, const char *prefix, const char *root)
{
    auto *st = static_cast<HostState *>(user);
    if (!st || !prefix || !root) return -1;
    for (auto &m : st->static_mounts) {
        if (m.url_prefix == prefix) {
            m.virtual_root = root;
            return 0;
        }
    }
    st->static_mounts.push_back(qjs_http::StaticMountInfo{prefix, root});
    return 0;
}

int host_clear_static(void *user)
{
    auto *st = static_cast<HostState *>(user);
    if (!st) return -1;
    st->static_mounts.clear();
    return 0;
}

int host_status(void *user, qjs_http::HostStatus *out)
{
    auto *st = static_cast<HostState *>(user);
    if (!st || !out) return -1;
    out->running = st->running;
    out->port = st->port;
    return 0;
}

struct ParsedUrl {
    std::string host;
    std::string port = "80";
    std::string path = "/";
};

bool parse_http_url(const std::string &url, ParsedUrl *out, std::string *error)
{
    const std::string prefix = "http://";
    if (url.rfind(prefix, 0) != 0) {
        if (error) *error = "only http:// URLs are supported";
        return false;
    }
    std::string rest = url.substr(prefix.size());
    size_t slash = rest.find('/');
    std::string authority = slash == std::string::npos ? rest : rest.substr(0, slash);
    out->path = slash == std::string::npos ? "/" : rest.substr(slash);
    if (authority.empty()) {
        if (error) *error = "missing host";
        return false;
    }
    size_t colon = authority.rfind(':');
    if (colon != std::string::npos) {
        out->host = authority.substr(0, colon);
        out->port = authority.substr(colon + 1);
    } else {
        out->host = authority;
    }
    if (out->host.empty() || out->port.empty()) {
        if (error) *error = "invalid authority";
        return false;
    }
    return true;
}

bool send_all(int fd, const std::string &data)
{
    size_t off = 0;
    while (off < data.size()) {
        ssize_t n = send(fd, data.data() + off, data.size() - off, 0);
        if (n <= 0) return false;
        off += (size_t)n;
    }
    return true;
}

bool split_headers_body(const std::string &raw, std::string *head, std::string *body)
{
    size_t p = raw.find("\r\n\r\n");
    size_t sep = 4;
    if (p == std::string::npos) {
        p = raw.find("\n\n");
        sep = 2;
    }
    if (p == std::string::npos) return false;
    *head = raw.substr(0, p);
    *body = raw.substr(p + sep);
    return true;
}

int host_fetch(void *, const qjs_http::FetchRequest *req, qjs_http::FetchResult *out, std::string *error)
{
    if (!req || !out) return -1;
    ParsedUrl url;
    if (!parse_http_url(req->url, &url, error)) return -2;

    addrinfo hints = {};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    addrinfo *res = nullptr;
    int gai = getaddrinfo(url.host.c_str(), url.port.c_str(), &hints, &res);
    if (gai != 0) {
        if (error) *error = gai_strerror(gai);
        return -3;
    }

    int fd = -1;
    for (addrinfo *it = res; it; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;
        timeval tv = {};
        tv.tv_sec = req->timeout_ms / 1000;
        tv.tv_usec = (req->timeout_ms % 1000) * 1000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) {
        if (error) *error = std::strerror(errno);
        return -4;
    }

    std::ostringstream request;
    request << req->method << " " << url.path << " HTTP/1.0\r\n";
    request << "Host: " << url.host << "\r\n";
    request << "Connection: close\r\n";
    for (const auto &h : req->headers) request << h.name << ": " << h.value << "\r\n";
    if (!req->body.empty()) {
        request << "Content-Length: " << req->body.size() << "\r\n";
    }
    request << "\r\n" << req->body;

    std::string request_text = request.str();
    if (!send_all(fd, request_text)) {
        if (error) *error = "send failed";
        close(fd);
        return -5;
    }

    std::string raw;
    char buf[1024];
    while (raw.size() <= req->max_response_bytes + 4096) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n == 0) break;
        if (n < 0) {
            if (error) *error = std::strerror(errno);
            close(fd);
            return -6;
        }
        raw.append(buf, (size_t)n);
    }
    close(fd);

    std::string head, body;
    if (!split_headers_body(raw, &head, &body)) {
        if (error) *error = "malformed HTTP response";
        return -7;
    }
    if (body.size() > req->max_response_bytes) {
        if (error) *error = "response body too large";
        return -8;
    }

    std::istringstream hs(head);
    std::string status_line;
    std::getline(hs, status_line);
    if (!status_line.empty() && status_line.back() == '\r') status_line.pop_back();
    std::istringstream ss(status_line);
    std::string httpver;
    ss >> httpver >> out->status;
    std::getline(ss, out->status_text);
    while (!out->status_text.empty() && out->status_text.front() == ' ') out->status_text.erase(out->status_text.begin());
    out->final_url = req->url;
    out->body = body;

    std::string line;
    while (std::getline(hs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string name = lower(line.substr(0, colon));
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());
        out->headers.push_back(qjs_http::Header{name, value});
    }
    return 0;
}

int host_fetch_async(void *user,
                     JSContext *ctx,
                     const qjs_http::FetchRequest *req,
                     JSValueConst resolve,
                     JSValueConst reject,
                     std::string *error)
{
    qjs_http::FetchResult result;
    std::string fetch_error;
    int rc = host_fetch(user, req, &result, &fetch_error);
    std::string settle_error;
    if (rc == 0) {
        rc = qjs_http::resolve_fetch_promise(ctx, resolve, *req, result, &settle_error);
    } else {
        rc = qjs_http::reject_fetch_promise(ctx, reject, fetch_error.empty() ? "fetch failed" : fetch_error.c_str(), &settle_error);
    }
    if (rc != 0 && error) *error = settle_error.empty() ? "fake async fetch settlement failed" : settle_error;
    return rc;
}

}  // namespace

qjs_http::HostOps make_host_ops(HostState *state)
{
    qjs_http::HostOps ops = {};
    ops.user = state;
    ops.start = host_start;
    ops.stop = host_stop;
    ops.add_static_mount = host_static;
    ops.clear_static_mounts = host_clear_static;
    ops.status = host_status;
    ops.fetch = host_fetch;
    if (state && state->fake_async_fetch) ops.fetch_async = host_fetch_async;
    return ops;
}

}  // namespace qjs_http_host
