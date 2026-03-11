#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wifi_console_register_extra_fn_t)(void);

typedef struct {
    const char *prompt;
    wifi_console_register_extra_fn_t register_extra;
} wifi_console_config_t;

void wifi_console_start(const wifi_console_config_t *cfg);

#ifdef __cplusplus
}  // extern "C"
#endif

