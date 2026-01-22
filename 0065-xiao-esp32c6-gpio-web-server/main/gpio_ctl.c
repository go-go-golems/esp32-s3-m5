#include "gpio_ctl.h"

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "mo065_gpio";

typedef struct {
    bool d2;
    bool d3;
} gpio_ctl_state_t;

static gpio_ctl_state_t s_state = {.d2 = false, .d3 = false};
static portMUX_TYPE s_mu = portMUX_INITIALIZER_UNLOCKED;

static inline int d2_gpio(void) { return CONFIG_MO065_GPIO_D2_GPIO_NUM; }
static inline int d3_gpio(void) { return CONFIG_MO065_GPIO_D3_GPIO_NUM; }
static inline bool d2_active_low(void) { return CONFIG_MO065_GPIO_D2_ACTIVE_LOW; }
static inline bool d3_active_low(void) { return CONFIG_MO065_GPIO_D3_ACTIVE_LOW; }

static int logical_to_level(bool logical, bool active_low)
{
    return (logical ^ active_low) ? 1 : 0;
}

static void apply_levels_unsafe(void)
{
    (void)gpio_set_level((gpio_num_t)d2_gpio(), logical_to_level(s_state.d2, d2_active_low()));
    (void)gpio_set_level((gpio_num_t)d3_gpio(), logical_to_level(s_state.d3, d3_active_low()));
}

esp_err_t gpio_ctl_init(void)
{
    const gpio_num_t d2 = (gpio_num_t)d2_gpio();
    const gpio_num_t d3 = (gpio_num_t)d3_gpio();

    if (d2_gpio() < 0 || d2_gpio() >= 64 || d3_gpio() < 0 || d3_gpio() >= 64 || d2_gpio() == d3_gpio()) {
        ESP_LOGE(TAG, "invalid gpio config: d2=%d d3=%d", d2_gpio(), d3_gpio());
        return ESP_ERR_INVALID_ARG;
    }

    const uint64_t mask = (1ULL << (uint64_t)d2) | (1ULL << (uint64_t)d3);
    gpio_config_t cfg = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    const esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) return err;

    portENTER_CRITICAL(&s_mu);
    s_state.d2 = false;
    s_state.d3 = false;
    apply_levels_unsafe();
    portEXIT_CRITICAL(&s_mu);

    ESP_LOGI(TAG, "gpio init: d2=gpio%d(active_low=%s) d3=gpio%d(active_low=%s)",
             d2_gpio(),
             d2_active_low() ? "yes" : "no",
             d3_gpio(),
             d3_active_low() ? "yes" : "no");
    return ESP_OK;
}

void gpio_ctl_get(bool *out_d2, bool *out_d3)
{
    portENTER_CRITICAL(&s_mu);
    const bool d2 = s_state.d2;
    const bool d3 = s_state.d3;
    portEXIT_CRITICAL(&s_mu);

    if (out_d2) *out_d2 = d2;
    if (out_d3) *out_d3 = d3;
}

esp_err_t gpio_ctl_set_d2(bool logical_level)
{
    portENTER_CRITICAL(&s_mu);
    s_state.d2 = logical_level;
    apply_levels_unsafe();
    portEXIT_CRITICAL(&s_mu);
    return ESP_OK;
}

esp_err_t gpio_ctl_set_d3(bool logical_level)
{
    portENTER_CRITICAL(&s_mu);
    s_state.d3 = logical_level;
    apply_levels_unsafe();
    portEXIT_CRITICAL(&s_mu);
    return ESP_OK;
}

esp_err_t gpio_ctl_set(bool has_d2, bool d2_level, bool has_d3, bool d3_level)
{
    portENTER_CRITICAL(&s_mu);
    if (has_d2) s_state.d2 = d2_level;
    if (has_d3) s_state.d3 = d3_level;
    apply_levels_unsafe();
    portEXIT_CRITICAL(&s_mu);
    return ESP_OK;
}

esp_err_t gpio_ctl_toggle_d2(void)
{
    portENTER_CRITICAL(&s_mu);
    s_state.d2 = !s_state.d2;
    apply_levels_unsafe();
    portEXIT_CRITICAL(&s_mu);
    return ESP_OK;
}

esp_err_t gpio_ctl_toggle_d3(void)
{
    portENTER_CRITICAL(&s_mu);
    s_state.d3 = !s_state.d3;
    apply_levels_unsafe();
    portEXIT_CRITICAL(&s_mu);
    return ESP_OK;
}

int gpio_ctl_d2_gpio_num(void) { return d2_gpio(); }
int gpio_ctl_d3_gpio_num(void) { return d3_gpio(); }
bool gpio_ctl_d2_active_low(void) { return d2_active_low(); }
bool gpio_ctl_d3_active_low(void) { return d3_active_low(); }
