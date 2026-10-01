#pragma once

#include <functional>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

class Object;

namespace dat_translator {

Value get_next_frame_by_raw_id(const Value& id, const std::u16string& zero_as,
                               const std::u16string& type, const Object* costs);
Value cook_next_frame_cost(Value& ret, const std::u16string& type, const Object* costs);
Value add_next_frame(const Value& src, const std::vector<Value>& items);
Value edit_next_frame(Value& nexts, const std::function<void(Value&, size_t)>& fn);

}

}
