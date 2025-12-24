#pragma once

// Simple in-memory registry of GIF assets scanned from /storage/gifs.
int gif_registry_refresh(void);
int gif_registry_count(void);
const char *gif_registry_path(int idx);

// Find by basename (case-insensitive). Accepts "foo.gif" or "foo" for a file "foo.gif".
int gif_registry_find_by_name(const char *name);

// Basename helper for printing paths.
const char *path_basename(const char *path);


