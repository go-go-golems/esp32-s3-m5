#include "web_setup.h"

#include <memory>
#include <string>
#include <vector>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config_store.h"
#include "scene_model.h"

namespace {
const char *TAG = "web_setup";

const char *kPageTop =
    "<!doctype html><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>PPA Dial Setup</title>"
    "<style>body{font-family:-apple-system,sans-serif;background:#111;color:#eee;"
    "max-width:560px;margin:24px auto;padding:0 16px}input,textarea{width:100%;"
    "padding:10px;margin:6px 0 14px;border-radius:8px;border:1px solid #444;"
    "background:#1c1c1e;color:#eee;font-size:15px}textarea{height:220px;"
    "font-family:monospace}button{background:#14a038;color:#fff;border:0;"
    "padding:12px 28px;border-radius:8px;font-size:16px}"
    ".err{background:#c03020;padding:10px;border-radius:8px}</style>"
    "<h2>PPA Dial &ndash; Einrichtung</h2>";

std::string html_escape(const std::string &in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default: out += c;
        }
    }
    return out;
}

std::string url_decode(const std::string &in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); i++) {
        if (in[i] == '+') {
            out += ' ';
        } else if (in[i] == '%' && i + 2 < in.size()) {
            char hex[3] = {in[i + 1], in[i + 2], 0};
            out += static_cast<char>(strtol(hex, nullptr, 16));
            i += 2;
        } else {
            out += in[i];
        }
    }
    return out;
}

std::string form_field(const std::string &body, const char *key) {
    const std::string needle = std::string(key) + "=";
    size_t pos = 0;
    while (pos < body.size()) {
        size_t end = body.find('&', pos);
        if (end == std::string::npos) end = body.size();
        if (body.compare(pos, needle.size(), needle) == 0)
            return url_decode(body.substr(pos + needle.size(), end - pos - needle.size()));
        pos = end + 1;
    }
    return "";
}

std::string render_form(const std::string &error) {
    std::string ssid, pass, presets;
    config_store_load(ssid, pass, presets);
    std::string page = kPageTop;
    if (!error.empty()) page += "<p class='err'>" + html_escape(error) + "</p>";
    page += "<form method='POST' action='/save'>"
            "<label>WLAN-Name (SSID)</label><input name='ssid' value='" +
            html_escape(ssid) +
            "'>"
            "<label>WLAN-Passwort</label><input name='pass' type='password' value='" +
            html_escape(pass) +
            "'>"
            "<label>Szenen: kompletten Inhalt der presets.json hier einf&uuml;gen<br>"
            "<small>(Mac: ~/Library/Application Support/PPA Group Control/presets.json)</small>"
            "</label><textarea name='presets'>" +
            html_escape(presets) +
            "</textarea>"
            "<button>Speichern &amp; Neustarten</button></form>";
    return page;
}

esp_err_t handle_root(httpd_req_t *req) {
    const std::string page = render_form("");
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, page.c_str(), page.size());
}

esp_err_t handle_save(httpd_req_t *req) {
    std::string body(req->content_len, '\0');
    size_t off = 0;
    while (off < body.size()) {
        int n = httpd_req_recv(req, body.data() + off, body.size() - off);
        if (n <= 0) return ESP_FAIL;
        off += n;
    }
    const std::string ssid = form_field(body, "ssid");
    const std::string pass = form_field(body, "pass");
    const std::string presets = form_field(body, "presets");

    // Validate the pasted JSON before persisting anything (design doc G7 fix).
    std::vector<PpaScene> scenes;
    if (!presets.empty() && !scene_model_parse(presets.c_str(), scenes)) {
        const std::string page =
            render_form("presets.json ist kein gültiges JSON – nichts gespeichert.");
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, page.c_str(), page.size());
    }

    if (!config_store_save(ssid, pass, presets)) {
        const std::string page = render_form("Speichern fehlgeschlagen.");
        httpd_resp_set_type(req, "text/html");
        return httpd_resp_send(req, page.c_str(), page.size());
    }

    const char *done =
        "<meta charset='utf-8'><body style='background:#111;color:#eee;"
        "font-family:sans-serif'><h2>Gespeichert &ndash; Dial startet neu &hellip;</h2>";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, done, HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG, "config saved (%d scenes), restarting", static_cast<int>(scenes.size()));
    vTaskDelay(pdMS_TO_TICKS(800));
    esp_restart();
    return ESP_OK;
}
} // namespace

bool web_setup_start() {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size = 8192;
    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed");
        return false;
    }
    static const httpd_uri_t root = {"/", HTTP_GET, handle_root, nullptr};
    static const httpd_uri_t save = {"/save", HTTP_POST, handle_save, nullptr};
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &save);
    ESP_LOGI(TAG, "setup server started");
    return true;
}
