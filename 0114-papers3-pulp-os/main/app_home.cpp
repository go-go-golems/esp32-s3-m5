#include "app_home.h"

#include <cstdio>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "s3paper/text.h"
#include "s3paper/widget.h"
#include "s3paper_runtime/runtime.h"

namespace pulp {
namespace {

const char *kTag = "home";

}  // namespace

StatusCode HomeShowNative() {
    s3paper::WidgetArena &a = s3paper_runtime::Arena();
    a.Reset();

    // Swiss chrome: XL wordmark, heavy rule, status lines flush left.
    s3paper::WidgetHandle header = s3paper::NewCol(a).value;
    s3paper::WidgetNode *hn = a.Configure(header);
    hn->padding = s3paper::Insets{48, 40, 8, 40};
    hn->gap = 12;
    (void)a.AddChild(header,
                     s3paper::NewText(a, "PULP", s3paper::kFontXL, 0).value);
    (void)a.AddChild(header,
                     s3paper::NewText(a, "THE PAPERBACK OF COMPUTERS",
                                      s3paper::kFontUi, 0)
                         .value);
    (void)a.AddChild(header, s3paper::NewDivider(a, 6, 0).value);

    s3paper::WidgetHandle content = s3paper::NewCol(a).value;
    s3paper::WidgetNode *cn = a.Configure(content);
    cn->padding = s3paper::Insets{32, 40, 24, 40};
    cn->gap = 18;
    (void)a.AddChild(content,
                     s3paper::NewText(a, "OS v2 skeleton",
                                      s3paper::kFontDisplay, 0)
                         .value);
    char line[s3paper::TextProps::kCapacity];
    snprintf(line, sizeof(line), "heap %u KB / psram %u KB",
             static_cast<unsigned>(
                 heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
             static_cast<unsigned>(
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    (void)a.AddChild(content,
                     s3paper::NewText(a, line, s3paper::kFontBody, 0).value);
    (void)a.AddChild(content,
                     s3paper::NewText(a, "JS launcher arrives in Phase 5.",
                                      s3paper::kFontBody, 96)
                         .value);

    s3paper::WidgetHandle footer = s3paper::NewCol(a).value;
    s3paper::WidgetNode *fn = a.Configure(footer);
    fn->padding = s3paper::Insets{6, 40, 12, 40};
    fn->gap = 8;
    (void)a.AddChild(footer, s3paper::NewDivider(a, 2, 0).value);
    (void)a.AddChild(footer,
                     s3paper::NewText(a, "native shell / s3paper_runtime",
                                      s3paper::kFontUi, 96)
                         .value);

    const s3paper::PageSlots slots{header, content, footer,
                                   s3paper::kNullWidget};
    const s3paper_runtime::PresentPageResult presented =
        s3paper_runtime::PresentPage(slots,
                                     s3paper::PresentIntent::CleanFull,
                                     true, nullptr, 0, nullptr);
    if (presented.status == StatusCode::Ok) {
        printf("pulp screen: home-native\n");
        ESP_LOGI(kTag, "home presented (full=%d)",
                 presented.full_refresh ? 1 : 0);
    }
    return presented.status;
}

}  // namespace pulp
