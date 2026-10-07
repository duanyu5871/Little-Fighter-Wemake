#include "lfw/animation/delay.h"

namespace lfw {

Delay::Delay(double value) { set_value(value); }

Animation& Delay::calc() { return *this; }

}
