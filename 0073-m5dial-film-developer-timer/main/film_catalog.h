#pragma once

#include <cstddef>
#include <cstdint>

namespace tutorial_0073 {

struct FilmCatalogEntry {
  const char *film;
  const char *developer;
  const char *dilution;
  int16_t temperature_tenths_c;
  const char *push_pull_type;
  const char *push_pull_display;
  int16_t push_pull_stops_hundredths;
  uint16_t time_seconds;
  bool has_time_35mm;
  bool has_time_120;
  bool has_time_sheet;
  uint16_t source_count;
  const char *film_category;
};

struct FilmCatalogStats {
  size_t recipe_count = 0;
  size_t film_count = 0;
  size_t developer_count = 0;
};

class FilmCatalog {
 public:
  bool init();

  const FilmCatalogEntry *entries() const { return entries_; }
  size_t recipe_count() const { return stats_.recipe_count; }
  const FilmCatalogStats &stats() const { return stats_; }

 private:
  const FilmCatalogEntry *entries_ = nullptr;
  FilmCatalogStats stats_{};
};

}  // namespace tutorial_0073
