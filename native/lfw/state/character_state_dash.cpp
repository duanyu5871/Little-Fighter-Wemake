#include "lfw/state/character_state_dash.h"

#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace state {
namespace {

void csd_enter(IStateEntity& e, const Value& prev_frame) {
  double px = 0;
  double py = 0;
  double pz = 0;
  e.position(px, py, pz);
  if (py > to_number(e.ground_y()) && e.velocity_y() != 0) return;
  const Value vx = e.velocity_x();
  const Value vz = e.velocity_z();
  Value next_vx = vx;
  Value next_vz = vz;
  const Value dash_distance = e.dataset(u"dash_distance");
  const Value dash_x_f = e.dataset(u"dash_x_f");
  const Value dash_distancez = e.dataset(u"dash_distancez");
  const Value dash_z_f = e.dataset(u"dash_z_f");
  const Value dash_height = e.dataset(u"dash_height");
  const Value dash_h_f = e.dataset(u"dash_h_f");
  const double dx = to_number(dash_distance) * to_number(dash_x_f);
  const double dz = to_number(dash_distancez) * to_number(dash_z_f);
  const double vy = to_number(dash_height) * to_number(dash_h_f);
  const double ud = e.ctrl_ud();
  const double lr = e.ctrl_lr();
  if (truthy(Value(ud))) next_vz = Value(ud * dz);
  if (strict_equals(lfw::field_or(prev_frame, u"state"),
                    Value(static_cast<double>(StateEnum::Running)))) {
    next_vx = Value(to_number(e.facing()) * dx);
  } else if (truthy(Value(lr))) {
    next_vx = Value(lr * dx);
  } else if (to_number(vx) > 0) {
    next_vx = Value(dx);
  } else if (to_number(vx) < 0) {
    next_vx = Value(-dx);
  } else {
    next_vx = Value(to_number(e.facing()) * dx);
  }
  e.set_velocity(next_vx, Value(vy), next_vz);
}

}

CharacterState_Dash::CharacterState_Dash(Value state) : CharacterState_Base(std::move(state)) {
  enter = &csd_enter;
}

}
}
