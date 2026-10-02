#pragma once

#include <vector>

#include "lfw/core/value.h"

namespace lfw {
namespace bot {

Value closest(const Value& me, const std::vector<Value>& list);

}
}
