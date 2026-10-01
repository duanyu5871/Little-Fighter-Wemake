#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

void make_frames_special(Value& ret);
void make_entity_special(Value& ret);
Value make_entity_data(Value& ctx);
void post_process_obj_data(Value& ctx);

}

}
