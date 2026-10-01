#include "lfw/dat_translator/broken_piece_frames.h"

#include <memory>
#include <optional>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/utils/math/range.h"

namespace lfw {
namespace dat_translator {

namespace {

Value piece_range(double from, double to) {
  const std::optional<std::vector<double>> nums = range(from, to);
  Array a;
  if (nums.has_value()) {
    for (double v : *nums) a.push_back(to_string(Value(v)));
  }
  return Value(std::make_shared<Array>(a));
}

}

Value stone() {
  static const Value v = piece_range(0, 3);
  return v;
}

Value sstone() {
  static const Value v = piece_range(4, 7);
  return v;
}

Value stick() {
  static const Value v = piece_range(10, 13);
  return v;
}

Value sstick() {
  static const Value v = piece_range(14, 17);
  return v;
}

Value hoe() {
  static const Value v = piece_range(20, 23);
  return v;
}

Value s_hoe() {
  static const Value v = piece_range(24, 27);
  return v;
}

Value k_hoe() {
  static const Value v = piece_range(30, 33);
  return v;
}

Value box0() {
  static const Value v = piece_range(40, 43);
  return v;
}

Value box1() {
  static const Value v = piece_range(44, 47);
  return v;
}

Value box2() {
  static const Value v = piece_range(50, 53);
  return v;
}

Value box3() {
  static const Value v = piece_range(54, 57);
  return v;
}

Value baseball() {
  static const Value v = piece_range(60, 63);
  return v;
}

Value milk1() {
  static const Value v = piece_range(70, 73);
  return v;
}

Value milk2() {
  static const Value v = piece_range(74, 77);
  return v;
}

Value milk3() {
  static const Value v = piece_range(80, 83);
  return v;
}

Value ice1() {
  static const Value v = piece_range(120, 123);
  return v;
}

Value ice2() {
  static const Value v = piece_range(125, 128);
  return v;
}

Value ice3() {
  static const Value v = piece_range(130, 133);
  return v;
}

Value ice4() {
  static const Value v = piece_range(130, 133);
  return v;
}

Value ice5() {
  static const Value v = piece_range(135, 138);
  return v;
}

Value burning_smoke() {
  static const Value v = piece_range(140, 143);
  return v;
}

Value icesword1() {
  static const Value v = piece_range(150, 153);
  return v;
}

Value icesword2() {
  static const Value v = piece_range(154, 157);
  return v;
}

Value beer1() {
  static const Value v = piece_range(160, 163);
  return v;
}

Value beer2() {
  static const Value v = piece_range(164, 167);
  return v;
}

Value boomerang() {
  static const Value v = piece_range(170, 173);
  return v;
}

Value armour() {
  static const Value v = piece_range(174, 177);
  return v;
}

}
}
