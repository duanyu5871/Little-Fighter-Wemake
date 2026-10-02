#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

void scale_num_field(Object& o, const char16_t* key);
Value float_scaling_entity(Value& ret);

}
}
