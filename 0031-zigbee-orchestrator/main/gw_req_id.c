#include "gw_req_id.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static portMUX_TYPE s_mu = portMUX_INITIALIZER_UNLOCKED;
static uint64_t s_next = 1;

uint64_t gw_req_id_next(void) {
    portENTER_CRITICAL(&s_mu);
    const uint64_t v = s_next++;
    portEXIT_CRITICAL(&s_mu);
    return v;
}

