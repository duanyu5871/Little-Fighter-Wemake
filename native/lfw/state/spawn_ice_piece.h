#pragma once

#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/math/mersenne_twister.h"

namespace lfw {
namespace state {

Value spawn_ice_piece(const std::u16string& id);

const std::vector<Value>& ice_piece_opoints();

Value ice_piece_dvx(const Value& e, MersenneTwister& mt);

Value ice_piece_dvy(const Value& e, MersenneTwister& mt);

Value ice_piece_x(const Value& e, MersenneTwister& mt);

Value ice_piece_y(const Value& e, MersenneTwister& mt);

}
}
