#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "film_catalog.h"

namespace tutorial_0073 {

enum class SelectorField {
  kFilm = 0,
  kDeveloper,
  kDilution,
  kTemperature,
  kPushPull,
  kReview,
};

struct SelectorSnapshot {
  SelectorField field = SelectorField::kFilm;
  size_t option_index = 0;
  size_t option_count = 0;
  RecipeSelection selection{};
  const FilmCatalogEntry *resolved_recipe = nullptr;
};

class RecipeSelectorModel {
 public:
  bool init(const FilmCatalog &catalog);
  void adjust(int delta);
  bool confirm();
  void back();

  SelectorSnapshot snapshot() const;
  const FilmCatalogEntry *resolved_recipe() const;

 private:
  const FilmCatalog *catalog_ = nullptr;

  std::vector<std::string_view> films_{};
  std::vector<std::string_view> developers_{};
  std::vector<std::string_view> dilutions_{};
  std::vector<int16_t> temperatures_{};
  std::vector<std::string_view> push_pulls_{};

  size_t film_index_ = 0;
  size_t developer_index_ = 0;
  size_t dilution_index_ = 0;
  size_t temperature_index_ = 0;
  size_t push_pull_index_ = 0;
  SelectorField field_ = SelectorField::kFilm;

  void rebuild_options();
  void advance_field();
  void retreat_field();
  size_t current_option_count() const;
  bool field_has_multiple_options(SelectorField field) const;
  RecipeSelection current_selection() const;
};

}  // namespace tutorial_0073
