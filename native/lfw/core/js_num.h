#pragma once

#include <cstdint>

namespace lfw {

double js_round(double x);
double js_floor(double x);
double js_ceil(double x);
double js_abs(double x);

uint32_t js_to_uint32(double x);
int32_t js_to_int32(double x);

uint64_t f64_bits(double x);
double f64_from_bits(uint64_t bits);

}
