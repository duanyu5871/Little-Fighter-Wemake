#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

Value hit_next_frame_drink();
Value hit_next_frame_super_punch();
Value hit_next_frame_punch();
void hit_next_frame_turn_back(Value& frame, const Value& back_frame = Value());
Value hit_next_frame_jump();
Value hit_next_frame_defend();
Value hit_next_frame_weapon_atk();
Value hit_next_frame_jump_atk();

}
}
