#pragma once

#include <utility>

#include "lfw/core/value.h"
#include "lfw/defines/state_enum.h"
#include "lfw/state/state_base_proxy.h"

namespace lfw {
namespace state {

class State_Burning : public StateBase_Proxy {
 public:
  explicit State_Burning(Value state = Value(static_cast<double>(StateEnum::Burning)));
};

}
}
