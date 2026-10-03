#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/character_state_base.h"

namespace lfw {
namespace state {

class CharacterState_Falling : public CharacterState_Base {
 public:
  explicit CharacterState_Falling(
      Value state = Value(static_cast<double>(StateEnum::Falling)));

  void update(IStateEntity& e) override;
  void leave(IStateEntity& e, const Value& next_frame) override;

  bool is_bouncing_frame(IStateEntity& e);
  void update_bouncing(IStateEntity& e);
  void update_falling(IStateEntity& e);

 private:
  std::map<std::u16string, std::set<std::u16string>> _bouncing_frames_map;
};

}
}
