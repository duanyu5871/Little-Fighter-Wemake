#pragma once

#include <string>

#include "lfw/buff/buff.h"

namespace lfw {
namespace buff {

class Buff_Healing : public Buff {
 public:
  static const char16_t* KIND;
  static double duration_of(const IBuffEntity& e, double amount);

  using Buff::Buff;

  void mount() override;
  void unmount() override;

 protected:
  bool has_on_tick() const override { return true; }
  void on_tick(IBuffEntity* attacker, IBuffEntity* victim) override;
};

}
}
