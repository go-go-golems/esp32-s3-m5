// PPA protocol engine: owns the UDP socket (port 5001), the uid->IP discovery
// cache, and the concurrent recall state machines. Runs as its own FreeRTOS
// task; the UI communicates via queues and never blocks on the network.
#pragma once

#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "scene_model.h"

enum class PpaEventType {
    kDiscoveryUpdate, // online counts may have changed
    kRecallProgress,  // actions_done/actions_total updated
    kRecallDone,      // ok = all actions confirmed
};

struct PpaEvent {
    PpaEventType type;
    int scene_index;
    int actions_done;
    int actions_total;
    bool ok;
};

// Starts the task. Events are posted to event_out (queue of PpaEvent).
bool ppa_client_start(QueueHandle_t event_out);

// Replaces the scene list (call at boot and after config save). Thread-safe.
void ppa_client_set_scenes(const std::vector<PpaScene> &scenes);

// Requests an async scene recall; progress/result arrive as events.
void ppa_client_request_recall(int scene_index);

// Number of actions of the scene whose module IP is currently known.
int ppa_client_online_count(int scene_index);
