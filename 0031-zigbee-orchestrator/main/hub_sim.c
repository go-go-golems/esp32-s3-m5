/*
 * Virtual device simulator for tutorial 0029.
 *
 * Periodically emits HUB_EVT_DEVICE_REPORT events (telemetry) for devices with matching caps.
 */

#include "hub_sim.h"

#include <math.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

#include "hub_bus.h"
#include "hub_registry.h"
#include "hub_types.h"

static const char *TAG = "hub_sim_0029";

static TaskHandle_t s_task = NULL;

static float rand_unit(void) {
    // [0,1)
    return (float)((double)esp_random() / (double)UINT32_MAX);
}

static float rand_walk(float cur, float step, float min_v, float max_v) {
    const float delta = (rand_unit() * 2.0f - 1.0f) * step;
    float next = cur + delta;
    if (next < min_v) next = min_v;
    if (next > max_v) next = max_v;
    return next;
}

static void sim_task(void *arg) {
    (void)arg;
    hub_device_t snap[32];

    while (true) {
        size_t n = 0;
        (void)hub_registry_snapshot(snap, sizeof(snap) / sizeof(snap[0]), &n);

        for (size_t i = 0; i < n; i++) {
            hub_device_t d = snap[i];

            bool dirty = false;
            hub_evt_device_report_t rep = {
                .ts_us = esp_timer_get_time(),
                .device_id = d.id,
                .has_power = false,
                .power_w = 0.0f,
                .has_temperature = false,
                .temperature_c = 0.0f,
            };

            if ((d.caps & HUB_CAP_POWER) && d.on) {
                // Simulate a power draw based on type and brightness.
                const float target = (d.caps & HUB_CAP_LEVEL) ? (0.5f + (float)d.level / 100.0f) : 1.0f;
                const float base = (d.type == HUB_DEVICE_PLUG) ? 25.0f : 8.0f;
                const float max = (d.type == HUB_DEVICE_PLUG) ? 120.0f : 35.0f;
                const float desired = base * target;

                d.power_w = rand_walk(d.power_w, 5.0f, 0.0f, max);
                // Nudge towards desired.
                d.power_w = 0.8f * d.power_w + 0.2f * desired;

                rep.has_power = true;
                rep.power_w = d.power_w;
                dirty = true;
            } else if ((d.caps & HUB_CAP_POWER) && !d.on && d.power_w != 0.0f) {
                d.power_w = 0.0f;
                rep.has_power = true;
                rep.power_w = 0.0f;
                dirty = true;
            }

            if (d.caps & HUB_CAP_TEMPERATURE) {
                d.temperature_c = rand_walk(d.temperature_c, 0.2f, 15.0f, 30.0f);
                rep.has_temperature = true;
                rep.temperature_c = d.temperature_c;
                dirty = true;
            }

            if (dirty) {
                (void)hub_registry_update(d.id, &d);

                esp_event_loop_handle_t loop = hub_bus_get_loop();
                if (loop) {
                    (void)esp_event_post_to(loop, HUB_EVT, HUB_EVT_DEVICE_REPORT, &rep, sizeof(rep), 0);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0029_SIM_PERIOD_MS));
    }
}

esp_err_t hub_sim_start(void) {
    if (s_task) {
        return ESP_OK;
    }
    BaseType_t ok = xTaskCreate(sim_task, "hub_sim", 4096, NULL, 5, &s_task);
    if (ok != pdPASS) {
        s_task = NULL;
        ESP_LOGE(TAG, "xTaskCreate failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void hub_sim_stop(void) {
    if (!s_task) {
        return;
    }
    vTaskDelete(s_task);
    s_task = NULL;
}

