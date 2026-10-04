#include "lfw/state/state_base.h"

#include <variant>

#include "lfw/buff/buff_healing.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/math/clamp.h"
#include "lfw/utils/math/float_equal.h"

namespace lfw {
namespace state {
namespace {

StateEnv g_env;

constexpr double kMinV = 0.5;
constexpr double kStateHealSelfHp = 104;

bool is_null(const Value& v) { return std::holds_alternative<NullTag>(v); }

Value clamp_velocity(const Value& v) {
  const double d = to_number(v);
  if (d < -kMinV) return Value(-kMinV);
  if (d > kMinV) return Value(kMinV);
  return v;
}

}

const StateEnv& state_env() { return g_env; }

void set_state_env(const StateEnv& env) { g_env = env; }

void State_Base::update(IStateEntity& e) { (void)e; }

void State_Base::leave(IStateEntity& e, const Value& next_frame) {
  (void)next_frame;
  if (!strict_equals(_state, Value(static_cast<double>(StateEnum::HealSelf)))) return;
  buff::grant_buff(g_env.buff_env, buff::Buff_Healing::KIND, nullptr, &e,
                   buff::Buff_Healing::duration_of(e, kStateHealSelfHp));
}

void State_Base::on_restrict(IStateEntity& e, double x, double y, double z) {
  Value vx(NullTag{});
  Value vz(NullTag{});
  const Value vy(NullTag{});
  double px = 0;
  double py = 0;
  double pz = 0;
  e.position(px, py, pz);
  if (!float_equal(x, px)) vx = clamp_velocity(e.velocity_x());
  if (!float_equal(z, pz)) vz = clamp_velocity(e.velocity_z());
  if (!float_equal(y, py)) {
    vx = clamp_velocity(e.velocity_x());
    vz = clamp_velocity(e.velocity_z());
  }
  if (!is_null(vx) || !is_null(vz) || !is_null(vy)) e.set_velocity(vx, vy, vz);
  // `e.position.x = x` 直写：TS 这里不经过 `set_position`。
  e.assign_position(x, y, z);
}

}
}
