#pragma once

#include <string>

#include "lfw/buff/buff.h"

namespace lfw {
namespace buff {

class Buff_Electrify : public Buff {
 public:
  static const char16_t* KIND;

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
