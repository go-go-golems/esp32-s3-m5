#include "gnosis_app.h"
#include "gnosis_console.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// The display/touch loop runs on a dedicated task so the console REPL
// can share the main task with FreeRTOS.

static void gnosis_task(void* /*arg*/)
{
    gnosis::GetApp().Run();  // never returns
}

extern "C" void app_main(void)
{
    // Start the console REPL first (runs on the REPL task internally).
    gnosis::ConsoleInit();

    // Launch the display/UI task on core 1 with generous stack.
    xTaskCreatePinnedToCore(gnosis_task, "gnosis_ui", 8192, nullptr, 5, nullptr, 1);
}
