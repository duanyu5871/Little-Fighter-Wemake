#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

enum class BinOp {
  kLess,
  kLessOrEqual,
  kEqual,
  kGreaterOrEqual,
  kGreater,
  kNotEqual,
  kIncludedBy,
  kInclude,
  kNotIncludedBy,
  kNotInclude,
};

inline const char16_t* bin_op_text(BinOp op) {
  switch (op) {
    case BinOp::kLess:
      return u"<";
    case BinOp::kLessOrEqual:
      return u"<=";
    case BinOp::kEqual:
      return u"==";
    case BinOp::kGreaterOrEqual:
      return u">=";
    case BinOp::kGreater:
      return u">";
    case BinOp::kNotEqual:
      return u"!=";
    case BinOp::kIncludedBy:
      return u"}}";
    case BinOp::kInclude:
      return u"{{";
    case BinOp::kNotIncludedBy:
      return u"!}";
    case BinOp::kNotInclude:
      return u"!{";
  }
  return u"?";
}

inline const std::vector<EnumTextEntry>& bin_op_entries() {
  static const std::vector<EnumTextEntry> e = {
      {u"LESS", bin_op_text(BinOp::kLess)},
      {u"LESS_OR_EQUAL", bin_op_text(BinOp::kLessOrEqual)},
      {u"EQUAL", bin_op_text(BinOp::kEqual)},
      {u"GREATER_OR_EQUAL", bin_op_text(BinOp::kGreaterOrEqual)},
      {u"GREATER", bin_op_text(BinOp::kGreater)},
      {u"NOT_EQUAL", bin_op_text(BinOp::kNotEqual)},
      {u"IncludedBy", bin_op_text(BinOp::kIncludedBy)},
      {u"Include", bin_op_text(BinOp::kInclude)},
      {u"NotIncludedBy", bin_op_text(BinOp::kNotIncludedBy)},
      {u"NotInclude", bin_op_text(BinOp::kNotInclude)},
  };
  return e;
}

inline bool bin_op_is_set(BinOp op) {
  return op == BinOp::kInclude || op == BinOp::kIncludedBy || op == BinOp::kNotInclude ||
         op == BinOp::kNotIncludedBy;
}

inline bool bin_op_from_two(char16_t a, char16_t b, BinOp& out) {
  if (a == u'=' && b == u'=') {
    out = BinOp::kEqual;
    return true;
  }
  if (a == u'!' && b == u'=') {
    out = BinOp::kNotEqual;
    return true;
  }
  if (a == u'<' && b == u'=') {
    out = BinOp::kLessOrEqual;
    return true;
  }
  if (a == u'>' && b == u'=') {
    out = BinOp::kGreaterOrEqual;
    return true;
  }
  if (a == u'{' && b == u'{') {
    out = BinOp::kInclude;
    return true;
  }
  if (a == u'}' && b == u'}') {
    out = BinOp::kIncludedBy;
    return true;
  }
  if (a == u'!' && b == u'{') {
    out = BinOp::kNotInclude;
    return true;
  }
  if (a == u'!' && b == u'}') {
    out = BinOp::kNotIncludedBy;
    return true;
  }
  return false;
}

inline bool bin_op_from_one(char16_t c, BinOp& out) {
  if (c == u'<') {
    out = BinOp::kLess;
    return true;
  }
  if (c == u'>') {
    out = BinOp::kGreater;
    return true;
  }
  return false;
}

}
