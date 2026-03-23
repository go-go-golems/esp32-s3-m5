#include "ereader_app.h"
#include "ereader_console.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void ereader_task(void* /*arg*/)
{
    // InitBoard (M5.begin) already called on main task.
    // Run the main loop here on core 1.
    ereader::GetApp().RunLoop();
}

extern "C" void app_main(void)
{
    // Init display on the main task (core 0) — M5.begin() allocates the
    // EPD framebuffer in PSRAM and must complete before any drawing.
    ereader::GetApp().Init();

    ereader::ConsoleInit();
    xTaskCreatePinnedToCore(ereader_task, "ereader_ui", 16384, nullptr, 5, nullptr, 1);
}
