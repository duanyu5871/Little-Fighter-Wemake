#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

Value edit_bdy_edit(Value& bdy, const Value& fields);
Value edit_bdy_clone(Value& bdy, const Value& fields);
Value cook_ball_bdy_get_hit_to_frame_20(Value& ctx);
Value cook_ball_bdy_get_hit_to_frame_30(Value& ctx);

}

}
