// LVGL screens for the PPA Dial (design doc §5.4). Must only be called from
// the UI task (the sole LVGL caller).
#pragma once

bool ui_init();

// Full-screen status page (connecting / AP mode / no scenes).
void ui_show_status(const char *title, const char *line1, const char *line2,
                    bool error, bool spinner);

// Scene carousel.
void ui_show_carousel();
// slide_dir: -1 = came from the right, +1 = from the left, 0 = no animation.
void ui_carousel_update(int index, int total, const char *name, bool is_active,
                        int active_index, int slide_dir);
void ui_carousel_set_online(int online, int total_actions);

// Activation overlay on top of the carousel.
void ui_activation_show(const char *scene_name);
void ui_activation_progress(int done, int total);
void ui_activation_result(bool ok, int done, int total);
void ui_activation_hide();
