// Storage shim: 0112's owner-facing storage API, now backed by the shared
// s3paper_storage component (ESP-51 Phase 2). The reader:: names are
// unchanged so call sites stay stable; this header re-exports the
// component API and keeps three firmware-specific adapters: configuration
// (display-before-mount hook + demo-book seed), the SdSnapshot fill, and
// the legacy StorageWriteDemoBook name.
#pragma once

#include "app_events.h"
#include "s3paper_storage/storage.h"

namespace reader {

using s3paper_storage::kMaxBooks;
using s3paper_storage::BookEntry;
using s3paper_storage::SdContentSource;
using s3paper_storage::StorageStats;

// Configures the component on first use (EnsureM5Init pre-mount hook,
// embedded demo book as the seed), then delegates.
StatusCode StorageMount();
StatusCode StorageWriteDemoBook();
void FillSdSnapshot(SdSnapshot *out);

using s3paper_storage::StorageUnmount;
using s3paper_storage::StorageMounted;
using s3paper_storage::LibraryScan;
using s3paper_storage::LibraryCount;
using s3paper_storage::LibraryGet;
using s3paper_storage::LibraryPrint;
using s3paper_storage::PositionsLoad;
using s3paper_storage::PositionsSave;
using s3paper_storage::PositionLookup;
using s3paper_storage::PositionStore;
using s3paper_storage::StorageFlushIfDue;
using s3paper_storage::StorageFlushNow;
using s3paper_storage::LastBookStore;
using s3paper_storage::LastBookGet;
using s3paper_storage::SettingsLoad;
using s3paper_storage::SettingsGet;
using s3paper_storage::SettingsSet;
using s3paper_storage::BookmarksLoad;
using s3paper_storage::BookmarkToggle;
using s3paper_storage::BookmarkIsSet;
using s3paper_storage::BookmarkCountFor;
using s3paper_storage::BookmarkGet;
using s3paper_storage::BookmarksPrint;

}  // namespace reader
