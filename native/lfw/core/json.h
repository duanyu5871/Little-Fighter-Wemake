#pragma once

#include <optional>
#include <string>

#include "lfw/core/value.h"

namespace lfw {

std::optional<std::u16string> json_stringify(const Value& v);
std::optional<Value> json_parse(const std::u16string& text);

}
