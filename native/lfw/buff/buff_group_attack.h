#pragma once

#include <string>
#include <vector>

#include "lfw/buff/buff.h"

namespace lfw {
namespace buff {

class Buff_GroupAttack : public Buff {
 public:
  static const char16_t* KIND;
  static const std::vector<std::u16string>& GROUPS();

  using Buff::Buff;

 protected:
  std::u16string effect_oid() const override;
  std::u16string effect_frame_id() const override;
  void place_effect(IBuffEntity* effect, IBuffEntity* victim) override;

 public:
  void mount() override;
  void unmount() override;
};

}
}
