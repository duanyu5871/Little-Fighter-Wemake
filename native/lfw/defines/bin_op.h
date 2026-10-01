#pragma once

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

inline const char* bin_op_text(BinOp op) {
  switch (op) {
    case BinOp::kLess:
      return "<";
    case BinOp::kLessOrEqual:
      return "<=";
    case BinOp::kEqual:
      return "==";
    case BinOp::kGreaterOrEqual:
      return ">=";
    case BinOp::kGreater:
      return ">";
    case BinOp::kNotEqual:
      return "!=";
    case BinOp::kIncludedBy:
      return "}}";
    case BinOp::kInclude:
      return "{{";
    case BinOp::kNotIncludedBy:
      return "!}";
    case BinOp::kNotInclude:
      return "!{";
  }
  return "?";
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
