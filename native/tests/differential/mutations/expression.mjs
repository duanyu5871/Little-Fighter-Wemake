// 已删除的等价变异（不要重加）：
// 1. &/| 分支的切片去尾右括号循环改成 if / 完全去掉。
//    该切片恒不可能以 ')' 结尾：扫描到任意 ')' 都会 break，而 p 在每次分支后都
//    已被推到已消费区之后（'(' 分支 p=i+1、'!(' 分支 p=i+2），所以 [p, stop)
//    里的每个下标都被扫描过，里面不可能藏着 ')'。→ 死代码。
// 2. run 的 and 组去掉 `continue`。`and_result && child.run(ctx)` 本身就会在
//    and_result 为假时短路，那个 continue 是冗余守卫。
export default {
  subject: "expression",
  mutations: [
    {
      note: "构造：空白不被剥离",
      file: "native/lfw/base/expression.h",
      from: `      if (!is_str_white_space(c)) src_text.push_back(c);`,
      to: `      src_text.push_back(c);`,
    },
    {
      note: "构造：多剥离一个字符",
      file: "native/lfw/base/expression.h",
      from: `      if (!is_str_white_space(c)) src_text.push_back(c);`,
      to: `      if (!is_str_white_space(c) && c != u'=') src_text.push_back(c);`,
    },
    {
      note: "构造：检测分隔符时漏掉右括号",
      file: "native/lfw/base/expression.h",
      from: `      if (c == u'&' || c == u'|' || c == u'(' || c == u')') {`,
      to: `      if (c == u'&' || c == u'|' || c == u'(') {`,
    },
    {
      note: "构造：检测分隔符时漏掉左括号",
      file: "native/lfw/base/expression.h",
      from: `      if (c == u'&' || c == u'|' || c == u'(' || c == u')') {`,
      to: `      if (c == u'&' || c == u'|' || c == u')') {`,
    },
    {
      note: "构造：has_meta 判定取反",
      file: "native/lfw/base/expression.h",
      from: `    if (!has_meta) {
      parse(src_text);
      return;
    }`,
      to: `    if (has_meta) {
      parse(src_text);
      return;
    }`,
    },
    {
      note: "构造：!（ 的分支不再要求后一个字符是左括号",
      file: "native/lfw/base/expression.h",
      from: `      if (letter == u'!' && i + 1 < src_text.size() && src_text[i + 1] == u'(') {`,
      to: `      if (letter == u'!' && i + 1 < src_text.size() && src_text[i + 1] == u'!') {`,
    },
    {
      note: "构造：!（ 的子节点不取反",
      file: "native/lfw/base/expression.h",
      from: `        Expression child(src_text.substr(i + 2), getter_getter);
        child.not_ = true;`,
      to: `        Expression child(src_text.substr(i + 2), getter_getter);
        child.not_ = false;`,
    },
    {
      note: "构造：!（ 的跳过量少 1",
      file: "native/lfw/base/expression.h",
      from: `        i += child.text.size() + 2;
        p = i + 2;`,
      to: `        i += child.text.size() + 1;
        p = i + 2;`,
    },
    {
      note: "构造：!（ 的切片起点少 1",
      file: "native/lfw/base/expression.h",
      from: `        i += child.text.size() + 2;
        p = i + 2;`,
      to: `        i += child.text.size() + 2;
        p = i + 1;`,
    },
    {
      note: "构造：（ 的跳过量多 1",
      file: "native/lfw/base/expression.h",
      from: `        Expression child(src_text.substr(i + 1), getter_getter);
        child.before = pending;
        i += child.text.size() + 1;
        p = i + 1;`,
      to: `        Expression child(src_text.substr(i + 1), getter_getter);
        child.before = pending;
        i += child.text.size() + 2;
        p = i + 1;`,
    },
    {
      note: "构造：（ 的切片起点多 1",
      file: "native/lfw/base/expression.h",
      from: `        Expression child(src_text.substr(i + 1), getter_getter);
        child.before = pending;
        i += child.text.size() + 1;
        p = i + 1;`,
      to: `        Expression child(src_text.substr(i + 1), getter_getter);
        child.before = pending;
        i += child.text.size() + 1;
        p = i + 2;`,
    },
    {
      note: "构造：双写分隔符不额外前进",
      file: "native/lfw/base/expression.h",
      from: `        if (i + 1 < src_text.size() && src_text[i + 1] == letter) {
          db = true;
          ++i;
        }`,
      to: `        if (i + 1 < src_text.size() && src_text[i + 1] == letter) {
          db = true;
        }`,
    },
    {
      note: "构造：双写分隔符的切片终点算错",
      file: "native/lfw/base/expression.h",
      from: `        const size_t stop = db ? i - 1 : i;`,
      to: `        const size_t stop = db ? i : i - 1;`,
    },
    {
      note: "构造：空切片也建子节点",
      file: "native/lfw/base/expression.h",
      from: `        if (p < stop) {`,
      to: `        if (p <= stop) {`,
    },
    {
      note: "构造：结束条件漏掉字符串尾",
      file: "native/lfw/base/expression.h",
      from: `      } else if (letter == u')' || letter == u'\\0') {`,
      to: `      } else if (letter == u')') {`,
    },
    {
      note: "构造：结束条件漏掉右括号",
      file: "native/lfw/base/expression.h",
      from: `      } else if (letter == u')' || letter == u'\\0') {`,
      to: `      } else if (letter == u'\\0') {`,
    },
    {
      note: "构造：结束前的空切片也建子节点",
      file: "native/lfw/base/expression.h",
      from: `      } else if (letter == u')' || letter == u'\\0') {
        if (p < i) {`,
      to: `      } else if (letter == u')' || letter == u'\\0') {
        if (p <= i) {`,
    },
    {
      note: "构造：this->text 多截一个字符",
      file: "native/lfw/base/expression.h",
      from: `    this->text = src_text.substr(0, i);`,
      to: `    this->text = src_text.substr(0, i + 1);`,
    },

    {
      note: "parse：空文本不报 [empty text]",
      file: "native/lfw/base/expression.h",
      from: `    if (src.empty()) return always_false(u"[empty text]");`,
      to: `    if (false) return always_false(u"[empty text]");`,
    },
    {
      note: "find_op 的二字运算从左侧开始找",
      file: "native/lfw/base/expression.h",
      from: `    if (t.size() >= 2) {
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
    }`,
      to: `    if (t.size() >= 2) {
      for (size_t j = 0; j + 1 < t.size(); ++j) {
        BinOp op{};
        if (bin_op_from_two(t[j], t[j + 1], op)) {
          k = j;
          oplen = 2;
          found = t.substr(j, 2);
          return true;
        }
      }
    }`,
    },
    {
      note: "find_op 的二字运算参数顺序颠倒",
      file: "native/lfw/base/expression.h",
      from: `        if (bin_op_from_two(t[j], t[j + 1], op)) {`,
      to: `        if (bin_op_from_two(t[j + 1], t[j], op)) {`,
    },
    {
      note: "find_op 的单字运算从左侧开始找",
      file: "native/lfw/base/expression.h",
      from: `      for (size_t j = t.size(); j-- > 0;) {
        if (t[j] == u'=' || t[j] == u'<' || t[j] == u'>') {`,
      to: `      for (size_t j = 0; j < t.size(); ++j) {
        if (t[j] == u'=' || t[j] == u'<' || t[j] == u'>') {`,
    },
    {
      note: "find_op 的单字运算不接受等号",
      file: "native/lfw/base/expression.h",
      from: `        if (t[j] == u'=' || t[j] == u'<' || t[j] == u'>') {`,
      to: `        if (t[j] == u'<' || t[j] == u'>') {`,
    },
    {
      note: "parse：操作数左右互换",
      file: "native/lfw/base/expression.h",
      from: `    const std::u16string w1 = src.substr(0, k);
    const std::u16string w2 = src.substr(k + oplen);`,
      to: `    const std::u16string w1 = src.substr(k + oplen);
    const std::u16string w2 = src.substr(0, k);`,
    },
    {
      note: "parse：空操作数的判定从或变成与",
      file: "native/lfw/base/expression.h",
      from: `    if (w1.empty() || w2.empty()) {`,
      to: `    if (w1.empty() && w2.empty()) {`,
    },
    {
      note: "parse：任一操作数为空都不报错",
      file: "native/lfw/base/expression.h",
      from: `    if (w1.empty() || w2.empty()) {
      return always_false(u"[wrong expression: " + src + u"]");
    }`,
      to: `    if (false) {
      return always_false(u"[wrong expression: " + src + u"]");
    }`,
    },
    {
      note: "parse：wrong operator 文案写成 wrong expression",
      file: "native/lfw/base/expression.h",
      from: `    if (!valid) return always_false(u"wrong operator: " + otext);`,
      to: `    if (!valid) return always_false(u"[wrong expression: " + otext + u"]");`,
    },
    {
      note: "parse：总是走常量分支",
      file: "native/lfw/base/expression.h",
      from: `    if (!getter_1 && !getter_2) {`,
      to: `    if (true) {`,
    },
    {
      note: "parse：从不走常量分支",
      file: "native/lfw/base/expression.h",
      from: `    if (!getter_1 && !getter_2) {`,
      to: `    if (false) {`,
    },
    {
      note: "parse：集合语义的常量切分用逗号以外的字符",
      file: "native/lfw/base/expression.h",
      from: `      if (i == s.size() || s[i] == u',') {`,
      to: `      if (s[i] == u';') {`,
    },
    {
      note: "split_commas 丢掉最后一段",
      file: "native/lfw/base/expression.h",
      from: `      if (i == s.size() || s[i] == u',') {
        out.push_back(Value(s.substr(start, i - start)));
        start = i + 1;
      }`,
      to: `      if (i < s.size() && s[i] == u',') {
        out.push_back(Value(s.substr(start, i - start)));
        start = i + 1;
      }`,
    },
    {
      note: "always_false 的常量值写成 true",
      file: "native/lfw/base/expression.h",
      from: `    mode = Mode::kConstant;
    constant = false;
  }`,
      to: `    mode = Mode::kConstant;
    constant = true;
  }`,
    },

    {
      note: "run：常量分支的返回值写成 true",
      file: "native/lfw/base/expression.h",
      from: `    if (mode == Mode::kConstant) {
      result = constant;
      has_result = true;
      return constant;
    }`,
      to: `    if (mode == Mode::kConstant) {
      result = true;
      has_result = true;
      return true;
    }`,
    },
    {
      note: "run：叶子第一个操作数取错名字",
      file: "native/lfw/base/expression.h",
      from: `      const Value v1 = getter_1 ? getter_1(ctx, word_1, op) : lit_1;
      const Value v2 = getter_2 ? getter_2(ctx, word_2, op) : lit_2;`,
      to: `      const Value v1 = getter_1 ? getter_1(ctx, word_2, op) : lit_1;
      const Value v2 = getter_2 ? getter_2(ctx, word_2, op) : lit_2;`,
    },
    {
      note: "run：叶子第二个操作数取错名字",
      file: "native/lfw/base/expression.h",
      from: `      const Value v1 = getter_1 ? getter_1(ctx, word_1, op) : lit_1;
      const Value v2 = getter_2 ? getter_2(ctx, word_2, op) : lit_2;`,
      to: `      const Value v1 = getter_1 ? getter_1(ctx, word_1, op) : lit_1;
      const Value v2 = getter_2 ? getter_2(ctx, word_1, op) : lit_2;`,
    },
    {
      note: "run：叶子记录的两个值互换",
      file: "native/lfw/base/expression.h",
      from: `      val_1 = v1;
      val_2 = v2;
      const bool r = apply_predicate(op, v1, v2);`,
      to: `      val_1 = v2;
      val_2 = v1;
      const bool r = apply_predicate(op, v1, v2);`,
    },
    {
      note: "run：叶子的谓词顺序颠倒",
      file: "native/lfw/base/expression.h",
      from: `      const bool r = apply_predicate(op, v1, v2);`,
      to: `      const bool r = apply_predicate(op, v2, v1);`,
    },
    {
      note: "run：and 组短路条件取反",
      file: "native/lfw/base/expression.h",
      from: `        if (and_set && !and_result) continue;`,
      to: `        if (and_set && and_result) continue;`,
    },
    {
      note: "run：or 组不再短路返回",
      file: "native/lfw/base/expression.h",
      from: `        if (or_result) {
          result = !not_;
          has_result = true;
          return result;
        }`,
      to: `        if (false) {
          result = !not_;
          has_result = true;
          return result;
        }`,
    },
    {
      note: "run：or 组短路返回时的 not 忘取反",
      file: "native/lfw/base/expression.h",
      from: `        if (or_result) {
          result = !not_;
          has_result = true;
          return result;
        }`,
      to: `        if (or_result) {
          result = not_;
          has_result = true;
          return result;
        }`,
    },
    {
      note: "run：最终结果忘取反",
      file: "native/lfw/base/expression.h",
      from: `    result = not_ ? !or_result : or_result;`,
      to: `    result = or_result;`,
    },

    {
      note: "bin_op：!( 被当成 !=",
      file: "native/lfw/defines/bin_op.h",
      from: `  if (a == u'!' && b == u'{') {
    out = BinOp::kNotInclude;`,
      to: `  if (a == u'!' && b == u'{') {
    out = BinOp::kNotEqual;`,
    },
    {
      note: "bin_op：}} 被当成 {{",
      file: "native/lfw/defines/bin_op.h",
      from: `  if (a == u'}' && b == u'}') {
    out = BinOp::kIncludedBy;`,
      to: `  if (a == u'}' && b == u'}') {
    out = BinOp::kInclude;`,
    },
    {
      note: "bin_op：< 不再是一元运算",
      file: "native/lfw/defines/bin_op.h",
      from: `inline bool bin_op_from_one(char16_t c, BinOp& out) {
  if (c == u'<') {`,
      to: `inline bool bin_op_from_one(char16_t c, BinOp& out) {
  if (c == u'^') {`,
    },
    {
      note: "bin_op：< 与 > 映射颠倒",
      file: "native/lfw/defines/bin_op.h",
      from: `  if (c == u'<') {
    out = BinOp::kLess;
    return true;
  }
  if (c == u'>') {
    out = BinOp::kGreater;`,
      to: `  if (c == u'<') {
    out = BinOp::kGreater;
    return true;
  }
  if (c == u'>') {
    out = BinOp::kLess;`,
    },
    {
      note: "bin_op：集合运算名单漏掉 !}",
      file: "native/lfw/defines/bin_op.h",
      from: `inline bool bin_op_is_set(BinOp op) {
  return op == BinOp::kInclude || op == BinOp::kIncludedBy || op == BinOp::kNotInclude ||
         op == BinOp::kNotIncludedBy;
}`,
      to: `inline bool bin_op_is_set(BinOp op) {
  return op == BinOp::kInclude || op == BinOp::kIncludedBy || op == BinOp::kNotInclude;
}`,
    },
    {
      note: "bin_op：集合运算名单漏掉 !{",
      file: "native/lfw/defines/bin_op.h",
      from: `  return op == BinOp::kInclude || op == BinOp::kIncludedBy || op == BinOp::kNotInclude ||
         op == BinOp::kNotIncludedBy;`,
      to: `  return op == BinOp::kInclude || op == BinOp::kIncludedBy || op == BinOp::kNotIncludedBy;`,
    },
    {
      note: "bin_op：所有运算都算集合运算",
      file: "native/lfw/defines/bin_op.h",
      from: `inline bool bin_op_is_set(BinOp op) {
  return op == BinOp::kInclude || op == BinOp::kIncludedBy || op == BinOp::kNotInclude ||
         op == BinOp::kNotIncludedBy;
}`,
      to: `inline bool bin_op_is_set(BinOp op) {
  (void)op;
  return true;
}`,
    },
    {
      note: "bin_op：集合运算都不算集合运算",
      file: "native/lfw/defines/bin_op.h",
      from: `inline bool bin_op_is_set(BinOp op) {
  return op == BinOp::kInclude || op == BinOp::kIncludedBy || op == BinOp::kNotInclude ||
         op == BinOp::kNotIncludedBy;
}`,
      to: `inline bool bin_op_is_set(BinOp op) {
  (void)op;
  return false;
}`,
    },

    {
      note: "predicate：== 用严相等",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kEqual:
      return equals(a, b);`,
      to: `    case BinOp::kEqual:
      return strict_equals(a, b);`,
    },
    {
      note: "predicate：!= 用严相等",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kNotEqual:
      return !equals(a, b);`,
      to: `    case BinOp::kNotEqual:
      return !strict_equals(a, b);`,
    },
    {
      note: "predicate：>= 用 >",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kGreaterOrEqual:
      return ge(a, b);`,
      to: `    case BinOp::kGreaterOrEqual:
      return gt(a, b);`,
    },
    {
      note: "predicate：<= 用 <",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kLessOrEqual:
      return le(a, b);`,
      to: `    case BinOp::kLessOrEqual:
      return lt(a, b);`,
    },
    {
      note: "predicate：< 用 <=",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kLess:
      return lt(a, b);`,
      to: `    case BinOp::kLess:
      return le(a, b);`,
    },
    {
      note: "predicate：> 用 >=",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kGreater:
      return gt(a, b);`,
      to: `    case BinOp::kGreater:
      return ge(a, b);`,
    },
    {
      note: "predicate：{{ 的两个参数互换",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kInclude:
      return a_included_b(a, b);`,
      to: `    case BinOp::kInclude:
      return a_included_b(b, a);`,
    },
    {
      note: "predicate：}} 的两个参数互换",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kIncludedBy:
      return a_included_b(b, a);`,
      to: `    case BinOp::kIncludedBy:
      return a_included_b(a, b);`,
    },
    {
      note: "predicate：!{ 忘取反",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kNotInclude:
      return !a_included_b(a, b);`,
      to: `    case BinOp::kNotInclude:
      return a_included_b(a, b);`,
    },
    {
      note: "predicate：!} 忘取反",
      file: "native/lfw/base/predicate.cpp",
      from: `    case BinOp::kNotIncludedBy:
      return !a_included_b(b, a);`,
      to: `    case BinOp::kNotIncludedBy:
      return a_included_b(b, a);`,
    },
    {
      note: "a_included_b：空 b 返回 false",
      file: "native/lfw/base/predicate.cpp",
      from: `    if (arr->empty()) return true;`,
      to: `    if (arr->empty()) return false;`,
    },
    {
      note: "a_included_b：含空 b 的 all 判定恒假",
      file: "native/lfw/base/predicate.cpp",
      from: `    for (size_t i = 0; i < arr->size(); ++i) {
      if (index_of(a, arr->at(i)) < 0) return false;
    }
    return true;`,
      to: `    for (size_t i = 0; i < arr->size(); ++i) {
      if (index_of(a, arr->at(i)) < 0) return false;
    }
    return false;`,
    },
    {
      note: "a_included_b：包含判定用 <= 0",
      file: "native/lfw/base/predicate.cpp",
      from: `      if (index_of(a, arr->at(i)) < 0) return false;`,
      to: `      if (index_of(a, arr->at(i)) <= 0) return false;`,
    },
    {
      note: "index_of：数组分支用宽松相等",
      file: "native/lfw/base/predicate.cpp",
      from: `      if (strict_equals(arr->at(i), item)) return static_cast<long>(i);`,
      to: `      if (equals(arr->at(i), item)) return static_cast<long>(i);`,
    },
    {
      note: "index_of：数组分支找不到时返回 0",
      file: "native/lfw/base/predicate.cpp",
      from: `    for (size_t i = 0; i < arr->size(); ++i) {
      if (strict_equals(arr->at(i), item)) return static_cast<long>(i);
    }
    return -1;`,
      to: `    for (size_t i = 0; i < arr->size(); ++i) {
      if (strict_equals(arr->at(i), item)) return static_cast<long>(i);
    }
    return 0;`,
    },
    {
      note: "index_of：字符串分支的 needle 恒为空",
      file: "native/lfw/base/predicate.cpp",
      from: `    const std::u16string needle = to_string(item);`,
      to: `    const std::u16string needle;`,
    },
    {
      note: "index_of：空 needle 返回 -1",
      file: "native/lfw/base/predicate.cpp",
      from: `    if (needle.empty()) return 0;`,
      to: `    if (needle.empty()) return -1;`,
    },
    {
      note: "index_of：needle 过长时返回 -1 的判定用 >=",
      file: "native/lfw/base/predicate.cpp",
      from: `    if (needle.size() > s->size()) return -1;`,
      to: `    if (needle.size() >= s->size()) return -1;`,
    },
    {
      note: "index_of：字符串分支的扫描上界用 <",
      file: "native/lfw/base/predicate.cpp",
      from: `    for (size_t i = 0; i + needle.size() <= s->size(); ++i) {`,
      to: `    for (size_t i = 0; i + needle.size() < s->size(); ++i) {`,
    },
    {
      note: "index_of：字符串分支的比较取反",
      file: "native/lfw/base/predicate.cpp",
      from: `      if (s->compare(i, needle.size(), needle) == 0) return static_cast<long>(i);`,
      to: `      if (s->compare(i, needle.size(), needle) != 0) return static_cast<long>(i);`,
    },
  ],
};
