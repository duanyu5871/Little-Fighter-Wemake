#include "predicate.h"

#include <cstddef>

namespace lfw {

namespace {

long index_of(const Value& a, const Value& item) {
  if (const Array* arr = as_array(a)) {
    for (size_t i = 0; i < arr->size(); ++i) {
      if (strict_equals(arr->at(i), item)) return static_cast<long>(i);
    }
    return -1;
  }
  const std::u16string* s = std::get_if<std::u16string>(&a);
  if (s != nullptr) {
    const std::u16string needle = to_string(item);
    if (needle.empty()) return 0;
    if (needle.size() > s->size()) return -1;
    for (size_t i = 0; i + needle.size() <= s->size(); ++i) {
      if (s->compare(i, needle.size(), needle) == 0) return static_cast<long>(i);
    }
    return -1;
  }
  return -1;
}

}

bool a_included_b(const Value& a, const Value& b) {
  if (const Array* arr = as_array(b)) {
    if (arr->empty()) return true;
    for (size_t i = 0; i < arr->size(); ++i) {
      if (index_of(a, arr->at(i)) < 0) return false;
    }
    return true;
  }
  if (const std::u16string* s = std::get_if<std::u16string>(&b)) {
    if (s->empty()) return true;
    return false;
  }
  return true;
}

bool apply_predicate(BinOp op, const Value& a, const Value& b) {
  switch (op) {
    case BinOp::kEqual:
      return equals(a, b);
    case BinOp::kNotEqual:
      return !equals(a, b);
    case BinOp::kGreaterOrEqual:
      return ge(a, b);
    case BinOp::kLessOrEqual:
      return le(a, b);
    case BinOp::kLess:
      return lt(a, b);
    case BinOp::kGreater:
      return gt(a, b);
    case BinOp::kInclude:
      return a_included_b(a, b);
    case BinOp::kIncludedBy:
      return a_included_b(b, a);
    case BinOp::kNotInclude:
      return !a_included_b(a, b);
    case BinOp::kNotIncludedBy:
      return !a_included_b(b, a);
  }
  return false;
}

}
