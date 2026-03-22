#include "ereader_app.h"
#include "ereader_console.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void ereader_task(void* /*arg*/)
{
    ereader::GetApp().Run();
}

extern "C" void app_main(void)
{
    ereader::ConsoleInit();
    xTaskCreatePinnedToCore(ereader_task, "ereader_ui", 8192, nullptr, 5, nullptr, 1);
}
