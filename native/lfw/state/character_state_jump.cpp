#include "lfw/state/character_state_jump.h"

#include <memory>
#include <string>

#include "lfw/defines/game_key.h"
#include "lfw/defines/speed_ctrl.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/float_equal.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {
namespace state {
namespace {

void csj_enter(IStateEntity& e, const Value& prev_frame) {
  (void)prev_frame;
  e.set_jumping_x(Value(0.0));
  e.set_jumping_y(Value(0.0));
  e.set_jumping_z(Value(0.0));
  e.set_jumping_t(Value(0.0));
}

void csj_on_landing(IStateEntity& e, const Value& velocity) {
  (void)velocity;
  const Value landing = e.frame_on_landing();
  if (truthy(landing)) {
    e.enter_frame(landing);
    return;
  }
  e.enter_frame_by_id(to_string(e.data_indexes_landing_1()));
  Object v;
  v.set(u"dvz", Value(4.0));
  v.set(u"ctrl_z", Value(static_cast<double>(SpeedCtrl::Control)));
  e.update_velocity(Value(std::make_shared<Object>(v)));
}

void add_jumping_t(IStateEntity& e) {
  const Value current = e.jumping_t();
  const Value atom = e.world_dataset(u"atom_time");
  e.set_jumping_t(Value(round_float(to_number(current) + to_number(atom))));
}

void add_jumping_y(IStateEntity& e) {
  const Value current = e.jumping_y();
  const Value atom = e.world_dataset(u"atom_time");
  e.set_jumping_y(Value(round_float(to_number(current) + to_number(atom))));
}

}

CharacterState_Jump::CharacterState_Jump(Value state) : CharacterState_Base(std::move(state)) {
  enter = &csj_enter;
  on_landing = &csj_on_landing;
}

void CharacterState_Jump::update(IStateEntity& e) {
  e.handle_ground_velocity_decay();
  double px = 0;
  double py = 0;
  double pz = 0;
  e.position(px, py, pz);
  if (!float_equal(py, to_number(e.ground_y()))) return;

  const Value prev = e.prev_frame();
  const Value jump_flag = field_or(prev, u"jump_flag");

  if (e.ctrl_is_bot()) {
    add_jumping_t(e);
    add_jumping_y(e);
  } else {
    add_jumping_t(e);
    if (!e.ctrl_is_end(std::u16string(gk::kR))) {
      const Value current = e.jumping_x();
      const Value atom = e.world_dataset(u"atom_time");
      e.set_jumping_x(Value(round_float(to_number(current) + to_number(atom))));
    }
    if (!e.ctrl_is_end(std::u16string(gk::kL))) {
      const Value current = e.jumping_x();
      const Value atom = e.world_dataset(u"atom_time");
      e.set_jumping_x(Value(round_float(to_number(current) - to_number(atom))));
    }
    if (!e.ctrl_is_end(std::u16string(gk::kU))) {
      const Value current = e.jumping_z();
      const Value atom = e.world_dataset(u"atom_time");
      e.set_jumping_z(Value(round_float(to_number(current) - to_number(atom))));
    }
    if (!e.ctrl_is_end(std::u16string(gk::kD))) {
      const Value current = e.jumping_z();
      const Value atom = e.world_dataset(u"atom_time");
      e.set_jumping_z(Value(round_float(to_number(current) + to_number(atom))));
    }
    if (!e.ctrl_is_end(std::u16string(gk::kj))) add_jumping_y(e);
  }
  if (!truthy(jump_flag)) return;

  const double lr = e.ctrl_lr();
  const double ud = e.ctrl_ud();
  const Value jump_height = e.dataset(u"jump_height");
  const Value jump_h_f = e.dataset(u"jump_h_f");
  double vy = to_number(jump_height) * to_number(jump_h_f);
  const Value jump_distancez = e.dataset(u"jump_distancez");
  const Value jump_z_f = e.dataset(u"jump_z_f");
  const double vz = to_number(jump_distancez) * ud * to_number(jump_z_f);
  const Value jump_distance = e.dataset(u"jump_distance");
  const Value jump_x_f = e.dataset(u"jump_x_f");
  const double vx = lr * (to_number(jump_distance) * to_number(jump_x_f) - abs(vz / 4.0));
  const double min_v = 4.0;
  if (truthy(e.jumping_t())) {
    const Value jy = e.jumping_y();
    const Value jt = e.jumping_t();
    vy = min_v + (vy - min_v) * to_number(jy) / to_number(jt);
  } else {
    vy = min_v;
  }
  e.set_velocity(Value(vx), Value(vy), Value(vz));
}

}
}
