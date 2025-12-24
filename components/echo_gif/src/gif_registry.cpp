/*
 * echo_gif: GIF registry (scan configured directory into an in-memory list).
 */

#include "echo_gif/gif_registry.h"

#include <string.h>
#include <strings.h>

#include "sdkconfig.h"

#include "echo_gif/gif_storage.h"

static char s_gif_paths[CONFIG_ECHO_GIF_MAX_COUNT][CONFIG_ECHO_GIF_MAX_PATH_LEN];
static int s_gif_count = 0;

int echo_gif_registry_refresh(void) {
    s_gif_count = echo_gif_storage_list_gifs((char *)s_gif_paths,
                                             CONFIG_ECHO_GIF_MAX_COUNT,
                                             CONFIG_ECHO_GIF_MAX_PATH_LEN);
    return s_gif_count;
}

int echo_gif_registry_count(void) {
    return s_gif_count;
}

const char *echo_gif_registry_path(int idx) {
    if (idx < 0 || idx >= s_gif_count) {
        return nullptr;
    }
    return s_gif_paths[idx];
}

const char *echo_gif_path_basename(const char *path) {
    if (!path) return "";
    const char *slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

int echo_gif_registry_find_by_name(const char *name) {
    if (!name || !*name) return -1;
    for (int i = 0; i < s_gif_count; i++) {
        const char *p = s_gif_paths[i];
        const char *base = echo_gif_path_basename(p);
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


