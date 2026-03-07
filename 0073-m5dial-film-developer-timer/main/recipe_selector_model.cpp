#include "recipe_selector_model.h"

namespace tutorial_0073 {

namespace {

size_t wrap_index(size_t current, int delta, size_t count) {
  if (count == 0) {
    return 0;
  }
  const int64_t base = static_cast<int64_t>(current);
  const int64_t size = static_cast<int64_t>(count);
  int64_t next = (base + delta) % size;
  if (next < 0) {
    next += size;
  }
  return static_cast<size_t>(next);
}

}  // namespace

bool RecipeSelectorModel::init(const FilmCatalog &catalog) {
  catalog_ = &catalog;
  films_ = catalog.films();
  if (films_.empty()) {
    return false;
  }

  film_index_ = 0;
  developer_index_ = 0;
  dilution_index_ = 0;
  temperature_index_ = 0;
  push_pull_index_ = 0;
  field_ = SelectorField::kFilm;
  rebuild_options();
  advance_field();
  return true;
}

void RecipeSelectorModel::adjust(int delta) {
  if (delta == 0) {
    return;
  }

  switch (field_) {
    case SelectorField::kFilm:
      film_index_ = wrap_index(film_index_, delta, films_.size());
      developer_index_ = 0;
      dilution_index_ = 0;
      temperature_index_ = 0;
      push_pull_index_ = 0;
      rebuild_options();
      break;
    case SelectorField::kDeveloper:
      developer_index_ = wrap_index(developer_index_, delta, developers_.size());
      dilution_index_ = 0;
      temperature_index_ = 0;
      push_pull_index_ = 0;
      rebuild_options();
      break;
    case SelectorField::kDilution:
      dilution_index_ = wrap_index(dilution_index_, delta, dilutions_.size());
      temperature_index_ = 0;
      push_pull_index_ = 0;
      rebuild_options();
      break;
    case SelectorField::kTemperature:
      temperature_index_ = wrap_index(temperature_index_, delta, temperatures_.size());
      push_pull_index_ = 0;
      rebuild_options();
      break;
    case SelectorField::kPushPull:
      push_pull_index_ = wrap_index(push_pull_index_, delta, push_pulls_.size());
      break;
    case SelectorField::kReview:
      break;
  }
}

bool RecipeSelectorModel::confirm() {
  if (field_ == SelectorField::kReview) {
    return true;
  }
  advance_field();
  return field_ == SelectorField::kReview;
}

void RecipeSelectorModel::back() {
  retreat_field();
}

SelectorSnapshot RecipeSelectorModel::snapshot() const {
  SelectorSnapshot out;
  out.field = field_;
  out.option_count = current_option_count();
  switch (field_) {
    case SelectorField::kFilm:
      out.option_index = film_index_;
      break;
    case SelectorField::kDeveloper:
      out.option_index = developer_index_;
      break;
    case SelectorField::kDilution:
      out.option_index = dilution_index_;
      break;
    case SelectorField::kTemperature:
      out.option_index = temperature_index_;
      break;
    case SelectorField::kPushPull:
      out.option_index = push_pull_index_;
      break;
    case SelectorField::kReview:
      out.option_index = 0;
      break;
  }
  out.selection = current_selection();
  out.resolved_recipe = resolved_recipe();
  return out;
}

const FilmCatalogEntry *RecipeSelectorModel::resolved_recipe() const {
  if (!catalog_) {
    return nullptr;
  }
  return catalog_->resolve(current_selection());
}

void RecipeSelectorModel::rebuild_options() {
  if (!catalog_ || films_.empty()) {
    return;
  }

  const std::string_view film = films_[film_index_];
  developers_ = catalog_->developers_for(film);
  if (developers_.empty()) {
    return;
  }
  if (developer_index_ >= developers_.size()) {
    developer_index_ = 0;
  }

  const std::string_view developer = developers_[developer_index_];
  dilutions_ = catalog_->dilutions_for(film, developer);
  if (dilution_index_ >= dilutions_.size()) {
    dilution_index_ = 0;
  }

  const std::string_view dilution = dilutions_.empty() ? std::string_view{} : dilutions_[dilution_index_];
  temperatures_ = catalog_->temperatures_for(film, developer, dilution);
  if (temperature_index_ >= temperatures_.size()) {
    temperature_index_ = 0;
  }

  const int16_t temperature = temperatures_.empty() ? 0 : temperatures_[temperature_index_];
  push_pulls_ = catalog_->push_pull_for(film, developer, dilution, temperature);
  if (push_pull_index_ >= push_pulls_.size()) {
    push_pull_index_ = 0;
  }
}

void RecipeSelectorModel::advance_field() {
  SelectorField candidate = field_;
  do {
    switch (candidate) {
      case SelectorField::kFilm:
        candidate = SelectorField::kDeveloper;
        break;
      case SelectorField::kDeveloper:
        candidate = SelectorField::kDilution;
        break;
      case SelectorField::kDilution:
        candidate = SelectorField::kTemperature;
        break;
      case SelectorField::kTemperature:
        candidate = SelectorField::kPushPull;
        break;
      case SelectorField::kPushPull:
        candidate = SelectorField::kReview;
        break;
      case SelectorField::kReview:
        return;
    }
  } while (candidate != SelectorField::kReview && !field_has_multiple_options(candidate));
  field_ = candidate;
}

void RecipeSelectorModel::retreat_field() {
  if (field_ == SelectorField::kFilm) {
    return;
  }

  SelectorField candidate = field_;
  do {
    switch (candidate) {
      case SelectorField::kReview:
        candidate = SelectorField::kPushPull;
        break;
      case SelectorField::kPushPull:
        candidate = SelectorField::kTemperature;
        break;
      case SelectorField::kTemperature:
        candidate = SelectorField::kDilution;
        break;
      case SelectorField::kDilution:
        candidate = SelectorField::kDeveloper;
        break;
      case SelectorField::kDeveloper:
      case SelectorField::kFilm:
        candidate = SelectorField::kFilm;
        break;
    }
  } while (candidate != SelectorField::kFilm && !field_has_multiple_options(candidate));
  field_ = candidate;
}

size_t RecipeSelectorModel::current_option_count() const {
  switch (field_) {
    case SelectorField::kFilm:
      return films_.size();
    case SelectorField::kDeveloper:
      return developers_.size();
    case SelectorField::kDilution:
      return dilutions_.size();
    case SelectorField::kTemperature:
      return temperatures_.size();
    case SelectorField::kPushPull:
      return push_pulls_.size();
    case SelectorField::kReview:
      return 1;
  }
  return 0;
}

bool RecipeSelectorModel::field_has_multiple_options(SelectorField field) const {
  switch (field) {
    case SelectorField::kFilm:
      return films_.size() > 1;
    case SelectorField::kDeveloper:
      return developers_.size() > 1;
    case SelectorField::kDilution:
      return dilutions_.size() > 1;
    case SelectorField::kTemperature:
      return temperatures_.size() > 1;
    case SelectorField::kPushPull:
      return push_pulls_.size() > 1;
    case SelectorField::kReview:
      return true;
  }
  return false;
}

RecipeSelection RecipeSelectorModel::current_selection() const {
  RecipeSelection out;
  if (films_.empty()) {
    return out;
  }

  out.film = films_[film_index_];
  if (!developers_.empty()) {
    out.developer = developers_[developer_index_];
  }
  if (!dilutions_.empty()) {
    out.dilution = dilutions_[dilution_index_];
  }
  if (!temperatures_.empty()) {
    out.temperature_tenths_c = temperatures_[temperature_index_];
  }
  if (!push_pulls_.empty()) {
    out.push_pull_type = push_pulls_[push_pull_index_];
  }
  return out;
}

}  // namespace tutorial_0073
