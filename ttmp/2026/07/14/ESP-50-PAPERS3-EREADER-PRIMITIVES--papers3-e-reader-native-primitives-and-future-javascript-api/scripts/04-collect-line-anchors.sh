#!/usr/bin/env bash
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"
TICKET_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEMO="$(realpath "$ROOT/../M5PaperS3-UserDemo")"

printf '%s\n' '== Imported JS design anchors =='
rg -n '^## |refreshPolicy|region\(|book\(|paginate|Layer \|' \
  "$TICKET_DIR/sources/local/s3paper-api-design.md"

printf '%s\n' '== Studio pipeline and runtime anchors =='
rg -n 'Pipeline|function layoutWidget|function layoutText|function layoutRow|function layoutCol|function layoutList|function layoutBook|function layoutRegion|function layoutPage|function paintOps|defaultPolicy|function createEngine|requestRender|mountPage|dispatch|function makeEnv|const book|const region|const page|const paper|const nav|const PRESETS' \
  "$TICKET_DIR/sources/local/s3paper-studio.jsx"

printf '%s\n' '== Existing reader app anchors =='
rg -n 'InitBoard|MountStorage|BuildReadingScreen|BuildLibraryScreen|ComputeTotalPages|LoadCurrentPage|FullRefresh|HandleTouch|SwitchScreen|ProcessDirtyRefresh|NextPage|PreviousPage|OpenBook|RunLoop|kFullRefresh|kCharsPerLine|kLinesPerPage' \
  "$ROOT/0080-papers3-ereader/main/ereader_app.cpp"

printf '%s\n' '== Existing reader subsystem anchors =='
rg -n 'Mount\(|LoadIndex|SaveIndex|ReadChunk|FileSize|EnsurePage|PaginateOnePage|FormatText|GetPageText|UpdatePosition|Save\(|DirtyCollector::Collect|DirtyCollector::Merge|LayoutScreen|LayoutNode|RenderSubtree|DrawTextBlock|ext_text|kMaxNodes|kMaxDirtyRects' \
  "$ROOT"/0080-papers3-ereader/main/{book_store.cpp,paginator.cpp,bookmark_store.cpp,dirty_tracker.cpp,layout_engine.cpp,widget_renderer.cpp,gnosis_types.h}

printf '%s\n' '== Factory demo HAL and refresh anchors =='
rg -n 'M5.begin|setRotation|powerOff|sd_card_init|wifi_init|isTouchPressed|wasTouchClickedArea|draw_gray_scale_bars|boot_display_test|check_full_display_refresh_request|setEpdMode|requestRefresh' \
  "$DEMO"/main/{hal/hal.cpp,hal/hal.h,main.cpp}

printf '%s\n' '== Existing queued canvas ABI anchors =='
rg -n 'NormalizePresentMode|MapPresentMode|BeginFrameIfNeeded|ClampRect|PaperCanvas|kMaxQueuedCommands|QueueHostCommand|FlushWasmHostFrame' \
  "$ROOT"/0079-papers3-wamr-assemblyscript-console/main/{papers3_canvas.cpp,wasm_host_api.cpp}
