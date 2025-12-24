#pragma once

// Simple in-memory registry of GIF assets scanned from storage.
int echo_gif_registry_refresh(void);
int echo_gif_registry_count(void);
const char *echo_gif_registry_path(int idx);

// Find by basename (case-insensitive). Accepts "foo.gif" or "foo" for a file "foo.gif".
int echo_gif_registry_find_by_name(const char *name);

// Basename helper for printing paths.
const char *echo_gif_path_basename(const char *path);


