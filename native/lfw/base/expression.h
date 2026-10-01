#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/predicate.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/defines/bin_op.h"

namespace lfw {

template <typename Ctx>
using ValGetter = Value (*)(const Ctx& ctx, const std::u16string& word, BinOp op);

template <typename Ctx>
using ValGetterGetter = ValGetter<Ctx> (*)(const std::u16string& word);

template <typename Ctx>
class Expression {
 public:
  Expression() = default;

  Expression(const std::u16string& source, ValGetterGetter<Ctx> getter_getter) {
    _getter_getter = getter_getter;
    std::u16string text;
    text.reserve(source.size());
    for (char16_t c : source) {
      if (!is_str_white_space(c)) text.push_back(c);
    }

    bool has_meta = false;
    for (char16_t c : text) {
      if (c == u'&' || c == u'|' || c == u'(' || c == u')') {
        has_meta = true;
        break;
      }
    }
    if (!has_meta) {
      parse(text);
      return;
    }

    size_t p = 0;
    const size_t count = text.size() + 1;
    size_t i = 0;
    char16_t letter = 0;
    std::u16string before;
    for (; i < count; ++i) {
      letter = i < text.size() ? text[i] : u'\0';
      if (letter == u'!' && i + 1 < text.size() && text[i + 1] == u'(') {
        Expression child(text.substr(i + 2), getter_getter);
        child.not_ = true;
        child.before = before;
        i += child.text.size() + 2;
        p = i + 2;
        children.push_back(std::move(child));
      } else if (letter == u'(') {
        Expression child(text.substr(i + 1), getter_getter);
        child.before = before;
        i += child.text.size() + 1;
        p = i + 1;
        children.push_back(std::move(child));
      } else if (letter == u'|' || letter == u'&') {
        bool db = false;
        if (i + 1 < text.size() && text[i + 1] == letter) {
          db = true;
          ++i;
        }
        const size_t stop = db ? i - 1 : i;
        if (p < stop) {
          std::u16string sub = text.substr(p, stop - p);
          while (!sub.empty() && sub.back() == u')') sub.pop_back();
          Expression child(sub, getter_getter);
          child.before = before;
          children.push_back(std::move(child));
          before = std::u16string(1, letter);
        } else {
          before = std::u16string(1, letter);
        }
        p = i + 1;
      } else if (letter == u')' || letter == u'\0') {
        if (p < i) {
          Expression child(text.substr(p, i - p), getter_getter);
          child.before = before;
          children.push_back(std::move(child));
        }
        break;
      }
    }
    this->text = text.substr(0, i);
  }

  bool run(const Ctx& ctx) {
    if (mode == Mode::kConstant) {
      result = constant;
      has_result = true;
      return constant;
    }
    if (mode == Mode::kLeaf) {
      const Value v1 = getter_1 ? getter_1(ctx, word_1, op) : lit_1;
      const Value v2 = getter_2 ? getter_2(ctx, word_2, op) : lit_2;
      val_1 = v1;
      val_2 = v2;
      const bool r = apply_predicate(op, v1, v2);
      result = r;
      has_result = true;
      return r;
    }

    bool or_result = false;
    bool and_result = false;
    bool and_set = false;
    const size_t len = children.size();
    for (size_t i = 0; i < len; ++i) {
      Expression& child = children[i];
      if (i == 0 || child.before.empty()) {
        and_result = child.run(ctx);
        and_set = true;
      } else if (child.before == u"&") {
        if (and_set && !and_result) continue;
        and_result = and_result && child.run(ctx);
        and_set = true;
      } else if (child.before == u"|") {
        or_result = or_result || and_result;
        if (or_result) {
          result = !not_;
          has_result = true;
          return result;
        }
        and_result = child.run(ctx);
        and_set = true;
      }
    }
    if (and_set) or_result = or_result || and_result;
    result = not_ ? !or_result : or_result;
    has_result = true;
    return result;
  }

  enum class Mode { kTree, kLeaf, kConstant };

  Mode mode = Mode::kTree;
  bool constant = false;
  bool is_expression = true;
  std::vector<Expression> children;
  std::u16string text;
  std::u16string before;
  bool not_ = false;
  std::u16string err;
  bool has_err = false;
  bool has_result = false;
  bool result = false;
  BinOp op = BinOp::kEqual;
  bool has_op = false;
  std::u16string op_text;
  Value val_1;
  Value val_2;

  ValGetter<Ctx> getter_1 = nullptr;
  ValGetter<Ctx> getter_2 = nullptr;
  std::u16string word_1;
  std::u16string word_2;

 private:
  void always_false(const std::u16string& e) {
    err = e;
    has_err = true;
    result = false;
    has_result = true;
    mode = Mode::kConstant;
    constant = false;
  }

  static bool find_op(const std::u16string& t, size_t& k, size_t& oplen, std::u16string& found) {
    if (t.size() >= 2) {
      for (size_t j = t.size() - 2;; --j) {
        BinOp op{};
        if (bin_op_from_two(t[j], t[j + 1], op)) {
          k = j;
          oplen = 2;
          found = t.substr(j, 2);
          return true;
        }
        if (j == 0) break;
      }
    }
    if (!t.empty()) {
      for (size_t j = t.size(); j-- > 0;) {
        if (t[j] == u'=' || t[j] == u'<' || t[j] == u'>') {
          k = j;
          oplen = 1;
          found = t.substr(j, 1);
          return true;
        }
      }
    }
    return false;
  }

  static std::shared_ptr<Array> make_array(std::vector<Value> items) {
    return std::make_shared<Array>(std::move(items));
  }

  static std::vector<Value> split_commas(const std::u16string& s) {
    std::vector<Value> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
      if (i == s.size() || s[i] == u',') {
        out.push_back(Value(s.substr(start, i - start)));
        start = i + 1;
      }
    }
    return out;
  }

  void parse(const std::u16string& src) {
    text = src;
    if (src.empty()) return always_false(u"[empty text]");

    size_t k = 0;
    size_t oplen = 0;
    std::u16string otext;
    if (!find_op(src, k, oplen, otext)) {
      return always_false(u"[wrong expression: " + src + u"]");
    }
    op_text = otext;
    has_op = true;

    const std::u16string w1 = src.substr(0, k);
    const std::u16string w2 = src.substr(k + oplen);
    if (w1.empty() || w2.empty()) {
      return always_false(u"[wrong expression: " + src + u"]");
    }

    BinOp fop{};
    const bool valid =
        oplen == 2 ? bin_op_from_two(otext[0], otext[1], fop) : bin_op_from_one(otext[0], fop);
    if (!valid) return always_false(u"wrong operator: " + otext);
    op = fop;

    getter_1 = _getter_getter ? _getter_getter(w1) : nullptr;
    getter_2 = _getter_getter ? _getter_getter(w2) : nullptr;
    word_1 = w1;
    word_2 = w2;

    Value v1 = Value(w1);
    Value v2 = Value(w2);
    if (bin_op_is_set(op)) {
      if (!getter_1) v1 = Value(make_array(split_commas(w1)));
      if (!getter_2) v2 = Value(make_array(split_commas(w2)));
    }
    lit_1 = v1;
    lit_2 = v2;

    if (!getter_1 && !getter_2) {
      val_1 = v1;
      val_2 = v2;
      constant = apply_predicate(op, v1, v2);
      result = constant;
      has_result = true;
      mode = Mode::kConstant;
      return;
    }
    mode = Mode::kLeaf;
  }

  ValGetterGetter<Ctx> _getter_getter = nullptr;
  Value lit_1;
  Value lit_2;
};

}
