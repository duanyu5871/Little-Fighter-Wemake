#pragma once

#include <string>

namespace lfw {

double string_to_number(const std::u16string& s);
std::u16string number_to_string(double v);

}
