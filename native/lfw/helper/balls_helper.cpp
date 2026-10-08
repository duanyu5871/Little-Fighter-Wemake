#include "lfw/helper/balls_helper.h"

#include "lfw/entity/entity.h"
#include "lfw/entity/entity_type_check.h"

namespace lfw {
namespace helper {

std::vector<Entity*> BallsHelper::all() const {
  std::vector<Entity*> ret;
  for (Entity* const v : _lfw->world_entities()) {
    if (entity::is_ball_data(v->data())) ret.push_back(v);
  }
  for (Entity* const v : _lfw->world_ghosts()) {
    if (entity::is_ball_data(v->data())) ret.push_back(v);
  }
  return ret;
}

}  // namespace helper
}  // namespace lfw
