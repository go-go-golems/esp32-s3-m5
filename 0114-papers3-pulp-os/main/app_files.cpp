#include "app_files.h"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "app_events.h"
#include "app_owner.h"
#include "s3paper_storage/storage.h"

namespace pulp {
namespace {

const char *kTag = "files";
constexpr char kRoot[] = "/sdcard";
constexpr uint32_t kRealPathCap = kFilesMaxPath + 8;

struct ListEntry {
    char name[40];
    uint32_t size;
    uint8_t is_dir;
};

struct LineRef {
    uint16_t start;
    uint16_t len;
};

struct FilesState {
    ListEntry list[kFilesMaxList];
    uint32_t list_count = 0;
    char *body = nullptr;  // PSRAM, kFilesMaxBody
    LineRef lines[kFilesMaxLines];
    uint32_t line_count = 0;
};

FilesState s_state;

bool EnsureBody() {
    if (s_state.body != nullptr) {
        return true;
    }
    s_state.body = static_cast<char *>(
        heap_caps_malloc(kFilesMaxBody, MALLOC_CAP_SPIRAM));
    if (s_state.body == nullptr) {
        s_state.body = static_cast<char *>(malloc(kFilesMaxBody));
    }
    return s_state.body != nullptr;
}

bool ValidChar(char c) {
    return isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' ||
           c == '-' || c == '/';
}

// Splits the resolved path into parent + leaf and mkdirs the parent (one
// level, e.g. /sdcard/notes). Deeper trees must be built by listing apps
// level by level; this keeps write() convenient without a mkdir verb.
void EnsureParentDir(const char *real_path) {
    char parent[kRealPathCap];
    snprintf(parent, sizeof(parent), "%s", real_path);
    char *slash = strrchr(parent, '/');
    if (slash == nullptr || slash == parent) {
        return;
    }
    *slash = '\0';
    if (strcmp(parent, kRoot) != 0) {
        mkdir(parent, 0775);
    }
}

// ESP-57: returns false when the completion could not be queued. The
// caller MUST propagate that as a non-Ok status so the JS binding cancels
// the registered callback — a completion that never queues means the
// callback would otherwise wait forever (silent catalog-scan death).
bool Post(int32_t kind, int32_t value, int32_t err) {
    const StatusCode posted =
        PostModuleDone(ModuleId::Files, kind, value, err);
    if (posted != StatusCode::Ok) {
        ESP_LOGW(kTag, "completion post failed: %s",
                 StatusCodeName(posted));
        return false;
    }
    return true;
}

}  // namespace

FilesStatusCode FilesResolvePath(const char *virtual_path, char *out,
                                 uint32_t cap) {
    if (virtual_path == nullptr || out == nullptr ||
        cap < kRealPathCap) {
        return FilesStatusCode::InvalidArgument;
    }
    const size_t len = strlen(virtual_path);
    if (len == 0 || len >= kFilesMaxPath || virtual_path[0] != '/') {
        return FilesStatusCode::InvalidArgument;
    }
    for (size_t i = 0; i < len; ++i) {
        if (!ValidChar(virtual_path[i])) {
            return FilesStatusCode::InvalidArgument;
        }
        if (virtual_path[i] == '/') {
            if (i + 1 < len && virtual_path[i + 1] == '/') {
                return FilesStatusCode::InvalidArgument;  // "//"
            }
            // Hidden/dot segments are native-owned (covers /.s3paper,
            // "..", and "." in one rule).
            if (i + 1 < len && virtual_path[i + 1] == '.') {
                return FilesStatusCode::InvalidArgument;
            }
        }
    }
    if (len > 1 && virtual_path[len - 1] == '/') {
        return FilesStatusCode::InvalidArgument;
    }
    snprintf(out, cap, "%s%s", kRoot,
             (len == 1) ? "" : virtual_path);
    return FilesStatusCode::Ok;
}

int32_t FilesExists(const char *virtual_path) {
    char real[kRealPathCap];
    if (FilesResolvePath(virtual_path, real, sizeof(real)) !=
        FilesStatusCode::Ok) {
        return 0;
    }
    struct stat st;
    return stat(real, &st) == 0 ? 1 : 0;
}

FilesStatusCode FilesList(const char *virtual_path) {
    char real[kRealPathCap];
    const FilesStatusCode resolved =
        FilesResolvePath(virtual_path, real, sizeof(real));
    if (resolved != FilesStatusCode::Ok) {
        return resolved;
    }
    s_state.list_count = 0;
    DIR *dir = opendir(real);
    if (dir == nullptr) {
        return Post(kDoneFilesList, 0, errno != 0 ? errno : -1)
                   ? FilesStatusCode::Ok
                   : FilesStatusCode::CapacityExceeded;
    }
    uint32_t skipped = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') {
            continue;  // hidden entries stay invisible to JS
        }
        // A truncated name could not be stat'ed or reopened by JS; names
        // beyond the mailbox field are skipped (and counted), not mangled.
        if (strlen(entry->d_name) >= sizeof(s_state.list[0].name) ||
            s_state.list_count >= kFilesMaxList) {
            skipped++;
            continue;
        }
        ListEntry &out = s_state.list[s_state.list_count];
        snprintf(out.name, sizeof(out.name), "%.39s", entry->d_name);
        out.is_dir = entry->d_type == DT_DIR ? 1 : 0;
        out.size = 0;
        if (!out.is_dir) {
            char full[kRealPathCap + sizeof(out.name)];
            snprintf(full, sizeof(full), "%s/%.39s", real, entry->d_name);
            struct stat st;
            if (stat(full, &st) == 0) {
                out.size = static_cast<uint32_t>(st.st_size);
            }
        }
        s_state.list_count++;
    }
    closedir(dir);
    if (skipped != 0) {
        ESP_LOGW(kTag, "list %s: %u entries beyond cap dropped",
                 virtual_path, static_cast<unsigned>(skipped));
    }
    return Post(kDoneFilesList,
                static_cast<int32_t>(s_state.list_count), 0)
               ? FilesStatusCode::Ok
               : FilesStatusCode::CapacityExceeded;
}

FilesStatusCode FilesRead(const char *virtual_path) {
    char real[kRealPathCap];
    const FilesStatusCode resolved =
        FilesResolvePath(virtual_path, real, sizeof(real));
    if (resolved != FilesStatusCode::Ok) {
        return resolved;
    }
    if (!EnsureBody()) {
        return FilesStatusCode::OutOfMemory;
    }
    s_state.line_count = 0;
    FILE *f = fopen(real, "rb");
    if (f == nullptr) {
        return Post(kDoneFilesRead, 0, errno != 0 ? errno : -1)
                   ? FilesStatusCode::Ok
                   : FilesStatusCode::CapacityExceeded;
    }
    const size_t got = fread(s_state.body, 1, kFilesMaxBody, f);
    fclose(f);
    // Line index over the buffer; CR stripped at the accessor.
    uint32_t start = 0;
    for (uint32_t i = 0; i <= got; ++i) {
        const bool at_end = (i == got);
        if (!at_end && s_state.body[i] != '\n') {
            continue;
        }
        if (at_end && i == start) {
            break;  // no trailing empty line
        }
        if (s_state.line_count >= kFilesMaxLines) {
            ESP_LOGW(kTag, "read %s: line index full, tail dropped",
                     virtual_path);
            break;
        }
        LineRef &ref = s_state.lines[s_state.line_count++];
        ref.start = static_cast<uint16_t>(start);
        ref.len = static_cast<uint16_t>(i - start);
        start = i + 1;
    }
    return Post(kDoneFilesRead,
                static_cast<int32_t>(s_state.line_count), 0)
               ? FilesStatusCode::Ok
               : FilesStatusCode::CapacityExceeded;
}

FilesStatusCode FilesWrite(const char *virtual_path, const char *body,
                           uint32_t len) {
    char real[kRealPathCap];
    const FilesStatusCode resolved =
        FilesResolvePath(virtual_path, real, sizeof(real));
    if (resolved != FilesStatusCode::Ok) {
        return resolved;
    }
    if (len > kFilesMaxBody) {
        return FilesStatusCode::CapacityExceeded;
    }
    EnsureParentDir(real);
    FILE *f = fopen(real, "wb");
    if (f == nullptr) {
        return Post(kDoneFilesWrite, 0, errno != 0 ? errno : -1)
                   ? FilesStatusCode::Ok
                   : FilesStatusCode::CapacityExceeded;
    }
    const size_t wrote = fwrite(body, 1, len, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    return Post(kDoneFilesWrite, static_cast<int32_t>(wrote),
                wrote == len ? 0 : -1)
               ? FilesStatusCode::Ok
               : FilesStatusCode::CapacityExceeded;
}

FilesStatusCode FilesAppend(const char *virtual_path, const char *line,
                            uint32_t len) {
    char real[kRealPathCap];
    const FilesStatusCode resolved =
        FilesResolvePath(virtual_path, real, sizeof(real));
    if (resolved != FilesStatusCode::Ok) {
        return resolved;
    }
    if (len > kFilesMaxBody) {
        return FilesStatusCode::CapacityExceeded;
    }
    EnsureParentDir(real);
    FILE *f = fopen(real, "ab");
    if (f == nullptr) {
        return Post(kDoneFilesAppend, 0, errno != 0 ? errno : -1)
                   ? FilesStatusCode::Ok
                   : FilesStatusCode::CapacityExceeded;
    }
    size_t wrote = fwrite(line, 1, len, f);
    wrote += fwrite("\n", 1, 1, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    return Post(kDoneFilesAppend, static_cast<int32_t>(wrote),
                wrote == len + 1 ? 0 : -1)
               ? FilesStatusCode::Ok
               : FilesStatusCode::CapacityExceeded;
}

FilesStatusCode FilesRemove(const char *virtual_path) {
    char real[kRealPathCap];
    const FilesStatusCode resolved =
        FilesResolvePath(virtual_path, real, sizeof(real));
    if (resolved != FilesStatusCode::Ok) {
        return resolved;
    }
    int rc = unlink(real);
    if (rc != 0 && errno == EISDIR) {
        rc = rmdir(real);
    }
    return Post(kDoneFilesRemove, rc == 0 ? 1 : 0,
                rc == 0 ? 0 : (errno != 0 ? errno : -1))
               ? FilesStatusCode::Ok
               : FilesStatusCode::CapacityExceeded;
}

uint32_t FilesListCount() { return s_state.list_count; }

const char *FilesListName(uint32_t i) {
    return i < s_state.list_count ? s_state.list[i].name : nullptr;
}

int32_t FilesListSize(uint32_t i) {
    return i < s_state.list_count
               ? static_cast<int32_t>(s_state.list[i].size)
               : -1;
}

int32_t FilesListIsDir(uint32_t i) {
    return i < s_state.list_count ? s_state.list[i].is_dir : -1;
}

uint32_t FilesLineCount() { return s_state.line_count; }

const char *FilesLine(uint32_t i, uint32_t *len) {
    if (i >= s_state.line_count || s_state.body == nullptr) {
        return nullptr;
    }
    const LineRef &ref = s_state.lines[i];
    uint32_t n = ref.len;
    // Strip a trailing CR so CRLF files read cleanly.
    if (n > 0 && s_state.body[ref.start + n - 1] == '\r') {
        n--;
    }
    *len = n;
    return s_state.body + ref.start;
}

}  // namespace pulp
