#include "control_plane.h"

QueueHandle_t ctrl_create_queue(size_t len) {
    return xQueueCreate((UBaseType_t)len, sizeof(CtrlEvent));
}

bool ctrl_send(QueueHandle_t q, const CtrlEvent &ev, TickType_t wait) {
    if (!q) return false;
    return xQueueSend(q, &ev, wait) == pdTRUE;
}

bool ctrl_recv(QueueHandle_t q, CtrlEvent *out, TickType_t wait) {
    if (!q || !out) return false;
    return xQueueReceive(q, out, wait) == pdTRUE;
}

