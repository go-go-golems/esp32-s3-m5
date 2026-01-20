#include "ws281x_encoder.h"

#include <sys/cdefs.h>

#include "driver/rmt_tx.h"
#include "esp_check.h"

static const char *TAG = "ws281x_encoder";

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_ws281x_encoder_t;

static inline uint32_t ws281x_ns_to_ticks(uint32_t ns, uint32_t resolution_hz)
{
    // Round up so we never produce 0-tick pulses.
    uint64_t ticks = ((uint64_t)ns * (uint64_t)resolution_hz + 1000000000ULL - 1) / 1000000000ULL;
    if (ticks == 0) {
        ticks = 1;
    }
    return (uint32_t)ticks;
}

static inline uint32_t ws281x_us_to_ticks(uint32_t us, uint32_t resolution_hz)
{
    uint64_t ticks = ((uint64_t)us * (uint64_t)resolution_hz + 1000000ULL - 1) / 1000000ULL;
    if (ticks == 0) {
        ticks = 1;
    }
    return (uint32_t)ticks;
}

static size_t rmt_encode_ws281x(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_ws281x_encoder_t *ws = __containerof(encoder, rmt_ws281x_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = ws->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = ws->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (ws->state) {
    case 0:
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            ws->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    // fall-through
    case 1:
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &ws->reset_code, sizeof(ws->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            ws->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }

out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_ws281x_encoder(rmt_encoder_t *encoder)
{
    rmt_ws281x_encoder_t *ws = __containerof(encoder, rmt_ws281x_encoder_t, base);
    rmt_del_encoder(ws->bytes_encoder);
    rmt_del_encoder(ws->copy_encoder);
    free(ws);
    return ESP_OK;
}

static esp_err_t rmt_ws281x_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_ws281x_encoder_t *ws = __containerof(encoder, rmt_ws281x_encoder_t, base);
    rmt_encoder_reset(ws->bytes_encoder);
    rmt_encoder_reset(ws->copy_encoder);
    ws->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

esp_err_t rmt_new_ws281x_encoder(const ws281x_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_ws281x_encoder_t *ws = NULL;

    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    ESP_GOTO_ON_FALSE(config->resolution_hz > 0, ESP_ERR_INVALID_ARG, err, TAG, "invalid resolution");

    ws = rmt_alloc_encoder_mem(sizeof(rmt_ws281x_encoder_t));
    ESP_GOTO_ON_FALSE(ws, ESP_ERR_NO_MEM, err, TAG, "no mem for ws281x encoder");

    ws->base.encode = rmt_encode_ws281x;
    ws->base.del = rmt_del_ws281x_encoder;
    ws->base.reset = rmt_ws281x_encoder_reset;

    rmt_bytes_encoder_config_t bytes_cfg = {
        .bit0 = {
            .level0 = 1,
            .duration0 = ws281x_ns_to_ticks(config->t0h_ns, config->resolution_hz),
            .level1 = 0,
            .duration1 = ws281x_ns_to_ticks(config->t0l_ns, config->resolution_hz),
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = ws281x_ns_to_ticks(config->t1h_ns, config->resolution_hz),
            .level1 = 0,
            .duration1 = ws281x_ns_to_ticks(config->t1l_ns, config->resolution_hz),
        },
        .flags.msb_first = 1, // WS281x expects MSB-first, often GRB byte order in the frame buffer
    };
    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_cfg, &ws->bytes_encoder), err, TAG, "create bytes encoder failed");

    rmt_copy_encoder_config_t copy_cfg = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_cfg, &ws->copy_encoder), err, TAG, "create copy encoder failed");

    const uint32_t reset_ticks = ws281x_us_to_ticks(config->reset_us, config->resolution_hz) / 2;
    ws->reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };

    *ret_encoder = &ws->base;
    return ESP_OK;

err:
    if (ws) {
        if (ws->bytes_encoder) {
            rmt_del_encoder(ws->bytes_encoder);
        }
        if (ws->copy_encoder) {
            rmt_del_encoder(ws->copy_encoder);
        }
        free(ws);
    }
    return ret;
}

