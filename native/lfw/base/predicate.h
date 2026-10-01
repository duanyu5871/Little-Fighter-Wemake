#pragma once

#include "lfw/core/value.h"
#include "lfw/defines/bin_op.h"

namespace lfw {

bool a_included_b(const Value& a, const Value& b);
bool apply_predicate(BinOp op, const Value& a, const Value& b);

}
