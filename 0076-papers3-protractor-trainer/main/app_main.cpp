#include "trainer_app.h"

extern "C" void app_main(void)
{
    protractor_demo::TrainerApp app;
    app.Run();
}
