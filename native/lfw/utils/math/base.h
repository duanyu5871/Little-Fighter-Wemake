#pragma once

#include <cmath>
#include <initializer_list>
#include <limits>
#include <numbers>

#include "lfw/core/js_num.h"

namespace lfw {

inline double floor(double x) { return js_floor(x); }
inline double ceil(double x) { return js_ceil(x); }
inline double abs(double x) { return js_abs(x); }
inline double round(double x) { return js_round(x); }

inline bool between(double v, double lo, double hi) { return v >= lo && v <= hi; }

inline double max(std::initializer_list<double> xs) {
  double r = -std::numeric_limits<double>::infinity();
  for (double x : xs) {
    if (std::isnan(x)) return x;
    if (x > r || (x == 0.0 && r == 0.0 && !std::signbit(x))) r = x;
  }
  return r;
}

inline double min(std::initializer_list<double> xs) {
  double r = std::numeric_limits<double>::infinity();
  for (double x : xs) {
    if (std::isnan(x)) return x;
    if (x < r || (x == 0.0 && r == 0.0 && std::signbit(x))) r = x;
  }
  return r;
}

inline double max(double a, double b) { return max({a, b}); }
inline double min(double a, double b) { return min({a, b}); }

inline double sqrt(double x) { return std::sqrt(x); }
inline double cos(double x) { return std::cos(x); }
inline double sin(double x) { return std::sin(x); }
inline double tan(double x) { return std::tan(x); }
inline double acos(double x) { return std::acos(x); }
inline double pow(double x, double y) { return std::pow(x, y); }

inline double sign(double x) {
  if (std::isnan(x)) return x;
  if (x == 0.0) return x;
  return x < 0.0 ? -1.0 : 1.0;
}

inline constexpr double PI = std::numbers::pi;

}
