#include "httpd_assets_embed.h"

size_t httpd_assets_embed_len(const uint8_t *start, const uint8_t *end, bool trim_trailing_nul)
{
    if (!start || !end || end < start) return 0;
    size_t len = (size_t)(end - start);
    if (trim_trailing_nul && len > 0 && start[len - 1] == 0) len--;
    return len;
}

esp_err_t httpd_assets_embed_send(httpd_req_t *req,
                                  const uint8_t *start,
                                  const uint8_t *end,
                                  const char *content_type,
                                  const char *cache_control,
                                  bool trim_trailing_nul)
{
    if (!req || !start || !end) return ESP_ERR_INVALID_ARG;
    if (content_type) (void)httpd_resp_set_type(req, content_type);
    if (cache_control) (void)httpd_resp_set_hdr(req, "cache-control", cache_control);
    const size_t len = httpd_assets_embed_len(start, end, trim_trailing_nul);
    return httpd_resp_send(req, (const char *)start, len);
}

