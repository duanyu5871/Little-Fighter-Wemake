#pragma once

#include "lfw/core/value.h"

namespace lfw {
namespace loader {

Value preprocess_pic(Value& pic);
Value preprocess_frame_pic(Value& frame);
Value preprocess_wpoint(Value& wpoint);

}
}
