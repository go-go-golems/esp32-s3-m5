/*
 * Tutorial 0013: GIF registry (scan /storage/gifs into an in-memory list).
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "sdkconfig.h"

#include "gif_registry.h"
#include "gif_storage.h"

static char s_gif_paths[CONFIG_TUTORIAL_0013_GIF_MAX_COUNT][CONFIG_TUTORIAL_0013_GIF_MAX_PATH_LEN];
static int s_gif_count = 0;

int gif_registry_refresh(void) {
    s_gif_count = gif_storage_list_gifs((char *)s_gif_paths,
                                        CONFIG_TUTORIAL_0013_GIF_MAX_COUNT,
                                        CONFIG_TUTORIAL_0013_GIF_MAX_PATH_LEN);
    return s_gif_count;
}

int gif_registry_count(void) {
    return s_gif_count;
}

const char *gif_registry_path(int idx) {
    if (idx < 0 || idx >= s_gif_count) {
        return nullptr;
    }
    return s_gif_paths[idx];
}

const char *path_basename(const char *path) {
    if (!path) return "";
    const char *slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

int gif_registry_find_by_name(const char *name) {
    if (!name || !*name) return -1;
    for (int i = 0; i < s_gif_count; i++) {
        const char *p = s_gif_paths[i];
        const char *base = path_basename(p);
        if (strcasecmp(name, base) == 0) return i;

        // Allow omitting ".gif"
        const size_t base_len = strlen(base);
        if (base_len > 4 && strcasecmp(base + (base_len - 4), ".gif") == 0) {
            const size_t stem_len = base_len - 4;
            if (strlen(name) == stem_len && strncasecmp(name, base, stem_len) == 0) {
                return i;
            }
        }
    }
    return -1;
}


