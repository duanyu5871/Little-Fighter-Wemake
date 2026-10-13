#pragma once

#include <string>

#include "lfw/buff/buff.h"

namespace lfw {
namespace buff {

class Buff_Electroshock : public Buff {
 public:
  static const char16_t* KIND;

  using Buff::Buff;

  void init() override;
  void mount() override;

 protected:
  std::u16string effect_oid() const override;
  void place_effect(Entity* effect, Entity* victim) override;
  bool has_on_tick() const override { return true; }
  void on_tick(Entity* attacker, Entity* victim) override;
};

}
}
