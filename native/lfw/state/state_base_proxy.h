#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/character_state_base.h"
#include "lfw/state/state_base.h"
#include "lfw/state/state_misc.h"
#include "lfw/state/weapon_state_base.h"

namespace lfw {
namespace state {

class StateBase_Proxy : public State_Base {
 public:
  explicit StateBase_Proxy(Value state);
  State_Base& get_proxy(IStateEntity& e);
  void update(IStateEntity& e) override;
  void leave(IStateEntity& e, const Value& next_frame) override;
  void on_restrict(IStateEntity& e, double x, double y, double z) override;

 private:
  CharacterState_Base _character_proxy;
  WeaponState_Base _weapon_proxy;
  BallState_Base _ball_proxy;
  State_Base _proxy;
};

class State_15 : public StateBase_Proxy {
 public:
  State_15();
};

class State_Frozen : public StateBase_Proxy {
 public:
  explicit State_Frozen(Value state = Value(static_cast<double>(StateEnum::Frozen)));
  void leave(IStateEntity& e, const Value& next_frame) override;
};

}
}
