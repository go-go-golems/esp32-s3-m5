#include "screens.h"
#include "node_builder.h"

namespace gnosis {

// ── Shared nav bar builder ──

static Node* BuildNavBar(NodePool& p)
{
    Node* nav = HBox(p, {
        Icon(p, 0, 48),
        Icon(p, 1, 48),
        Icon(p, 2, 48),
        Icon(p, 3, 48),
        Spacer(p),
        Badge(p, "AUTO", 84),
        Spacer(p),
        Label(p, "PIXEL MONOSPACED", 1, 2),
    }, 32);
    if (nav) nav->border_t = true;
    return nav;
}

static Node* BuildStatusBar(NodePool& p, const char* extra = nullptr, int extra_color = 1)
{
    Node* bar;
    if (extra) {
        bar = HBox(p, {
            Label(p, "GNOSIS//3.1"),
            Spacer(p),
            Label(p, extra, 1, extra_color),
            Label(p, "PWR:EINK", 1, 1, 0, 0, 100),
            Dot(p, 24),
        }, 32);
    } else {
        bar = HBox(p, {
            Label(p, "GNOSIS//3.1"),
            Spacer(p),
            Dot(p, 24),
        }, 32);
    }
    if (bar) bar->border_b = true;
    return bar;
}

// ── Dashboard ──

Screen BuildDashboard(NodePool& pool)
{
    Screen s;
    s.bar = BuildStatusBar(pool, "SIG:97%");

    Node* left = Fixed(pool, {
        Label(pool, "CHRONO", 1, 2, 16, 12),
        Label(pool, "14:37", 4, 0, 16, 44),
        Label(pool, "2026.03.22", 1, 1, 16, 120),
        Label(pool, "SEC 42", 1, 2, 16, 148),
        Sep(pool, 0, 192, 480),
        Label(pool, "ORIENTATION", 1, 2, 16, 210),
        Circle(pool, 240, 360, 120),
        Circle(pool, 240, 360, 90),
        Circle(pool, 240, 360, 56),
        Cross(pool, 240, 360, 20),
    });

    Node* log_list = List(pool, 18, 7, 16, 280, 420, 140);
    ListAddRow(log_list, "00:00:01", 80, 2, "kernel init", 200, 0);
    ListAddRow(log_list, "00:00:02", 80, 2, "epd driver ok", 200, 0);
    ListAddRow(log_list, "00:00:02", 80, 2, "mesh scan...", 200, 0);
    ListAddRow(log_list, "00:00:03", 80, 2, "3 nodes found", 200, 0);
    ListAddRow(log_list, "00:00:04", 80, 2, "telemetry open", 200, 0);
    ListAddRow(log_list, "00:00:05", 80, 2, "orient lock", 200, 0);
    ListAddRow(log_list, "00:00:06", 80, 2, "ready", 200, 0);

    Node* right = Fixed(pool, {
        Label(pool, "TELEMETRY", 1, 2, 16, 12),
        Gauge(pool, "R", 15, 360, 16, 44, 420),
        Gauge(pool, "P", 34, 360, 16, 72, 420),
        Gauge(pool, "Y", 127, 360, 16, 100, 420),
        Gauge(pool, "T", 291, 360, 16, 128, 420),
        Gauge(pool, "V", 3, 360, 16, 156, 420),
        Gauge(pool, "A", 188, 360, 16, 184, 420),
        Sep(pool, 0, 224, 460),
        Label(pool, "SYS.LOG", 1, 2, 16, 248),
        log_list,
    });

    s.body = HBoxSplit(pool, 480, left, right);
    s.nav = BuildNavBar(pool);
    return s;
}

// ── Calendar ──

Screen BuildCalendar(NodePool& pool)
{
    Screen s;
    s.bar = BuildStatusBar(pool, "SIG:97%");

    // Event days bitmask: days 3,7,12,15,21,25,28 (0-indexed: 2,6,11,14,20,24,27)
    uint32_t events = (1u << 2) | (1u << 6) | (1u << 11) | (1u << 14) |
                      (1u << 20) | (1u << 24) | (1u << 27);

    Node* left = Fixed(pool, {
        Label(pool, "TEMPORAL MAP", 1, 2, 16, 12),
        Label(pool, "MARCH 2026", 2, 0, 16, 40),
        Grid(pool, 7, 80, 50, 35, 21, events, 16, 90, 544, 260),
    });

    Node* agenda = List(pool, 32, 4, 16, 76, 240, 140);
    ListAddRow(agenda, "09:00", 56, 1, "Team sync", 160, 0);
    ListAddRow(agenda, "11:30", 56, 1, "Design rev", 160, 0);
    ListAddRow(agenda, "14:00", 56, 1, "EPD testing", 160, 0);
    ListAddRow(agenda, "16:30", 56, 1, "Code review", 160, 0);

    Node* upcoming = List(pool, 18, 3, 16, 270, 240, 60);
    ListAddRow(upcoming, "03.24", 56, 2, "Sprint end", 160, 0);
    ListAddRow(upcoming, "03.27", 56, 2, "Demo day", 160, 0);
    ListAddRow(upcoming, "04.01", 56, 2, "Release v4", 160, 0);

    Node* right = Fixed(pool, {
        Label(pool, "AGENDA", 1, 2, 16, 12),
        Label(pool, "2026.03.22", 1, 0, 16, 40),
        agenda,
        Sep(pool, 0, 230, 380),
        Label(pool, "UPCOMING", 1, 2, 16, 242),
        upcoming,
    });

    s.body = HBoxSplit(pool, 580, left, right);
    s.nav = BuildNavBar(pool);
    return s;
}

// ── Boot ──

Screen BuildBoot(NodePool& pool)
{
    Screen s;

    Node* top = HBox(pool, {
        Label(pool, "GNOSIS//BOOT"),
        Spacer(pool),
    }, 32);
    top->border_b = true;

    Node* body = Fixed(pool, {
        Circle(pool, 480, 200, 140),
        Circle(pool, 480, 200, 100),
        Circle(pool, 480, 200, 56),
        Circle(pool, 480, 200, 16),
        Cross(pool, 480, 200, 160),
        Label(pool, "GNOSIS", 4, 0, 380, 360),
        Label(pool, "E-INK OPERATING SYSTEM", 1, 1, 290, 410),
        Bar(pool, 100, 72, 4, 280, 440, 400),
        Label(pool, "72%", 1, 2, 460, 456),
    });

    Node* bot = HBox(pool, {
        Label(pool, "INITIATED:2022", 1, 2),
        Spacer(pool),
        Label(pool, "CLASSIFICATION:PIXEL MONO", 1, 2),
    }, 32);
    bot->border_t = true;

    s.bar = top;
    s.body = body;
    s.nav = bot;
    return s;
}

// ── Widget Gallery ──

Screen BuildWidgetGallery(NodePool& pool)
{
    Screen s;

    Node* top = HBox(pool, {
        Label(pool, "WIDGET GALLERY"),
        Spacer(pool),
        Label(pool, "DEBUG", 1, 2),
    }, 32);
    top->border_b = true;

    Node* gallery_list = List(pool, 20, 5, 440, 280, 320, 110);
    ListAddRow(gallery_list, "List item 0", 200, 0);
    ListAddRow(gallery_list, "Selected item", 200, 0);
    ListAddRow(gallery_list, "List item 2", 200, 0);
    ListAddRow(gallery_list, "List item 3", 200, 0);
    ListAddRow(gallery_list, "List item 4", 200, 0);
    gallery_list->list_selected = 1;

    Node* body = Fixed(pool, {
        Label(pool, "SIZE 1 TEXT", 1, 0, 16, 16),
        Label(pool, "SIZE 2", 2, 0, 16, 36),
        Label(pool, "BIG", 4, 0, 16, 68),
        Sep(pool, 16, 120, 380),
        Gauge(pool, "A", 42, 100, 16, 136, 380),
        Gauge(pool, "B", 88, 100, 16, 164, 380),
        Gauge(pool, "C", 7, 100, 16, 192, 380),
        Sep(pool, 16, 220, 380),
        Bar(pool, 100, 65, 6, 16, 236, 380),
        Bar(pool, 100, 30, 6, 16, 252, 380),
        Sep(pool, 16, 276, 380),
        Circle(pool, 120, 360, 60),
        Cross(pool, 120, 360, 24),
        Circle(pool, 300, 360, 40),
        Circle(pool, 300, 360, 60),
        Circle(pool, 300, 360, 80),
        Label(pool, "ICONS:", 1, 2, 440, 136),
        Badge(pool, "BADGE", 0),
        // position badge manually via offset
        Label(pool, "cursor>", 1, 2, 440, 196),
        gallery_list,
    });
    // Manually fix badge offset
    if (body->n_children >= 18) {
        body->children[17]->offset_x = 440;
        body->children[17]->offset_y = 164;
    }

    Node* bot = HBox(pool, {
        Icon(pool, 0, 48),
        Icon(pool, 1, 48),
        Icon(pool, 2, 48),
        Icon(pool, 3, 48),
        Spacer(pool),
        Badge(pool, "DBG", 60),
    }, 32);
    bot->border_t = true;

    s.bar = top;
    s.body = body;
    s.nav = bot;
    return s;
}

// ── Telemetry ──

Screen BuildTelemetry(NodePool& pool)
{
    Screen s;
    s.bar = BuildStatusBar(pool, "TELEMETRY FULL");

    Node* left = Fixed(pool, {
        Label(pool, "PRIMARY", 1, 2, 16, 12),
        Gauge(pool, "R", 15, 360, 16, 44, 420),
        Gauge(pool, "P", 34, 360, 16, 76, 420),
        Gauge(pool, "Y", 127, 360, 16, 108, 420),
        Gauge(pool, "T", 291, 360, 16, 140, 420),
        Gauge(pool, "V", 3, 360, 16, 172, 420),
        Gauge(pool, "A", 188, 360, 16, 204, 420),
        Sep(pool, 0, 248, 460),
        Label(pool, "SECONDARY", 1, 2, 16, 264),
        Gauge(pool, "X", 44, 100, 16, 292, 420),
        Gauge(pool, "Z", 77, 100, 16, 324, 420),
        Gauge(pool, "W", 12, 100, 16, 356, 420),
        Gauge(pool, "Q", 95, 100, 16, 388, 420),
    });

    Node* right = Fixed(pool, {
        Label(pool, "POSITION", 1, 2, 16, 12),
        Circle(pool, 200, 190, 140),
        Circle(pool, 200, 190, 100),
        Circle(pool, 200, 190, 56),
        Cross(pool, 200, 190, 24),
        Label(pool, "000", 1, 1, 180, 36),
        Label(pool, "090", 1, 1, 344, 186),
        Label(pool, "180", 1, 1, 180, 338),
        Label(pool, "270", 1, 1, 20, 186),
        Sep(pool, 0, 370, 440),
        Label(pool, "STATUS", 1, 2, 16, 386),
        Label(pool, "MESH: ACTIVE", 1, 0, 16, 410),
        Label(pool, "NODES: 3/8", 1, 0, 16, 434),
    });

    s.body = HBoxSplit(pool, 480, left, right);
    s.nav = BuildNavBar(pool);
    return s;
}

// ── Reader ──

Screen BuildReader(NodePool& pool)
{
    Screen s;
    s.bar = BuildStatusBar(pool, "PWR:EINK");

    Node* sidebar_list = List(pool, 34, 7, 8, 70, 280, 240);
    ListAddRow(sidebar_list, "Neuromancer", 200, 0);
    ListAddRow(sidebar_list, "Gibson  72%", 200, 2);
    ListAddRow(sidebar_list, "Snow Crash", 200, 0);
    ListAddRow(sidebar_list, "Stephenson  45%", 200, 2);
    ListAddRow(sidebar_list, "Dune", 200, 0);
    ListAddRow(sidebar_list, "Herbert  100%", 200, 2);
    ListAddRow(sidebar_list, "Solaris", 200, 0);
    sidebar_list->list_selected = 2;

    Node* left = Fixed(pool, {
        Label(pool, "LIBRARY", 1, 2, 16, 12),
        Label(pool, "7 VOL", 1, 1, 16, 36),
        sidebar_list,
    });

    Node* right = Fixed(pool, {
        Label(pool, "READER", 1, 2, 16, 12),
        Label(pool, "SNOW CRASH", 2, 0, 16, 40),
        Label(pool, "Neal Stephenson", 1, 1, 16, 76),
        Label(pool, "CH 12", 1, 2, 380, 76),
        Sep(pool, 0, 100, 520),
        TextBlock(pool,
            "The Deliverator belongs\n"
            "to an elite order, a\n"
            "hallowed sub-category.\n"
            "He is a pizza delivery\n"
            "driver.",
            16, 16, 114, 480, 200),
        Sep(pool, 0, 400, 520),
        Label(pool, "P.187/440", 1, 2, 16, 416),
        Bar(pool, 100, 45, 3, 144, 422, 200),
        Label(pool, "BM NT", 1, 1, 380, 416),
    });

    s.body = HBoxSplit(pool, 300, left, right);
    s.nav = BuildNavBar(pool);
    return s;
}

// ── Minimal ──

Screen BuildMinimal(NodePool& pool)
{
    Screen s;
    s.bar = BuildStatusBar(pool);

    s.body = Fixed(pool, {
        Circle(pool, 480, 240, 160),
        Circle(pool, 480, 240, 110),
        Cross(pool, 480, 240, 32),
        Label(pool, "STANDBY", 1, 2, 420, 420),
    });

    s.nav = BuildNavBar(pool);
    return s;
}

// ── Preset registry ──

const PresetEntry kPresets[] = {
    {"dashboard",  BuildDashboard},
    {"calendar",   BuildCalendar},
    {"boot",       BuildBoot},
    {"gallery",    BuildWidgetGallery},
    {"telemetry",  BuildTelemetry},
    {"reader",     BuildReader},
    {"minimal",    BuildMinimal},
};

const int kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

}  // namespace gnosis
