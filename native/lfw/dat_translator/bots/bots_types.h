#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

class CondMaker;

namespace dat_translator {
namespace bots {

using EditBotAction = std::function<Value(Value& action, CondMaker& cond)>;
using EditBotActionFunc = std::function<Value(const EditBotAction* fn)>;

}
}
}
