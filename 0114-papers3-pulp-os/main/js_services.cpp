// Native services exposed to JS: the headless book service (same
// LayoutKey as 0112 so persisted positions interoperate), the library,
// the settings store, the postcard journal, and the battery.
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "app_js_internal.h"
#include "s3paper/content.h"
#include "s3paper/paginator.h"
#include "s3paper/text.h"
#include "s3paper_m5/m5_power.h"
#include "s3paper_runtime/runtime.h"
#include "s3paper_storage/storage.h"

namespace pulp {

using namespace jsi;

namespace {

struct JsBook {
    bool open = false;
    s3paper_storage::SdContentSource sd;
    s3paper::Paginator *paginator = nullptr;
    s3paper::ContentHash hash = 0;
    s3paper::TextLocator current{};
    s3paper::PageLayout page{};
    char title[40] = {};
};

JsBook s_book;

s3paper::LayoutKey JsBookKey(s3paper::ContentHash content) {
    s3paper::LayoutKey key{};
    key.content = content;
    key.font_id = s3paper::kFontBody;
    key.viewport_w = 540;
    key.viewport_h = 960;
    key.margin_x = 40;
    key.margin_top = 72;
    key.margin_bottom = 56;
    key.engine_version = s3paper::kLayoutEngineVersion;
    return key;
}

// Composes the page at `at` (by value: `at` may alias page.next).
StatusCode JsBookCompose(s3paper::TextLocator at) {
    const s3paper::Status composed =
        s_book.paginator->ComposePage(at, &s_book.page);
    if (!composed.ok()) {
        return composed.code;
    }
    s_book.current = at;
    if (s3paper_storage::StorageMounted()) {
        s3paper_storage::PositionStore(s_book.hash, s_book.current);
    }
    return StatusCode::Ok;
}

}  // namespace

extern "C" {

JSValue js_pulp_book_open(JSContext *ctx, JSValue *, int argc,
                          JSValue *argv) {
    int index = -1;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    delete s_book.paginator;
    s_book.paginator = nullptr;
    s_book.open = false;
    s_book.sd.Close();
    const s3paper_storage::BookEntry *book =
        s3paper_storage::LibraryGet(static_cast<uint32_t>(index));
    if (index < 0 || book == nullptr) {
        return JS_NewInt32(
            ctx, static_cast<int32_t>(StatusCode::InvalidArgument));
    }
    if (s_book.sd.Open(book->path) != StatusCode::Ok) {
        return JS_NewInt32(ctx, static_cast<int32_t>(StatusCode::Busy));
    }
    snprintf(s_book.title, sizeof(s_book.title), "%s", book->title);
    const s3paper::Result<s3paper::ContentHash> hash = s_book.sd.Hash();
    if (!hash.ok()) {
        return JS_NewInt32(ctx, static_cast<int32_t>(hash.code));
    }
    s_book.hash = hash.value;
    s_book.paginator =
        new s3paper::Paginator(&s_book.sd, JsBookKey(hash.value));
    s3paper::TextLocator start{};
    s3paper::TextLocator persisted{};
    if (s3paper_storage::StorageMounted() &&
        s3paper_storage::PositionLookup(s_book.hash, &persisted) &&
        s_book.paginator->Validate(persisted).ok()) {
        start = persisted;  // resumes exactly where 0112 left the book
    } else {
        const s3paper::Result<s3paper::TextLocator> begin =
            s_book.paginator->Begin();
        if (!begin.ok()) {
            return JS_NewInt32(ctx, static_cast<int32_t>(begin.code));
        }
        start = begin.value;
    }
    const StatusCode composed = JsBookCompose(start);
    if (composed == StatusCode::Ok) {
        s_book.open = true;
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(composed));
}

JSValue js_pulp_book_title(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewString(ctx, s_book.open ? s_book.title : "");
}

JSValue js_pulp_book_line_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(
        ctx, s_book.open ? static_cast<int32_t>(s_book.page.line_count)
                         : 0);
}

JSValue js_pulp_book_line(JSContext *ctx, JSValue *, int argc,
                          JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    if (!s_book.open || index < 0 ||
        static_cast<uint32_t>(index) >= s_book.page.line_count) {
        return JS_ThrowTypeError(ctx, "line index out of range");
    }
    const s3paper::PageLine &line = s_book.page.lines[index];
    char buf[s3paper::TextProps::kCapacity];
    uint32_t len = line.byte_len < sizeof(buf) - 1
                       ? line.byte_len
                       : static_cast<uint32_t>(sizeof(buf) - 1);
    const s3paper::Result<uint32_t> got = s_book.sd.ReadAt(
        line.byte_start, reinterpret_cast<uint8_t *>(buf), len);
    if (!got.ok()) {
        return JS_ThrowTypeError(ctx, "line read failed");
    }
    len = got.value;
    while (len > 0 &&
           (static_cast<uint8_t>(buf[len - 1]) & 0xC0) == 0x80) {
        len--;  // never split a UTF-8 sequence at the truncation point
    }
    return JS_NewStringLen(ctx, buf, len);
}

JSValue js_pulp_book_next(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s_book.open) {
        return JS_NewInt32(ctx, static_cast<int32_t>(StatusCode::Busy));
    }
    if (s_book.page.at_end) {
        return JS_NewInt32(
            ctx, static_cast<int32_t>(StatusCode::InvalidArgument));
    }
    return JS_NewInt32(
        ctx, static_cast<int32_t>(JsBookCompose(s_book.page.next)));
}

JSValue js_pulp_book_prev(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s_book.open) {
        return JS_NewInt32(ctx, static_cast<int32_t>(StatusCode::Busy));
    }
    const s3paper::Result<s3paper::TextLocator> prev =
        s_book.paginator->PreviousPageStart(s_book.current);
    if (!prev.ok()) {
        return JS_NewInt32(ctx, static_cast<int32_t>(prev.code));
    }
    if (prev.value.byte_offset == s_book.current.byte_offset) {
        return JS_NewInt32(
            ctx, static_cast<int32_t>(StatusCode::InvalidArgument));
    }
    return JS_NewInt32(ctx,
                       static_cast<int32_t>(JsBookCompose(prev.value)));
}

JSValue js_pulp_book_progress(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s_book.open) {
        return JS_NewInt32(ctx, 0);
    }
    const s3paper::Result<uint32_t> progress =
        s_book.paginator->ProgressPermille(s_book.page.next);
    return JS_NewInt32(
        ctx, progress.ok() ? static_cast<int32_t>(progress.value) : 0);
}

// ---- library + persistence + power services ----

JSValue js_pulp_library_count(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_NewInt32(
        ctx, static_cast<int32_t>(s3paper_storage::LibraryCount()));
}

JSValue js_pulp_library_line(JSContext *ctx, JSValue *, int argc,
                             JSValue *argv) {
    int index = 0;
    if (argc < 1 || JS_ToInt32(ctx, &index, argv[0])) {
        return JS_EXCEPTION;
    }
    const s3paper_storage::BookEntry *book =
        s3paper_storage::LibraryGet(static_cast<uint32_t>(index));
    if (index < 0 || book == nullptr) {
        return JS_ThrowTypeError(ctx, "book index out of range");
    }
    char line[64];
    snprintf(line, sizeof(line), "%s  %uK", book->title,
             static_cast<unsigned>((book->size + 1023) / 1024));
    return JS_NewString(ctx, line);
}

JSValue js_pulp_library_rescan(JSContext *ctx, JSValue *, int, JSValue *) {
    uint32_t count = 0;
    const StatusCode scanned = s3paper_storage::LibraryScan(&count);
    if (scanned != StatusCode::Ok) {
        return JS_NewInt32(ctx, -1);
    }
    return JS_NewInt32(ctx, static_cast<int32_t>(count));
}

JSValue js_pulp_store_get(JSContext *ctx, JSValue *, int argc,
                          JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "storeGet(key, fallback)");
    }
    char key[16];
    JSValue err;
    if (!ArgString(ctx, argv[0], key, sizeof(key), &err)) {
        return err;
    }
    int fallback = 0;
    if (JS_ToInt32(ctx, &fallback, argv[1])) {
        return JS_EXCEPTION;
    }
    return JS_NewInt32(ctx, s3paper_storage::SettingsGet(key, fallback));
}

JSValue js_pulp_store_set(JSContext *ctx, JSValue *, int argc,
                          JSValue *argv) {
    if (argc < 2) {
        return JS_ThrowTypeError(ctx, "storeSet(key, value)");
    }
    char key[16];
    JSValue err;
    if (!ArgString(ctx, argv[0], key, sizeof(key), &err)) {
        return err;
    }
    int value = 0;
    if (JS_ToInt32(ctx, &value, argv[1])) {
        return JS_EXCEPTION;
    }
    s3paper_storage::SettingsSet(key, value);
    return JS_UNDEFINED;
}

// Appends one line to the fixed postcard journal. The path is native-owned
// (JS never supplies paths); the file is an ordinary .txt so the library
// scan lists your own journal as a book.
JSValue js_pulp_append_postcard(JSContext *ctx, JSValue *, int argc,
                                JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "appendPostcard(line)");
    }
    if (!s3paper_storage::StorageMounted()) {
        return JS_NewInt32(ctx, static_cast<int32_t>(StatusCode::Busy));
    }
    char line[96];
    JSValue err;
    if (!ArgString(ctx, argv[0], line, sizeof(line), &err)) {
        return err;
    }
    FILE *f = fopen("/sdcard/books/postcard.txt", "a");
    if (f == nullptr) {
        return JS_NewInt32(ctx,
                           static_cast<int32_t>(StatusCode::CorruptData));
    }
    fprintf(f, "%s\n\n", line);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    return JS_NewInt32(ctx, static_cast<int32_t>(StatusCode::Ok));
}

JSValue js_pulp_battery_level(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s3paper_runtime::M5BackendState().initialized) {
        return JS_NewInt32(ctx, -1);
    }
    const s3paper::PowerStatus st = s3paper::PowerRead();
    return JS_NewInt32(ctx, st.battery_level);
}

// ---- battery singleton (ESP-54): charging + mv already in PowerStatus ----
//
// All four are sync, owner-only, thin wrappers over s3paper::PowerRead().
// `batteryLevel()` (above) is kept as the historical alias; the new
// `battery.*` surface adds the missing charging/mv fields and a formatted
// status string for the home glyph.

JSValue js_battery_level(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s3paper_runtime::M5BackendState().initialized) {
        return JS_NewInt32(ctx, -1);
    }
    const s3paper::PowerStatus st = s3paper::PowerRead();
    return JS_NewInt32(ctx, st.battery_level);
}

JSValue js_battery_mv(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s3paper_runtime::M5BackendState().initialized) {
        return JS_NewInt32(ctx, -1);
    }
    const s3paper::PowerStatus st = s3paper::PowerRead();
    return JS_NewInt32(ctx, st.battery_mv);
}

JSValue js_battery_charging(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s3paper_runtime::M5BackendState().initialized) {
        return JS_NewInt32(ctx, -1);
    }
    const s3paper::PowerStatus st = s3paper::PowerRead();
    return JS_NewInt32(ctx, st.charging ? 1 : 0);
}

JSValue js_battery_status_text(JSContext *ctx, JSValue *, int, JSValue *) {
    if (!s3paper_runtime::M5BackendState().initialized) {
        return JS_NewString(ctx, "?");
    }
    const s3paper::PowerStatus st = s3paper::PowerRead();
    if (st.battery_level < 0) {
        return JS_NewString(ctx, "?");
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%%s",
             static_cast<int>(st.battery_level),
             st.charging ? " +" : "");
    return JS_NewString(ctx, buf);
}

}  // extern "C"

}  // namespace pulp
