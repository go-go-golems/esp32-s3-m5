#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

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

struct RecipeSelection {
  std::string_view film;
  std::string_view developer;
  std::string_view dilution;
  int16_t temperature_tenths_c = 0;
  std::string_view push_pull_type;
};

class FilmCatalog {
 public:
  bool init();

  const FilmCatalogEntry *entries() const { return entries_; }
  size_t recipe_count() const { return stats_.recipe_count; }
  const FilmCatalogStats &stats() const { return stats_; }

  std::vector<std::string_view> films() const;
  std::vector<std::string_view> developers_for(std::string_view film) const;
  std::vector<std::string_view> dilutions_for(std::string_view film, std::string_view developer) const;
  std::vector<int16_t> temperatures_for(std::string_view film, std::string_view developer, std::string_view dilution) const;
  std::vector<std::string_view> push_pull_for(std::string_view film,
                                              std::string_view developer,
                                              std::string_view dilution,
                                              int16_t temperature_tenths_c) const;
  const FilmCatalogEntry *resolve(const RecipeSelection &selection) const;

 private:
  const FilmCatalogEntry *entries_ = nullptr;
  FilmCatalogStats stats_{};
};

}  // namespace tutorial_0073
