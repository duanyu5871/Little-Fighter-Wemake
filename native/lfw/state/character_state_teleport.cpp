#include "lfw/state/character_state_teleport.h"

#include "lfw/utils/math/base.h"
#include "lfw/entity/entity.h"

namespace lfw {
namespace state {
namespace {

Value pick_target(Entity& e, bool nearest_enemy) {
  double best = -1;
  Value tar;
  const Value list = e.world_entities();
  const Array* arr = as_array(list);
  if (arr != nullptr) {
    for (size_t i = 0; i < arr->size(); ++i) {
      const Value o = arr->at(i);
      if (!e.is_fighter_ref(o) || e.is_self_ref(o)) continue;
      if (nearest_enemy ? e.is_ally_ref(o) : !e.is_ally_ref(o)) continue;
      if (to_number(e.ref_hp(o)) <= 0) continue;
      double mx = 0;
      double my = 0;
      double mz = 0;
      mx = e.position.x; my = e.position.y; mz = e.position.z;
      const double dis = abs(e.ref_position_x(o) - mx) +
                         abs(e.ref_position_z(o) - e.ref_position_z(o));
      const bool better = nearest_enemy ? (best < 0 || dis < best) : (dis > best);
      if (better) {
        best = dis;
        tar = o;
      }
    }
  }
  return tar;
}

void teleport_to(Entity& e, const Value& tar) {
  double x = 0;
  double y = 0;
  double z = 0;
  x = e.position.x; y = e.position.y; z = e.position.z;
  if (truthy(tar)) {
    x = round(e.ref_position_x(tar) - to_number(e.facing) * 120);
    z = round(e.ref_position_z(tar));
  }
  const Value segment = e.ground_segment(x, z);
  const double gy = e.ground_y(segment, x, z);
  e.set_position(Value(x), Value(gy), Value(z));
}

void cst_nearest_enter(Entity& e, const Value& prev_frame) {
  (void)prev_frame;
  teleport_to(e, pick_target(e, true));
}

void cst_farthest_enter(Entity& e, const Value& prev_frame) {
  (void)prev_frame;
  teleport_to(e, pick_target(e, false));
}

}

CharacterState_Teleport2NearestEnemy::CharacterState_Teleport2NearestEnemy(Value state)
    : CharacterState_Base(std::move(state)) {
  enter = &cst_nearest_enter;
}

CharacterState_Teleport2FarthestAlly::CharacterState_Teleport2FarthestAlly(Value state)
    : CharacterState_Base(std::move(state)) {
  enter = &cst_farthest_enter;
}

}
}
