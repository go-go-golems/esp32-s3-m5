#include "film_catalog.h"

#include "generated_film_catalog.h"

namespace tutorial_0073 {

bool FilmCatalog::init() {
  entries_ = generated::kEntries;
  stats_.recipe_count = generated::kEntryCount;
  stats_.film_count = generated::kFilmCount;
  stats_.developer_count = generated::kDeveloperCount;
  return entries_ != nullptr && stats_.recipe_count > 0;
}

}  // namespace tutorial_0073
