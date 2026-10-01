#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

void cook_bdy(Value& bdy, const Value& frame);
void cook_wpoint(Value& wpoint, const Value& frame);
void cook_cpoint(Value& cpoint, const Value& frame);
void cook_itr(Value& itr, const Value& frame);
void cook_opoint(Value& opoint, const Value& frame);
void float_scaling_itr(Value& v);

}

}
