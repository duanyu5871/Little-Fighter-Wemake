#pragma once

#include <functional>
#include <string>

#include "lfw/buff/buff.h"
#include "lfw/core/value.h"
#include "lfw/defines/itr_kind.h"

namespace lfw {
namespace buff {

struct MagicFluteEnv {
  std::function<void(IBuffEntity* a, const Value& injury, IBuffEntity* v,
                     const Value& prev_hp)>
      summary_apply_damage;
};

const MagicFluteEnv& magic_flute_env();
void set_magic_flute_env(const MagicFluteEnv& env);

class Buff_MagicFlute : public Buff {
 public:
  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute);
  using Buff::Buff;
  void init() override;

 protected:
  bool has_on_update() const override { return true; }
  bool has_on_tick() const override { return true; }
  void on_tick(IBuffEntity* attacker, IBuffEntity* victim) override;
  void on_update(IBuffEntity* attacker, IBuffEntity* victim) override;
};

class Buff_MagicFlute2 : public Buff {
 public:
  static constexpr int KIND = static_cast<int>(ItrKind::MagicFlute2);
  using Buff::Buff;
  void init() override;

 protected:
  bool has_on_update() const override { return true; }
  bool has_on_tick() const override { return true; }
  void on_tick(IBuffEntity* attacker, IBuffEntity* victim) override;
  void on_update(IBuffEntity* attacker, IBuffEntity* victim) override;
};

}
}
