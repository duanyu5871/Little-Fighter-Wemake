#pragma once

#include <string>

#include "lfw/buff/buff.h"

namespace lfw {
namespace buff {

class Buff_Healing : public Buff {
 public:
  static const char16_t* KIND;
  // 读实体的 `dataset`（TS 的 `victim.dataset`）⇒ 收 `const Entity&`，与
  // `Buff_MpHealing::duration_of` 同名同型（实现落在 .cpp，变异锚点也在那儿）。
  static double duration_of(const Entity& e, double amount);

  using Buff::Buff;

  void mount() override;
  void unmount() override;

 protected:
  bool has_on_tick() const override { return true; }
  void on_tick(Entity* attacker, Entity* victim) override;
};

}
}
