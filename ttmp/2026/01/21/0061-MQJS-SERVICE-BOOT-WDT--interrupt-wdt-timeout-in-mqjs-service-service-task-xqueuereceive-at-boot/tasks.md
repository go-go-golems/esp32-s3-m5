# Tasks

## TODO

- [x] Capture a complete failing boot log (include `app_init` “App version” + SHA + build timestamp)
- [x] Prove we are flashing the intended binary:
- [x] `idf.py -B build_esp32s3_mqjs_service_comp fullclean`
- [x] `idf.py -B build_esp32s3_mqjs_service_comp build`
- [x] `idf.py -B build_esp32s3_mqjs_service_comp flash monitor`
  - [ ] Optional: `idf.py -B build_esp32s3_mqjs_service_comp erase-flash flash monitor`
- [ ] Add boot-time instrumentation (minimal, before the first `xQueueReceive`):
- [x] `components/mqjs_service/mqjs_service.cpp`: log `s`, `s->q`, `s->ready`, core id, task name/priority
  - [ ] `components/mqjs_service/mqjs_service.cpp`: add one-time “queue sanity” probes with a short timeout
- [ ] Diagnostic: pin mqjs_service to core1 (only to isolate CPU0/WDT interactions)
  - [ ] `0048-cardputer-js-web/main/js_service.cpp`: set `mqjs_service_config_t.task_core_id = 1`
- [ ] Add a startup self-test mode:
  - [ ] `mqjs_service_start`: send a `MSG_JOB` no-op with timeout and verify the worker acks it
  - [ ] If the ack fails, return a clear error (and log a single-line reason)
- [x] Force mqjs_service queue allocation in internal RAM via xQueueCreateWithCaps(MALLOC_CAP_INTERNAL)
- [x] Force mqjs_service start handshake semaphore allocation in internal RAM (xSemaphoreCreateBinaryWithCaps) or static
