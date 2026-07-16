// Widget class + factories: the v2 builder surface over the retained
// s3paper widget tree. Opaque = packed WidgetHandle; methods chain by
// returning *this_val; staleness throws TypeError.
#include <cstring>

#include "esp_log.h"

#include "app_js_internal.h"
#include "s3paper/text.h"
#include "s3paper/widget.h"
#include "s3paper_runtime/runtime.h"

namespace pulp {

using namespace jsi;

namespace {
const char *kTag = "js";
}  // namespace

extern "C" {

// ---- factories ----

JSValue js_pulp_text(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "text(value|fn)");
    }
    int32_t dyn_cb = 0;
    char text[s3paper::TextProps::kCapacity] = {};
    if (JS_IsFunction(ctx, argv[0])) {
        if (g_dyn_count >= kMaxDynValues) {
            return JS_ThrowTypeError(ctx, "dynamic value table full");
        }
        dyn_cb = RegisterCb(ctx, argv[0]);
        if (dyn_cb == 0) {
            return JS_EXCEPTION;
        }
        const JSValue v = CallCb(dyn_cb, 0, 0, 0, 0);
        if (!JS_IsUndefined(v)) {
            JSCStringBuf buf;
            size_t len;
            const char *str = JS_ToCStringLen(ctx, &len, v, &buf);
            if (str != nullptr) {
                snprintf(text, sizeof(text), "%.*s", static_cast<int>(len),
                         str);
            }
        }
    } else {
        JSValue err;
        if (!ArgString(ctx, argv[0], text, sizeof(text), &err)) {
            return err;
        }
    }
    const s3paper::Result<s3paper::WidgetHandle> r = s3paper::NewText(
        s3paper_runtime::Arena(), text, s3paper::kFontBody, 0);
    if (r.ok() && dyn_cb != 0) {
        g_dyn[g_dyn_count++] = DynEntry{r.value, dyn_cb};
    }
    ESP_LOGD(kTag, "text[%u.%u] \"%s\"", r.ok() ? r.value.index : 9999,
             r.ok() ? r.value.generation : 0, text);
    return MakeWidget(ctx, r);
}

JSValue js_pulp_row(JSContext *ctx, JSValue *, int, JSValue *) {
    return MakeWidget(ctx, s3paper::NewRow(s3paper_runtime::Arena()));
}

JSValue js_pulp_col(JSContext *ctx, JSValue *, int, JSValue *) {
    return MakeWidget(ctx, s3paper::NewCol(s3paper_runtime::Arena()));
}

JSValue js_pulp_spacer(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int fixed = 0;
    int flex = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &fixed, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &flex, argv[1])) return JS_EXCEPTION;
    return MakeWidget(
        ctx, s3paper::NewSpacer(s3paper_runtime::Arena(), fixed,
                                static_cast<uint16_t>(flex < 0 ? 0 : flex)));
}

JSValue js_pulp_divider(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int thickness = 1;
    int gray = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &thickness, argv[0]))
        return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &gray, argv[1])) return JS_EXCEPTION;
    return MakeWidget(
        ctx, s3paper::NewDivider(s3paper_runtime::Arena(), thickness,
                                 static_cast<s3paper::Gray8>(gray)));
}

JSValue js_pulp_progress_bar(JSContext *ctx, JSValue *, int argc,
                             JSValue *argv) {
    int permille = 0;
    int height = 12;
    if (argc >= 1 && JS_ToInt32(ctx, &permille, argv[0]))
        return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &height, argv[1])) return JS_EXCEPTION;
    return MakeWidget(
        ctx, s3paper::NewProgress(
                 s3paper_runtime::Arena(),
                 static_cast<uint16_t>(permille < 0 ? 0 : permille), height,
                 0));
}

JSValue js_pulp_list(JSContext *ctx, JSValue *, int, JSValue *) {
    return MakeWidget(ctx, s3paper::NewList(s3paper_runtime::Arena()));
}

JSValue js_pulp_region(JSContext *ctx, JSValue *, int argc, JSValue *argv) {
    int id = 0;
    int interval = 0;
    int quiet = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &id, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &interval, argv[1]))
        return JS_EXCEPTION;
    if (argc >= 3 && JS_ToInt32(ctx, &quiet, argv[2])) return JS_EXCEPTION;
    return MakeWidget(
        ctx, s3paper::NewRegion(s3paper_runtime::Arena(),
                                static_cast<uint32_t>(id),
                                static_cast<uint32_t>(interval),
                                quiet != 0));
}

// ---- Widget class ----

JSValue js_widget_ctor(JSContext *ctx, JSValue *, int, JSValue *) {
    return JS_ThrowTypeError(ctx,
                             "use factories: text(), row(), col(), ...");
}

void js_widget_finalizer(JSContext *, void *) {
    // No-op by design: the retained tree owns nodes; wrappers are views.
}

JSValue js_w_pad(JSContext *ctx, JSValue *this_val, int argc,
                 JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int t = 0, r = 0, b = 0, l = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &t, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &r, argv[1])) return JS_EXCEPTION;
    if (argc >= 3 && JS_ToInt32(ctx, &b, argv[2])) return JS_EXCEPTION;
    if (argc >= 4 && JS_ToInt32(ctx, &l, argv[3])) return JS_EXCEPTION;
    if (argc == 1) {  // pad(all)
        r = b = l = t;
    } else if (argc == 2) {  // pad(v, h)
        b = t;
        l = r;
    }
    n->padding = s3paper::Insets{t, r, b, l};
    return *this_val;
}

JSValue js_w_gap(JSContext *ctx, JSValue *this_val, int argc,
                 JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int g = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &g, argv[0])) return JS_EXCEPTION;
    n->gap = g;
    return *this_val;
}

JSValue js_w_main_align(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int a = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &a, argv[0])) return JS_EXCEPTION;
    if (a < 0 || a > 3) return JS_ThrowTypeError(ctx, "bad align");
    n->main_align = static_cast<s3paper::MainAlign>(a);
    return *this_val;
}

JSValue js_w_cross_align(JSContext *ctx, JSValue *this_val, int argc,
                         JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int a = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &a, argv[0])) return JS_EXCEPTION;
    if (a < 0 || a > 3) return JS_ThrowTypeError(ctx, "bad align");
    n->cross_align = static_cast<s3paper::CrossAlign>(a);
    return *this_val;
}

JSValue js_w_width(JSContext *ctx, JSValue *this_val, int argc,
                   JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int v = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &v, argv[0])) return JS_EXCEPTION;
    n->fixed_w = v;
    return *this_val;
}

JSValue js_w_height(JSContext *ctx, JSValue *this_val, int argc,
                    JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int v = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &v, argv[0])) return JS_EXCEPTION;
    n->fixed_h = v;
    return *this_val;
}

JSValue js_w_flex(JSContext *ctx, JSValue *this_val, int argc,
                  JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int f = 1;
    if (argc >= 1 && JS_ToInt32(ctx, &f, argv[0])) return JS_EXCEPTION;
    n->flex = static_cast<uint16_t>(f < 0 ? 0 : f);
    return *this_val;
}

JSValue js_w_font(JSContext *ctx, JSValue *this_val, int argc,
                  JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (n->kind != s3paper::WidgetKind::Text) {
        return JS_ThrowTypeError(ctx, "font: not a Text");
    }
    int f = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &f, argv[0])) return JS_EXCEPTION;
    if (f < 0 || f >= s3paper::kFontCount) {
        return JS_ThrowTypeError(ctx, "bad font id");
    }
    n->props.text.font_id = static_cast<uint8_t>(f);
    return *this_val;
}

// size(token): 'xs'/'sm' -> ui, 'md' -> body, 'lg' -> display, 'xl' -> XL.
JSValue js_w_size(JSContext *ctx, JSValue *this_val, int argc,
                  JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (n->kind != s3paper::WidgetKind::Text) {
        return JS_ThrowTypeError(ctx, "size: not a Text");
    }
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "size(token)");
    }
    int font = -1;
    if (JS_IsString(ctx, argv[0])) {
        char token[8];
        if (!ArgString(ctx, argv[0], token, sizeof(token), &err)) {
            return err;
        }
        if (strcmp(token, "xs") == 0 || strcmp(token, "sm") == 0) {
            font = s3paper::kFontUi;
        } else if (strcmp(token, "md") == 0) {
            font = s3paper::kFontBody;
        } else if (strcmp(token, "lg") == 0) {
            font = s3paper::kFontDisplay;
        } else if (strcmp(token, "xl") == 0) {
            font = s3paper::kFontXL;
        } else if (strcmp(token, "title") == 0) {
            font = s3paper::kFontTitle;  // serif at display size
        }
    } else if (JS_ToInt32(ctx, &font, argv[0])) {
        return JS_EXCEPTION;
    }
    if (font < 0 || font >= s3paper::kFontCount) {
        return JS_ThrowTypeError(ctx, "bad size token");
    }
    n->props.text.font_id = static_cast<uint8_t>(font);
    return *this_val;
}

JSValue js_w_gray(JSContext *ctx, JSValue *this_val, int argc,
                  JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (n->kind != s3paper::WidgetKind::Text) {
        return JS_ThrowTypeError(ctx, "gray: not a Text");
    }
    int g = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &g, argv[0])) return JS_EXCEPTION;
    n->props.text.gray = static_cast<s3paper::Gray8>(g);
    return *this_val;
}

JSValue js_w_center(JSContext *ctx, JSValue *this_val, int, JSValue *) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (n->kind != s3paper::WidgetKind::Text) {
        return JS_ThrowTypeError(ctx, "center: not a Text");
    }
    n->props.text.align = s3paper::TextAlign::Center;
    return *this_val;
}

JSValue js_w_align(JSContext *ctx, JSValue *this_val, int argc,
                   JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (n->kind != s3paper::WidgetKind::Text) {
        return JS_ThrowTypeError(ctx, "align: not a Text");
    }
    int a = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &a, argv[0])) return JS_EXCEPTION;
    if (a < 0 || a > 2) return JS_ThrowTypeError(ctx, "bad align");
    n->props.text.align = static_cast<s3paper::TextAlign>(a);
    return *this_val;
}

JSValue js_w_invert(JSContext *ctx, JSValue *this_val, int, JSValue *) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (n->kind != s3paper::WidgetKind::Text) {
        return JS_ThrowTypeError(ctx, "invert: not a Text");
    }
    n->props.text.invert = 1;
    return *this_val;
}

JSValue js_w_dep(JSContext *ctx, JSValue *this_val, int argc,
                 JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int d = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &d, argv[0])) return JS_EXCEPTION;
    n->dependency = static_cast<uint32_t>(d);
    return *this_val;
}

JSValue js_w_hit(JSContext *ctx, JSValue *this_val, int argc,
                 JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    int id = 0, z = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &id, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &z, argv[1])) return JS_EXCEPTION;
    n->hit_id = static_cast<uint32_t>(id);
    n->hit_z = static_cast<int16_t>(z);
    return *this_val;
}

JSValue js_w_add(JSContext *ctx, JSValue *this_val, int argc,
                 JSValue *argv) {
    JSValue err;
    s3paper::WidgetHandle parent;
    if (ThisNode(ctx, this_val, &parent, &err) == nullptr) return err;
    for (int i = 0; i < argc; ++i) {
        if (JS_GetClassID(ctx, argv[i]) != JS_CLASS_WIDGET) {
            return JS_ThrowTypeError(ctx, "add: expecting Widget");
        }
        const s3paper::WidgetHandle child =
            UnpackWidget(JS_GetOpaque(ctx, argv[i]));
        ESP_LOGD(kTag, "add parent[%u.%u] child[%u.%u]", parent.index,
                 parent.generation, child.index, child.generation);
        const s3paper::Status st =
            s3paper_runtime::Arena().AddChild(parent, child);
        if (!st.ok()) {
            return JS_ThrowTypeError(ctx, "AddChild failed: %s",
                                     s3paper::StatusCodeName(st.code));
        }
    }
    return *this_val;
}

JSValue js_w_set(JSContext *ctx, JSValue *this_val, int argc,
                 JSValue *argv) {
    JSValue err;
    s3paper::WidgetHandle h;
    if (ThisNode(ctx, this_val, &h, &err) == nullptr) return err;
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "set(value)");
    }
    char text[s3paper::TextProps::kCapacity];
    if (!ArgString(ctx, argv[0], text, sizeof(text), &err)) {
        return err;
    }
    const s3paper::Status st = s3paper_runtime::Arena().SetText(h, text);
    if (!st.ok()) {
        return JS_ThrowTypeError(ctx, "SetText failed: %s",
                                 s3paper::StatusCodeName(st.code));
    }
    return *this_val;
}

JSValue js_w_progress(JSContext *ctx, JSValue *this_val, int argc,
                      JSValue *argv) {
    JSValue err;
    s3paper::WidgetHandle h;
    if (ThisNode(ctx, this_val, &h, &err) == nullptr) return err;
    int permille = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &permille, argv[0]))
        return JS_EXCEPTION;
    const s3paper::Status st = s3paper_runtime::Arena().SetProgress(
        h, static_cast<uint16_t>(permille < 0 ? 0 : permille));
    if (!st.ok()) {
        return JS_ThrowTypeError(ctx, "SetProgress failed: %s",
                                 s3paper::StatusCodeName(st.code));
    }
    return *this_val;
}

// onTap(fn): the callback id IS the hit id; dispatch calls __cbs[id]
// directly (no facade, no eval-string parse).
JSValue js_w_on_tap(JSContext *ctx, JSValue *this_val, int argc,
                    JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
        return JS_ThrowTypeError(ctx, "onTap(fn)");
    }
    const int32_t id = RegisterCb(ctx, argv[0]);
    if (id == 0) {
        return JS_EXCEPTION;
    }
    n->hit_id = static_cast<uint32_t>(id);
    return *this_val;
}

// every(ms, quiet?) on a Region widget: interval configuration.
JSValue js_w_every(JSContext *ctx, JSValue *this_val, int argc,
                   JSValue *argv) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (n->kind != s3paper::WidgetKind::Region) {
        return JS_ThrowTypeError(ctx, "every: not a Region");
    }
    int ms = 0;
    int quiet = 0;
    if (argc >= 1 && JS_ToInt32(ctx, &ms, argv[0])) return JS_EXCEPTION;
    if (argc >= 2 && JS_ToInt32(ctx, &quiet, argv[1])) return JS_EXCEPTION;
    n->props.region.interval_ms = static_cast<uint32_t>(ms < 0 ? 0 : ms);
    n->props.region.quiet_while_active = quiet != 0;
    return *this_val;
}

JSValue js_w_quiet(JSContext *ctx, JSValue *this_val, int, JSValue *) {
    JSValue err;
    s3paper::WidgetNode *n = ThisNode(ctx, this_val, nullptr, &err);
    if (n == nullptr) return err;
    if (n->kind != s3paper::WidgetKind::Region) {
        return JS_ThrowTypeError(ctx, "quiet: not a Region");
    }
    n->props.region.quiet_while_active = true;
    return *this_val;
}

}  // extern "C"

}  // namespace pulp
