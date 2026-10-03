#pragma once

#include <array>
#include <variant>

#include "lfw/core/value.h"
#include "lfw/defines/weapon_type.h"

namespace lfw {

struct WeaponBounceTable {
  std::array<double, 6> min_y;
  std::array<double, 6> min_x;
  std::array<double, 6> min_z;
  std::array<double, 6> bounce_y;
  std::array<double, 6> bounce_x;
  std::array<double, 6> bounce_z;
  std::array<double, 6> fast_y;
  std::array<double, 6> fast_x;
  std::array<double, 6> fast_z;
};

inline const WeaponBounceTable& weapon_bounce_table() {
  static const WeaponBounceTable t = {
    {{2, 2, 2, 2, 1, 1}},
    {{99, 99, 2, 99, 2, 2}},
    {{99, 99, 99, 99, 99, 99}},
    {{0.5, 0.2, 0.3, 0.2, 0.45, 0.45}},
    {{0.5, 0.5, 0.75, 0.5, 0.75, 0.75}},
    {{0.5, 0.5, 0.75, 0.5, 0.75, 0.75}},
    {{99, 99, 1, 99, 99, 99}},
    {{99, 99, 1, 99, 4.5, 4.5}},
    {{99, 99, 1, 99, 99, 99}},
  };
  return t;
}

inline int weapon_bounce_index(const Value& wt) {
  if (!std::holds_alternative<double>(wt)) return -1;
  const double d = std::get<double>(wt);
  if (!(d >= 0.0 && d <= 5.0)) return -1;
  const int i = static_cast<int>(d);
  if (static_cast<double>(i) != d) return -1;
  return i;
}

inline Value wt_bounce_min_y(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().min_y[static_cast<size_t>(i)]);
}

inline Value wt_bounce_min_x(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().min_x[static_cast<size_t>(i)]);
}

inline Value wt_bounce_min_z(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().min_z[static_cast<size_t>(i)]);
}

inline Value wt_bounce_y(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().bounce_y[static_cast<size_t>(i)]);
}

inline Value wt_bounce_x(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().bounce_x[static_cast<size_t>(i)]);
}

inline Value wt_bounce_z(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().bounce_z[static_cast<size_t>(i)]);
}

inline Value wt_fast_y(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().fast_y[static_cast<size_t>(i)]);
}

inline Value wt_fast_x(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().fast_x[static_cast<size_t>(i)]);
}

inline Value wt_fast_z(const Value& wt) {
  const int i = weapon_bounce_index(wt);
  if (i < 0) return Value();
  return Value(weapon_bounce_table().fast_z[static_cast<size_t>(i)]);
}

}
