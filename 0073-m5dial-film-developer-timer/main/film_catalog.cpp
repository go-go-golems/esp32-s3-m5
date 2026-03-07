#include "film_catalog.h"

#include "generated_film_catalog.h"

namespace tutorial_0073 {

namespace {

template <typename T>
void append_unique(std::vector<T> &values, const T &candidate) {
  if (values.empty() || values.back() != candidate) {
    values.push_back(candidate);
  }
}

}  // namespace

bool FilmCatalog::init() {
  entries_ = generated::kEntries;
  stats_.recipe_count = generated::kEntryCount;
  stats_.film_count = generated::kFilmCount;
  stats_.developer_count = generated::kDeveloperCount;
  return entries_ != nullptr && stats_.recipe_count > 0;
}

std::vector<std::string_view> FilmCatalog::films() const {
  std::vector<std::string_view> out;
  if (!entries_) {
    return out;
  }

  out.reserve(stats_.film_count);
  for (size_t i = 0; i < stats_.recipe_count; ++i) {
    append_unique(out, std::string_view(entries_[i].film));
  }
  return out;
}

std::vector<std::string_view> FilmCatalog::developers_for(std::string_view film) const {
  std::vector<std::string_view> out;
  if (!entries_) {
    return out;
  }

  for (size_t i = 0; i < stats_.recipe_count; ++i) {
    const FilmCatalogEntry &entry = entries_[i];
    if (film != entry.film) {
      continue;
    }
    append_unique(out, std::string_view(entry.developer));
  }
  return out;
}

std::vector<std::string_view> FilmCatalog::dilutions_for(std::string_view film, std::string_view developer) const {
  std::vector<std::string_view> out;
  if (!entries_) {
    return out;
  }

  for (size_t i = 0; i < stats_.recipe_count; ++i) {
    const FilmCatalogEntry &entry = entries_[i];
    if (film != entry.film || developer != entry.developer) {
      continue;
    }
    append_unique(out, std::string_view(entry.dilution));
  }
  return out;
}

std::vector<int16_t> FilmCatalog::temperatures_for(std::string_view film,
                                                   std::string_view developer,
                                                   std::string_view dilution) const {
  std::vector<int16_t> out;
  if (!entries_) {
    return out;
  }

  for (size_t i = 0; i < stats_.recipe_count; ++i) {
    const FilmCatalogEntry &entry = entries_[i];
    if (film != entry.film || developer != entry.developer || dilution != entry.dilution) {
      continue;
    }
    append_unique(out, entry.temperature_tenths_c);
  }
  return out;
}

std::vector<std::string_view> FilmCatalog::push_pull_for(std::string_view film,
                                                         std::string_view developer,
                                                         std::string_view dilution,
                                                         int16_t temperature_tenths_c) const {
  std::vector<std::string_view> out;
  if (!entries_) {
    return out;
  }

  for (size_t i = 0; i < stats_.recipe_count; ++i) {
    const FilmCatalogEntry &entry = entries_[i];
    if (film != entry.film || developer != entry.developer || dilution != entry.dilution ||
        temperature_tenths_c != entry.temperature_tenths_c) {
      continue;
    }
    append_unique(out, std::string_view(entry.push_pull_type));
  }
  return out;
}

const FilmCatalogEntry *FilmCatalog::resolve(const RecipeSelection &selection) const {
  if (!entries_) {
    return nullptr;
  }

  for (size_t i = 0; i < stats_.recipe_count; ++i) {
    const FilmCatalogEntry &entry = entries_[i];
    if (selection.film == entry.film && selection.developer == entry.developer && selection.dilution == entry.dilution &&
        selection.temperature_tenths_c == entry.temperature_tenths_c && selection.push_pull_type == entry.push_pull_type) {
      return &entry;
    }
  }
  return nullptr;
}

}  // namespace tutorial_0073
