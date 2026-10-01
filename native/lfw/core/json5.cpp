#include "json5.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <variant>
#include <vector>

#include "js_string.h"
#include "json5_util.h"

namespace lfw {

namespace {

constexpr uint32_t kEof = 0xFFFFFFFFu;

const char16_t* const kHexDigits = u"0123456789abcdef";

std::u16string num_str(long v) { return number_to_string(static_cast<double>(v)); }

std::u16string cp_to_u16(uint32_t cp) {
  std::u16string out;
  if (cp == kEof) return out;
  if (cp <= 0xFFFF) {
    out.push_back(static_cast<char16_t>(cp));
    return out;
  }
  const uint32_t v = cp - 0x10000;
  out.push_back(static_cast<char16_t>(0xD800 + (v >> 10)));
  out.push_back(static_cast<char16_t>(0xDC00 + (v & 0x3FF)));
  return out;
}

std::u16string format_char(uint32_t cp) {
  switch (cp) {
    case u'\'':
      return u"\\'";
    case u'"':
      return u"\\\"";
    case u'\\':
      return u"\\\\";
    case 0x0008:
      return u"\\b";
    case 0x000c:
      return u"\\f";
    case 0x000a:
      return u"\\n";
    case 0x000d:
      return u"\\r";
    case 0x0009:
      return u"\\t";
    case 0x000b:
      return u"\\v";
    case 0x0000:
      return u"\\0";
    case 0x2028:
      return u"\\u2028";
    case 0x2029:
      return u"\\u2029";
    default:
      break;
  }
  if (cp < 0x20) {
    std::u16string out = u"\\x";
    out.push_back(kHexDigits[(cp >> 4) & 0xf]);
    out.push_back(kHexDigits[cp & 0xf]);
    return out;
  }
  return cp_to_u16(cp);
}

enum class TokKind { kEof, kNull, kBoolean, kNumeric, kString, kIdentifier, kPunctuator };

struct Token {
  TokKind kind = TokKind::kEof;
  bool b = false;
  double num = 0;
  std::u16string text;
  char16_t punct = 0;
};

enum class LexState {
  kDefault,
  kComment,
  kMultiLineComment,
  kMultiLineCommentAsterisk,
  kSingleLineComment,
  kValue,
  kIdentifierNameStartEscape,
  kIdentifierName,
  kIdentifierNameEscape,
  kSign,
  kZero,
  kDecimalInteger,
  kDecimalPointLeading,
  kDecimalPoint,
  kDecimalFraction,
  kDecimalExponent,
  kDecimalExponentSign,
  kDecimalExponentInteger,
  kHexadecimal,
  kHexadecimalInteger,
  kString,
  kStart,
  kBeforePropertyName,
  kAfterPropertyName,
  kBeforePropertyValue,
  kAfterPropertyValue,
  kBeforeArrayValue,
  kAfterArrayValue,
  kEnd,
};

enum class ParseState {
  kStart,
  kBeforePropertyName,
  kAfterPropertyName,
  kBeforePropertyValue,
  kBeforeArrayValue,
  kAfterPropertyValue,
  kAfterArrayValue,
  kEnd,
};

LexState lex_state_of(ParseState s) {
  switch (s) {
    case ParseState::kStart:
      return LexState::kStart;
    case ParseState::kBeforePropertyName:
      return LexState::kBeforePropertyName;
    case ParseState::kAfterPropertyName:
      return LexState::kAfterPropertyName;
    case ParseState::kBeforePropertyValue:
      return LexState::kBeforePropertyValue;
    case ParseState::kAfterPropertyValue:
      return LexState::kAfterPropertyValue;
    case ParseState::kBeforeArrayValue:
      return LexState::kBeforeArrayValue;
    case ParseState::kAfterArrayValue:
      return LexState::kAfterArrayValue;
    case ParseState::kEnd:
      return LexState::kEnd;
  }
  return LexState::kEnd;
}

struct Parser {
  explicit Parser(const std::u16string& text) : src(text) {}

  const std::u16string& src;
  size_t pos = 0;
  long line = 1;
  long column = 0;
  uint32_t c = kEof;
  LexState lex_state = LexState::kDefault;
  ParseState parse_state = ParseState::kStart;
  std::u16string buffer;
  bool double_quote = false;
  double sign = 1;
  Token token;
  std::u16string key;
  bool has_root = false;
  Value root;
  std::vector<Value> stack;
  std::u16string error;

  uint32_t peek() const {
    if (pos >= src.size()) return kEof;
    const char16_t u = src[pos];
    if (u >= 0xD800 && u <= 0xDBFF && pos + 1 < src.size()) {
      const char16_t v = src[pos + 1];
      if (v >= 0xDC00 && v <= 0xDFFF) {
        return 0x10000 + ((static_cast<uint32_t>(u) - 0xD800) << 10) +
               (static_cast<uint32_t>(v) - 0xDC00);
      }
    }
    return u;
  }

  uint32_t read() {
    const uint32_t ch = peek();
    const size_t len = ch == kEof ? 0 : (ch > 0xFFFF ? 2 : 1);
    if (ch == 0x000a) {
      ++line;
      column = 0;
    } else if (ch != kEof) {
      column += static_cast<long>(len);
    } else {
      ++column;
    }
    if (ch != kEof) pos += len;
    return ch;
  }

  bool fail_char(uint32_t ch) {
    if (ch == kEof) {
      error = u"JSON5: invalid end of input at " + num_str(line) + u":" + num_str(column);
    } else {
      error = u"JSON5: invalid character '" + format_char(ch) + u"' at " + num_str(line) + u":" +
              num_str(column);
    }
    return false;
  }

  bool fail_eof() {
    error = u"JSON5: invalid end of input at " + num_str(line) + u":" + num_str(column);
    return false;
  }

  bool fail_identifier() {
    column -= 5;
    error = u"JSON5: invalid identifier character at " + num_str(line) + u":" + num_str(column);
    return false;
  }

  void literal(const char16_t* s) {
    for (const char16_t* p = s; *p != 0; ++p) {
      const uint32_t got = peek();
      if (got != static_cast<uint32_t>(*p)) {
        fail_char(read());
        return;
      }
      read();
    }
  }

  uint32_t hex_escape() {
    std::u16string buf;
    if (!json5_is_hex_digit(peek())) {
      fail_char(read());
      return 0;
    }
    buf.push_back(static_cast<char16_t>(read()));
    if (!json5_is_hex_digit(peek())) {
      fail_char(read());
      return 0;
    }
    buf.push_back(static_cast<char16_t>(read()));
    unsigned v = 0;
    for (char16_t h : buf) {
      v = v * 16 + (h <= u'9' ? static_cast<unsigned>(h - u'0')
                              : (h <= u'F' ? static_cast<unsigned>(h - u'A') + 10
                                           : static_cast<unsigned>(h - u'a') + 10));
    }
    return v;
  }

  uint32_t unicode_escape() {
    unsigned v = 0;
    for (int k = 0; k < 4; ++k) {
      const uint32_t h = peek();
      if (!json5_is_hex_digit(h)) {
        fail_char(read());
        return 0;
      }
      read();
      v = v * 16 + (h <= u'9' ? static_cast<unsigned>(h - u'0')
                              : (h <= u'F' ? static_cast<unsigned>(h - u'A') + 10
                                           : static_cast<unsigned>(h - u'a') + 10));
    }
    return v;
  }

  std::u16string escape() {
    const uint32_t e = peek();
    switch (e) {
      case u'b':
        read();
        return u"\b";
      case u'f':
        read();
        return u"\f";
      case u'n':
        read();
        return u"\n";
      case u'r':
        read();
        return u"\r";
      case u't':
        read();
        return u"\t";
      case u'v':
        read();
        return u"\v";
      case u'0':
        read();
        if (json5_is_digit(peek())) {
          fail_char(read());
          return std::u16string();
        }
        return std::u16string(1, 0);
      case u'x':
        read();
        return cp_to_u16(hex_escape());
      case u'u':
        read();
        return cp_to_u16(unicode_escape());
      case 0x000a:
      case 0x2028:
      case 0x2029:
        read();
        return std::u16string();
      case 0x000d:
        read();
        if (peek() == 0x000a) read();
        return std::u16string();
      case u'1':
      case u'2':
      case u'3':
      case u'4':
      case u'5':
      case u'6':
      case u'7':
      case u'8':
      case u'9':
        fail_char(read());
        return std::u16string();
      case kEof:
        fail_char(read());
        return std::u16string();
      default:
        break;
    }
    return cp_to_u16(read());
  }

  bool lex(Token& out) {
    lex_state = LexState::kDefault;
    buffer.clear();
    double_quote = false;
    sign = 1;
    for (;;) {
      c = peek();
      switch (lex_state) {
        case LexState::kDefault: {
          switch (c) {
            case 0x0009:
            case 0x000b:
            case 0x000c:
            case 0x0020:
            case 0x00a0:
            case 0xfeff:
            case 0x000a:
            case 0x000d:
            case 0x2028:
            case 0x2029:
              read();
              continue;
            case u'/':
              read();
              lex_state = LexState::kComment;
              continue;
            case kEof:
              read();
              out.kind = TokKind::kEof;
              return true;
            default:
              break;
          }
          if (json5_is_space_separator(c)) {
            read();
            continue;
          }
          lex_state = lex_state_of(parse_state);
          break;
        }

        case LexState::kComment:
          if (c == u'*') {
            read();
            lex_state = LexState::kMultiLineComment;
            break;
          }
          if (c == u'/') {
            read();
            lex_state = LexState::kSingleLineComment;
            break;
          }
          return fail_char(read());

        case LexState::kMultiLineComment:
          if (c == u'*') {
            read();
            lex_state = LexState::kMultiLineCommentAsterisk;
            break;
          }
          if (c == kEof) return fail_char(read());
          read();
          break;

        case LexState::kMultiLineCommentAsterisk:
          if (c == u'*') {
            read();
            break;
          }
          if (c == u'/') {
            read();
            lex_state = LexState::kDefault;
            break;
          }
          if (c == kEof) return fail_char(read());
          read();
          lex_state = LexState::kMultiLineComment;
          break;

        case LexState::kSingleLineComment:
          if (c == 0x000a || c == 0x000d || c == 0x2028 || c == 0x2029) {
            read();
            lex_state = LexState::kDefault;
            break;
          }
          if (c == kEof) {
            read();
            out.kind = TokKind::kEof;
            return true;
          }
          read();
          break;

        case LexState::kValue:
          switch (c) {
            case u'{':
            case u'[':
              out.kind = TokKind::kPunctuator;
              out.punct = static_cast<char16_t>(read());
              return true;
            case u'n':
              read();
              literal(u"ull");
              if (!error.empty()) return false;
              out.kind = TokKind::kNull;
              return true;
            case u't':
              read();
              literal(u"rue");
              if (!error.empty()) return false;
              out.kind = TokKind::kBoolean;
              out.b = true;
              return true;
            case u'f':
              read();
              literal(u"alse");
              if (!error.empty()) return false;
              out.kind = TokKind::kBoolean;
              out.b = false;
              return true;
            case u'-':
            case u'+':
              if (read() == u'-') sign = -1;
              lex_state = LexState::kSign;
              break;
            case u'.':
              buffer += cp_to_u16(read());
              lex_state = LexState::kDecimalPointLeading;
              break;
            case u'0':
              buffer += cp_to_u16(read());
              lex_state = LexState::kZero;
              break;
            case u'1':
            case u'2':
            case u'3':
            case u'4':
            case u'5':
            case u'6':
            case u'7':
            case u'8':
            case u'9':
              buffer += cp_to_u16(read());
              lex_state = LexState::kDecimalInteger;
              break;
            case u'I':
              read();
              literal(u"nfinity");
              if (!error.empty()) return false;
              out.kind = TokKind::kNumeric;
              out.num = std::numeric_limits<double>::infinity();
              return true;
            case u'N':
              read();
              literal(u"aN");
              if (!error.empty()) return false;
              out.kind = TokKind::kNumeric;
              out.num = std::numeric_limits<double>::quiet_NaN();
              return true;
            case u'"':
            case u'\'':
              double_quote = (read() == u'"');
              buffer.clear();
              lex_state = LexState::kString;
              break;
            default:
              return fail_char(read());
          }
          break;

        case LexState::kIdentifierNameStartEscape: {
          if (c != u'u') return fail_char(read());
          read();
          const uint32_t u = unicode_escape();
          if (!error.empty()) return false;
          if (u != u'$' && u != u'_' && !json5_is_id_start_char(u)) return fail_identifier();
          buffer += cp_to_u16(u);
          lex_state = LexState::kIdentifierName;
          break;
        }

        case LexState::kIdentifierName:
          if (c == u'$' || c == u'_' || c == 0x200c || c == 0x200d) {
            buffer += cp_to_u16(read());
            break;
          }
          if (c == u'\\') {
            read();
            lex_state = LexState::kIdentifierNameEscape;
            break;
          }
          if (json5_is_id_continue_char(c)) {
            buffer += cp_to_u16(read());
            break;
          }
          out.kind = TokKind::kIdentifier;
          out.text = buffer;
          return true;

        case LexState::kIdentifierNameEscape: {
          if (c != u'u') return fail_char(read());
          read();
          const uint32_t u = unicode_escape();
          if (!error.empty()) return false;
          if (u != u'$' && u != u'_' && u != 0x200c && u != 0x200d && !json5_is_id_continue_char(u)) {
            return fail_identifier();
          }
          buffer += cp_to_u16(u);
          lex_state = LexState::kIdentifierName;
          break;
        }

        case LexState::kSign:
          if (c == u'.') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalPointLeading;
            break;
          }
          if (c == u'0') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kZero;
            break;
          }
          if (c >= u'1' && c <= u'9') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalInteger;
            break;
          }
          if (c == u'I') {
            read();
            literal(u"nfinity");
            if (!error.empty()) return false;
            out.kind = TokKind::kNumeric;
            out.num = sign * std::numeric_limits<double>::infinity();
            return true;
          }
          if (c == u'N') {
            read();
            literal(u"aN");
            if (!error.empty()) return false;
            out.kind = TokKind::kNumeric;
            out.num = std::numeric_limits<double>::quiet_NaN();
            return true;
          }
          return fail_char(read());

        case LexState::kZero:
          if (c == u'.') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalPoint;
            break;
          }
          if (c == u'e' || c == u'E') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalExponent;
            break;
          }
          if (c == u'x' || c == u'X') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kHexadecimal;
            break;
          }
          out.kind = TokKind::kNumeric;
          out.num = sign * 0;
          return true;

        case LexState::kDecimalInteger:
          if (c == u'.') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalPoint;
            break;
          }
          if (c == u'e' || c == u'E') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalExponent;
            break;
          }
          if (json5_is_digit(c)) {
            buffer += cp_to_u16(read());
            break;
          }
          out.kind = TokKind::kNumeric;
          out.num = sign * string_to_number(buffer);
          return true;

        case LexState::kDecimalPointLeading:
          if (json5_is_digit(c)) {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalFraction;
            break;
          }
          return fail_char(read());

        case LexState::kDecimalPoint:
          if (c == u'e' || c == u'E') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalExponent;
            break;
          }
          if (json5_is_digit(c)) {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalFraction;
            break;
          }
          out.kind = TokKind::kNumeric;
          out.num = sign * string_to_number(buffer);
          return true;

        case LexState::kDecimalFraction:
          if (c == u'e' || c == u'E') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalExponent;
            break;
          }
          if (json5_is_digit(c)) {
            buffer += cp_to_u16(read());
            break;
          }
          out.kind = TokKind::kNumeric;
          out.num = sign * string_to_number(buffer);
          return true;

        case LexState::kDecimalExponent:
          if (c == u'+' || c == u'-') {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalExponentSign;
            break;
          }
          if (json5_is_digit(c)) {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalExponentInteger;
            break;
          }
          return fail_char(read());

        case LexState::kDecimalExponentSign:
          if (json5_is_digit(c)) {
            buffer += cp_to_u16(read());
            lex_state = LexState::kDecimalExponentInteger;
            break;
          }
          return fail_char(read());

        case LexState::kDecimalExponentInteger:
          if (json5_is_digit(c)) {
            buffer += cp_to_u16(read());
            break;
          }
          out.kind = TokKind::kNumeric;
          out.num = sign * string_to_number(buffer);
          return true;

        case LexState::kHexadecimal:
          if (json5_is_hex_digit(c)) {
            buffer += cp_to_u16(read());
            lex_state = LexState::kHexadecimalInteger;
            break;
          }
          return fail_char(read());

        case LexState::kHexadecimalInteger:
          if (json5_is_hex_digit(c)) {
            buffer += cp_to_u16(read());
            break;
          }
          out.kind = TokKind::kNumeric;
          out.num = sign * string_to_number(buffer);
          return true;

        case LexState::kString:
          if (c == u'\\') {
            read();
            buffer += escape();
            if (!error.empty()) return false;
            break;
          }
          if (c == u'"') {
            if (double_quote) {
              read();
              out.kind = TokKind::kString;
              out.text = buffer;
              return true;
            }
            buffer += cp_to_u16(read());
            break;
          }
          if (c == u'\'') {
            if (!double_quote) {
              read();
              out.kind = TokKind::kString;
              out.text = buffer;
              return true;
            }
            buffer += cp_to_u16(read());
            break;
          }
          if (c == 0x000a || c == 0x000d) return fail_char(read());
          if (c == 0x2028 || c == 0x2029) {
            buffer += cp_to_u16(read());
            break;
          }
          if (c == kEof) return fail_char(read());
          buffer += cp_to_u16(read());
          break;

        case LexState::kStart:
          if (c == u'{' || c == u'[') {
            out.kind = TokKind::kPunctuator;
            out.punct = static_cast<char16_t>(read());
            return true;
          }
          lex_state = LexState::kValue;
          break;

        case LexState::kBeforePropertyName:
          if (c == u'$' || c == u'_') {
            buffer = cp_to_u16(read());
            lex_state = LexState::kIdentifierName;
            break;
          }
          if (c == u'\\') {
            read();
            lex_state = LexState::kIdentifierNameStartEscape;
            break;
          }
          if (c == u'}') {
            out.kind = TokKind::kPunctuator;
            out.punct = static_cast<char16_t>(read());
            return true;
          }
          if (c == u'"' || c == u'\'') {
            double_quote = (read() == u'"');
            lex_state = LexState::kString;
            break;
          }
          if (json5_is_id_start_char(c)) {
            buffer += cp_to_u16(read());
            lex_state = LexState::kIdentifierName;
            break;
          }
          return fail_char(read());

        case LexState::kAfterPropertyName:
          if (c == u':') {
            out.kind = TokKind::kPunctuator;
            out.punct = static_cast<char16_t>(read());
            return true;
          }
          return fail_char(read());

        case LexState::kBeforePropertyValue:
          lex_state = LexState::kValue;
          break;

        case LexState::kAfterPropertyValue:
          if (c == u',' || c == u'}') {
            out.kind = TokKind::kPunctuator;
            out.punct = static_cast<char16_t>(read());
            return true;
          }
          return fail_char(read());

        case LexState::kBeforeArrayValue:
          if (c == u']') {
            out.kind = TokKind::kPunctuator;
            out.punct = static_cast<char16_t>(read());
            return true;
          }
          lex_state = LexState::kValue;
          break;

        case LexState::kAfterArrayValue:
          if (c == u',' || c == u']') {
            out.kind = TokKind::kPunctuator;
            out.punct = static_cast<char16_t>(read());
            return true;
          }
          return fail_char(read());

        case LexState::kEnd:
          return fail_char(read());
      }
    }
  }

  bool push() {
    Value value;
    if (token.kind == TokKind::kPunctuator) {
      if (token.punct == u'{') value = Value(std::make_shared<Object>());
      else if (token.punct == u'[') value = Value(std::make_shared<Array>());
    } else if (token.kind == TokKind::kNull) {
      value = Value(NullTag{});
    } else if (token.kind == TokKind::kBoolean) {
      value = Value(token.b);
    } else if (token.kind == TokKind::kNumeric) {
      value = Value(token.num);
    } else if (token.kind == TokKind::kString) {
      value = Value(token.text);
    }

    if (!has_root) {
      root = value;
      has_root = true;
    } else {
      Value& parent = stack.back();
      if (Array* a = as_array(parent)) {
        a->push_back(value);
      } else if (Object* o = as_object(parent)) {
        o->set(key, value);
      }
    }

    if (as_array(value) != nullptr || as_object(value) != nullptr) {
      stack.push_back(value);
      parse_state = as_array(value) != nullptr ? ParseState::kBeforeArrayValue
                                               : ParseState::kBeforePropertyName;
    } else if (stack.empty()) {
      parse_state = ParseState::kEnd;
    } else if (as_array(stack.back()) != nullptr) {
      parse_state = ParseState::kAfterArrayValue;
    } else {
      parse_state = ParseState::kAfterPropertyValue;
    }
    return true;
  }

  void pop() {
    stack.pop_back();
    if (stack.empty()) {
      parse_state = ParseState::kEnd;
    } else if (as_array(stack.back()) != nullptr) {
      parse_state = ParseState::kAfterArrayValue;
    } else {
      parse_state = ParseState::kAfterPropertyValue;
    }
  }

  bool step() {
    switch (parse_state) {
      case ParseState::kStart:
        if (token.kind == TokKind::kEof) return fail_eof();
        return push();

      case ParseState::kBeforePropertyName:
        if (token.kind == TokKind::kIdentifier || token.kind == TokKind::kString) {
          key = token.text;
          parse_state = ParseState::kAfterPropertyName;
          return true;
        }
        if (token.kind == TokKind::kPunctuator) {
          pop();
          return true;
        }
        if (token.kind == TokKind::kEof) return fail_eof();
        return true;

      case ParseState::kAfterPropertyName:
        if (token.kind == TokKind::kEof) return fail_eof();
        parse_state = ParseState::kBeforePropertyValue;
        return true;

      case ParseState::kBeforePropertyValue:
        if (token.kind == TokKind::kEof) return fail_eof();
        return push();

      case ParseState::kBeforeArrayValue:
        if (token.kind == TokKind::kEof) return fail_eof();
        if (token.kind == TokKind::kPunctuator && token.punct == u']') {
          pop();
          return true;
        }
        return push();

      case ParseState::kAfterPropertyValue:
        if (token.kind == TokKind::kEof) return fail_eof();
        if (token.kind == TokKind::kPunctuator && token.punct == u',') {
          parse_state = ParseState::kBeforePropertyName;
          return true;
        }
        if (token.kind == TokKind::kPunctuator && token.punct == u'}') pop();
        return true;

      case ParseState::kAfterArrayValue:
        if (token.kind == TokKind::kEof) return fail_eof();
        if (token.kind == TokKind::kPunctuator && token.punct == u',') {
          parse_state = ParseState::kBeforeArrayValue;
          return true;
        }
        if (token.kind == TokKind::kPunctuator && token.punct == u']') pop();
        return true;

      case ParseState::kEnd:
        return true;
    }
    return true;
  }

  bool run(Value& out) {
    for (;;) {
      if (!lex(token)) return false;
      if (!step()) return false;
      if (token.kind == TokKind::kEof) break;
    }
    out = root;
    return true;
  }
};

}

Json5Result json5_parse(const std::u16string& text) {
  Json5Result r;
  Parser p(text);
  Value out;
  if (!p.run(out)) {
    r.ok = false;
    r.error = p.error;
    return r;
  }
  r.ok = true;
  r.value = out;
  return r;
}

namespace {

uint32_t cp_at(const std::u16string& s, size_t i) {
  const char16_t u = s[i];
  if (u >= 0xD800 && u <= 0xDBFF && i + 1 < s.size()) {
    const char16_t v = s[i + 1];
    if (v >= 0xDC00 && v <= 0xDFFF) {
      return 0x10000 + ((static_cast<uint32_t>(u) - 0xD800) << 10) +
             (static_cast<uint32_t>(v) - 0xDC00);
    }
  }
  return u;
}

const char16_t* quote_replacement(char16_t c) {
  switch (c) {
    case u'\\':
      return u"\\\\";
    case 0x0008:
      return u"\\b";
    case 0x000c:
      return u"\\f";
    case 0x000a:
      return u"\\n";
    case 0x000d:
      return u"\\r";
    case 0x0009:
      return u"\\t";
    case 0x000b:
      return u"\\v";
    case 0x0000:
      return u"\\0";
    case 0x2028:
      return u"\\u2028";
    case 0x2029:
      return u"\\u2029";
    default:
      return nullptr;
  }
}

struct Stringifier {
  std::vector<const void*> stack;
  bool circular = false;

  bool holds(const void* p) const {
    for (const void* q : stack) {
      if (q == p) return true;
    }
    return false;
  }

  std::u16string quote_string(const std::u16string& value) const {
    double n_single = 0.1;
    double n_double = 0.2;
    std::u16string product;

    for (size_t i = 0; i < value.size(); ++i) {
      const char16_t c = value[i];
      if (c == u'\'') {
        n_single += 0.1;
        product.push_back(c);
        continue;
      }
      if (c == u'"') {
        n_double += 0.2;
        product.push_back(c);
        continue;
      }
      if (c == 0 && i + 1 < value.size() && json5_is_digit(value[i + 1])) {
        product += u"\\x00";
        continue;
      }
      if (const char16_t* rep = quote_replacement(c)) {
        product += rep;
        continue;
      }
      if (c < u' ') {
        product += u"\\x";
        product.push_back(kHexDigits[(c >> 4) & 0xf]);
        product.push_back(kHexDigits[c & 0xf]);
        continue;
      }
      product.push_back(c);
    }

    const char16_t quote = n_single < n_double ? u'\'' : u'"';
    std::u16string out;
    out.push_back(quote);
    for (char16_t c : product) {
      if (c == quote) out.push_back(u'\\');
      out.push_back(c);
    }
    out.push_back(quote);
    return out;
  }

  std::u16string serialize_key(const std::u16string& key) const {
    if (key.empty()) return quote_string(key);

    const uint32_t first = cp_at(key, 0);
    if (!json5_is_id_start_char(first)) return quote_string(key);

    const size_t first_len = first > 0xFFFF ? 2 : 1;
    for (size_t i = first_len; i < key.size(); ++i) {
      if (!json5_is_id_continue_char(cp_at(key, i))) return quote_string(key);
    }
    return key;
  }

  std::u16string serialize(const Value& value) {
    if (std::holds_alternative<NullTag>(value)) return u"null";
    if (const bool* b = std::get_if<bool>(&value)) return *b ? u"true" : u"false";
    if (const double* d = std::get_if<double>(&value)) return number_to_string(*d);
    if (const std::u16string* s = std::get_if<std::u16string>(&value)) return quote_string(*s);
    if (const Array* a = as_array(value)) return serialize_array(*a);
    if (const Object* o = as_object(value)) return serialize_object(*o);
    return u"null";
  }

  std::u16string serialize_array(const Array& a) {
    if (holds(&a)) {
      circular = true;
      return std::u16string();
    }
    stack.push_back(&a);

    std::u16string out = u"[";
    const std::vector<Value>& items = a.items();
    for (size_t i = 0; i < items.size(); ++i) {
      if (i != 0) out.push_back(u',');
      out += serialize(items[i]);
      if (circular) return std::u16string();
    }
    out.push_back(u']');

    stack.pop_back();
    return out;
  }

  std::u16string serialize_object(const Object& o) {
    if (holds(&o)) {
      circular = true;
      return std::u16string();
    }
    stack.push_back(&o);

    std::u16string out = u"{";
    bool first_member = true;
    for (const std::u16string& key : o.keys()) {
      const Value* v = o.get(key);
      if (v == nullptr) continue;
      if (!first_member) out.push_back(u',');
      first_member = false;
      out += serialize_key(key);
      out.push_back(u':');
      out += serialize(*v);
      if (circular) return std::u16string();
    }
    out.push_back(u'}');

    stack.pop_back();
    return out;
  }
};

}

Json5TextResult json5_stringify(const Value& value) {
  Json5TextResult r;
  Stringifier s;
  r.text = s.serialize(value);
  if (s.circular) {
    r.ok = false;
    r.text.clear();
    r.error = u"Converting circular structure to JSON5";
    return r;
  }
  r.ok = true;
  return r;
}

}
