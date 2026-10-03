#include "lfw/state/state_burning.h"

#include <memory>

#include "lfw/state/character_state_burning.h"

namespace lfw {
namespace state {

State_Burning::State_Burning(Value state)
    : StateBase_Proxy(state, std::make_unique<CharacterState_Burning>()) {}

}
}
