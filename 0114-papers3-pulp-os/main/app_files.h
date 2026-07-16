// Bounded general SD access for JS (ESP-53 P2). Owner-task-only.
//
// Path discipline: JS supplies rooted virtual paths ("/notes/a.txt"); the
// sanitizer validates and prefixes /sdcard natively. Segments starting
// with '.' are denied (hidden entries — including the /sdcard/.s3paper
// state directory — are native-owned). Operations run synchronously on
// the owner (SD whole-file ops at these caps are single-digit ms) but
// deliver through the completion-mailbox path so JS sees one async model
// everywhere.
#pragma once

#include <stdint.h>

#include "s3paper/status.h"

namespace pulp {

using FilesStatusCode = s3paper::StatusCode;

constexpr uint32_t kFilesMaxPath = 96;      // virtual path cap
constexpr uint32_t kFilesMaxList = 32;      // listing mailbox entries
constexpr uint32_t kFilesMaxBody = 16384;   // read/write byte cap
constexpr uint32_t kFilesMaxLines = 512;    // read mailbox line index

// Validates a virtual path and writes the real "/sdcard/..." path into
// out (cap >= kFilesMaxPath + 8). Rules: leading '/', charset
// [A-Za-z0-9._-/], no ".." or "." segments, no leading-dot segments, no
// "//", no trailing '/' (except the root itself).
FilesStatusCode FilesResolvePath(const char *virtual_path, char *out,
                                 uint32_t cap);

// Sync: 1 = exists (file or dir), 0 = not. Invalid paths read as 0.
int32_t FilesExists(const char *virtual_path);

// Each op validates, executes, fills its mailbox, and posts
// ModuleDone{Files, kind, value, err}. Return != Ok means nothing was
// posted (bad path / busy is reported synchronously to the caller).
FilesStatusCode FilesList(const char *virtual_path);
FilesStatusCode FilesRead(const char *virtual_path);
FilesStatusCode FilesWrite(const char *virtual_path, const char *body,
                           uint32_t len);
FilesStatusCode FilesAppend(const char *virtual_path, const char *line,
                            uint32_t len);
FilesStatusCode FilesRemove(const char *virtual_path);

// Listing mailbox accessors (valid after a list completion).
uint32_t FilesListCount();
const char *FilesListName(uint32_t i);   // nullptr out of range
int32_t FilesListSize(uint32_t i);
int32_t FilesListIsDir(uint32_t i);

// Read mailbox accessors (valid after a read completion).
uint32_t FilesLineCount();
// Returns the line's bytes (not NUL-terminated) or nullptr; *len out.
const char *FilesLine(uint32_t i, uint32_t *len);

}  // namespace pulp
