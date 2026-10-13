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

Value nan_to_null(double v) { return std::isnan(v) ? Value() : Value(v); }

Value clamp_velocity(const Value& v) {
  const double d = to_number(v);
  if (d < -kMinV) return Value(-kMinV);
  if (d > kMinV) return Value(kMinV);
  return v;
}

double clamp_velocity(double v) {
  if (v < -kMinV) return -kMinV;
  if (v > kMinV) return kMinV;
  return v;
}

}

const StateEnv& state_env() { return g_env; }

void set_state_env(const StateEnv& env) { g_env = env; }

void State_Base::update(IStateEntity& e) { (void)e; }

void State_Base::leave(IStateEntity& e, const Value& next_frame) {
  (void)next_frame;
  if (!strict_equals(_state, Value(static_cast<double>(StateEnum::HealSelf)))) return;
  // `duration_of` 读实体自己的 `dataset`（TS: `victim.dataset`）⇒ 经 `entity_self()`
  // 回到 `Entity`（切片 4BY 前这里收的是 `IBuffEntity&`）。台面里 `entity_self()` 给了
  // 空值时受害者也空，`grant_buff` 只会走到 `create_buff` 那条日志，时长取什么都看不见。
  Entity* const victim = e.entity_self();
  const double duration =
      victim == nullptr ? 0.0 : buff::Buff_Healing::duration_of(*victim, kStateHealSelfHp);
  buff::grant_buff(g_env.buff_env, buff::Buff_Healing::KIND, nullptr, victim, duration);
}

void State_Base::on_restrict(IStateEntity& e, double x, double y, double z) {
  Value vx(NullTag{});
  Value vz(NullTag{});
  const Value vy(NullTag{});
  double px = 0;
  double py = 0;
  double pz = 0;
  e.position(px, py, pz);
  if (!float_equal(x, px)) vx = nan_to_null(clamp_velocity(e.velocity_x()));
  if (!float_equal(z, pz)) vz = clamp_velocity(e.velocity_z());
  if (!float_equal(y, py)) {
    vx = nan_to_null(clamp_velocity(e.velocity_x()));
    vz = clamp_velocity(e.velocity_z());
  }
  if (!is_null(vx) || !is_null(vz) || !is_null(vy)) e.set_velocity(vx, vy, vz);
  // `e.position.x = x` 直写：TS 这里不经过 `set_position`。
  e.assign_position(x, y, z);
}

}
}
