// 已删 / 未写的等价变异（不要重加）：
// 1. `is_array_index` 里 `k.size() > 10` 的长度上限：单看"值 > 4294967294"似乎能兜住，
//    但去掉它后 20 位的键会在 `v = v * 10 + d` 里**溢出 uint64**，溢出后可能落到
//    ≤ 4294967294 ⇒ 仍可观察（用例键 `18446744073709551616` = 2^64 ⇒ 溢出成 0）。
//    这条**保留**，只是提醒：要观察它必须用溢出样例。
// 2. `as_array` / `as_object` 把 `p && *p ? p->get()` 写成 `p ? p->get()`：
//    空的 `shared_ptr` 调 `get()` 仍然返回 nullptr ⇒ 等价，不写。
// 3. `Object::remove` 的 `return _ints.erase(idx) != 0;` 写成 `> 0`：无符号比较，等价。
// 4. `kind_of` 把"对象"归到 V_ARR：`equals` 里两侧同为对象时仍走 `ka == kb` 的严格比较，
//    而对象与数组混比又落回 `strict_equals` 的指针比较 ⇒ 不可观察，不写。
// 5. `strict_equals` 末尾的 `return false;` 改成 `true`：**死代码**。
//    走到那里要求 `a` 既不是 undefined/null/bool/double/string，也不是数组/对象
//    —— 变体的 7 个分支已覆盖全部种类（只有人为塞进空 `shared_ptr` 才能到达），
//    而 harness 的 `parse_value` 造不出空 `shared_ptr` ⇒ 不可达。
export default {
  subject: "value",
  mutations: [
    {
      note: "kind_of：null 归成 undefined",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<NullTag>(v)) return V_NULL;`,
      to: `  if (std::holds_alternative<NullTag>(v)) return V_UNDEF;`,
    },
    {
      note: "kind_of：bool 归成 number",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<bool>(v)) return V_BOOL;`,
      to: `  if (std::holds_alternative<bool>(v)) return V_NUM;`,
    },
    {
      note: "kind_of：字符串归成 number",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<std::u16string>(v)) return V_STR;`,
      to: `  if (std::holds_alternative<std::u16string>(v)) return V_NUM;`,
    },

    {
      note: "is_array_index：前导零也当整数键",
      file: "native/lfw/core/value.cpp",
      from: `  if (k.size() > 1 && k[0] == u'0') return false;`,
      to: `  if (false) return false;`,
    },
    {
      note: "is_array_index：上界从 4294967294 放到 4294967295",
      file: "native/lfw/core/value.cpp",
      from: `  if (v > 4294967294ull) return false;`,
      to: `  if (v > 4294967295ull) return false;`,
    },
    {
      note: "is_array_index：不校验非数字字符",
      file: "native/lfw/core/value.cpp",
      from: `    if (c < u'0' || c > u'9') return false;`,
      to: `    if (c < u'0') return false;`,
    },
    {
      note: "is_array_index：长度上限去掉（溢出路径）",
      file: "native/lfw/core/value.cpp",
      from: `  if (k.empty() || k.size() > 10) return false;`,
      to: `  if (k.empty()) return false;`,
    },
    {
      note: "is_array_index：下标偏移 1",
      file: "native/lfw/core/value.cpp",
      from: `  out = static_cast<uint32_t>(v);
  return true;`,
      to: `  out = static_cast<uint32_t>(v) + 1;
  return true;`,
    },

    {
      note: "truthy：undefined 为真",
      file: "native/lfw/core/value.cpp",
      from: `bool truthy(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return false;`,
      to: `bool truthy(const Value& v) {
  if (std::holds_alternative<std::monostate>(v)) return true;`,
    },
    {
      note: "truthy：null 为真",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<NullTag>(v)) return false;
  if (const bool* b = std::get_if<bool>(&v)) return *b;`,
      to: `  if (std::holds_alternative<NullTag>(v)) return true;
  if (const bool* b = std::get_if<bool>(&v)) return *b;`,
    },
    {
      note: "truthy：NaN 为真",
      file: "native/lfw/core/value.cpp",
      from: `  if (const double* d = std::get_if<double>(&v)) return *d != 0.0 && !std::isnan(*d);`,
      to: `  if (const double* d = std::get_if<double>(&v)) return *d != 0.0;`,
    },
    {
      note: "truthy：负数算假",
      file: "native/lfw/core/value.cpp",
      from: `  if (const double* d = std::get_if<double>(&v)) return *d != 0.0 && !std::isnan(*d);`,
      to: `  if (const double* d = std::get_if<double>(&v)) return *d > 0.0;`,
    },
    {
      note: "truthy：空串为真",
      file: "native/lfw/core/value.cpp",
      from: `  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return !s->empty();`,
      to: `  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return true;`,
    },
    {
      note: "truthy：数组/对象为假",
      file: "native/lfw/core/value.cpp",
      from: `  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return !s->empty();
  return true;`,
      to: `  if (const std::u16string* s = std::get_if<std::u16string>(&v)) return !s->empty();
  return false;`,
    },

    {
      note: "type_of：undefined 说成 null",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<std::monostate>(v)) return "undefined";`,
      to: `  if (std::holds_alternative<std::monostate>(v)) return "null";`,
    },
    {
      note: "type_of：null 说成 null",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<NullTag>(v)) return "object";
  if (std::holds_alternative<bool>(v)) return "boolean";`,
      to: `  if (std::holds_alternative<NullTag>(v)) return "null";
  if (std::holds_alternative<bool>(v)) return "boolean";`,
    },
    {
      note: "type_of：bool 文案写错",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<bool>(v)) return "boolean";`,
      to: `  if (std::holds_alternative<bool>(v)) return "bool";`,
    },
    {
      note: "type_of：number 文案写错",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<double>(v)) return "number";`,
      to: `  if (std::holds_alternative<double>(v)) return "double";`,
    },
    {
      note: "type_of：string 文案写错",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<std::u16string>(v)) return "string";
  return "object";`,
      to: `  if (std::holds_alternative<std::u16string>(v)) return "str";
  return "object";`,
    },
    {
      note: "type_of：数组说成 array",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<std::u16string>(v)) return "string";
  return "object";`,
      to: `  if (std::holds_alternative<std::u16string>(v)) return "string";
  return "array";`,
    },

    {
      note: "is_array：拿 as_object 判定",
      file: "native/lfw/core/value.cpp",
      from: `bool is_array(const Value& v) { return as_array(v) != nullptr; }`,
      to: `bool is_array(const Value& v) { return as_object(v) != nullptr; }`,
    },

    {
      note: "Object::has：缺键也说有",
      file: "native/lfw/core/value.cpp",
      from: `bool Object::has(const std::u16string& key) const { return get(key) != nullptr; }`,
      to: `bool Object::has(const std::u16string& key) const { return true; }`,
    },
    {
      note: "Object::get：整数键查错下标",
      file: "native/lfw/core/value.cpp",
      from: `    const auto it = _ints.find(idx);
    return it == _ints.end() ? nullptr : &it->second;`,
      to: `    const auto it = _ints.find(idx + 1);
    return it == _ints.end() ? nullptr : &it->second;`,
    },
    {
      note: "Object::set：整数键写错下标",
      file: "native/lfw/core/value.cpp",
      from: `    _ints[idx] = std::move(v);
    return;`,
      to: `    _ints[idx + 1] = std::move(v);
    return;`,
    },
    {
      note: "Object::set：同名键永远追加",
      file: "native/lfw/core/value.cpp",
      from: `  for (auto& kv : _strs) {
    if (kv.first == key) {
      kv.second = std::move(v);
      return;
    }
  }
  _strs.emplace_back(key, std::move(v));`,
      to: `  _strs.emplace_back(key, std::move(v));`,
    },
    {
      note: "Object::set：新键插到最前",
      file: "native/lfw/core/value.cpp",
      from: `  _strs.emplace_back(key, std::move(v));`,
      to: `  _strs.emplace(_strs.begin(), key, std::move(v));`,
    },
    {
      note: "Object::remove：整数键删错下标",
      file: "native/lfw/core/value.cpp",
      from: `  if (is_array_index(key, idx)) return _ints.erase(idx) != 0;`,
      to: `  if (is_array_index(key, idx)) return _ints.erase(idx + 1) != 0;`,
    },
    {
      note: "Object::remove：没删掉也说删了",
      file: "native/lfw/core/value.cpp",
      from: `      _strs.erase(_strs.begin() + static_cast<std::ptrdiff_t>(i));
      return true;
    }
  }
  return false;`,
      to: `      _strs.erase(_strs.begin() + static_cast<std::ptrdiff_t>(i));
      return true;
    }
  }
  return true;`,
    },
    {
      note: "Object::remove：删首元素而不是命中项",
      file: "native/lfw/core/value.cpp",
      from: `      _strs.erase(_strs.begin() + static_cast<std::ptrdiff_t>(i));`,
      to: `      _strs.erase(_strs.begin());`,
    },
    {
      note: "Object::keys：字符串键排在整数键之前",
      file: "native/lfw/core/value.cpp",
      from: `  for (const auto& kv : _ints) out.push_back(number_to_string(static_cast<double>(kv.first)));
  for (const auto& kv : _strs) out.push_back(kv.first);`,
      to: `  for (const auto& kv : _strs) out.push_back(kv.first);
  for (const auto& kv : _ints) out.push_back(number_to_string(static_cast<double>(kv.first)));`,
    },
    {
      note: "Object::keys：整数键名字偏移 1",
      file: "native/lfw/core/value.cpp",
      from: `  for (const auto& kv : _ints) out.push_back(number_to_string(static_cast<double>(kv.first)));`,
      to: `  for (const auto& kv : _ints) {
    out.push_back(number_to_string(static_cast<double>(kv.first + 1)));
  }`,
    },

    {
      note: "array_join：分隔符条件取反",
      file: "native/lfw/core/value.cpp",
      from: `    if (i != 0) out.push_back(u',');`,
      to: `    if (i == 0) out.push_back(u',');`,
    },
    {
      note: "array_join：undefined 元素写出 undefined",
      file: "native/lfw/core/value.cpp",
      from: `    if (std::holds_alternative<std::monostate>(item)) continue;`,
      to: `    if (std::holds_alternative<std::monostate>(item)) out += u"undefined";`,
    },
    {
      note: "array_join：null 元素写出 null",
      file: "native/lfw/core/value.cpp",
      from: `    if (std::holds_alternative<NullTag>(item)) continue;`,
      to: `    if (std::holds_alternative<NullTag>(item)) out += u"null";`,
    },
    {
      note: "array_join：不累加而是覆盖",
      file: "native/lfw/core/value.cpp",
      from: `    out += to_string(item);`,
      to: `    out = to_string(item);`,
    },

    {
      note: "to_string：undefined 写成 null 字面量",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<std::monostate>(v)) return u"undefined";`,
      to: `  if (std::holds_alternative<std::monostate>(v)) return u"null";`,
    },
    {
      note: "to_string：null 写成 undefined 字面量",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<NullTag>(v)) return u"null";
  if (const bool* b = std::get_if<bool>(&v)) return *b ? u"true" : u"false";`,
      to: `  if (std::holds_alternative<NullTag>(v)) return u"undefined";
  if (const bool* b = std::get_if<bool>(&v)) return *b ? u"true" : u"false";`,
    },
    {
      note: "to_string：bool 写成 1/0",
      file: "native/lfw/core/value.cpp",
      from: `  if (const bool* b = std::get_if<bool>(&v)) return *b ? u"true" : u"false";`,
      to: `  if (const bool* b = std::get_if<bool>(&v)) return *b ? u"1" : u"0";`,
    },
    {
      note: "to_string：数字写成空串",
      file: "native/lfw/core/value.cpp",
      from: `  if (const double* d = std::get_if<double>(&v)) return number_to_string(*d);`,
      to: `  if (const double* d = std::get_if<double>(&v)) return std::u16string();`,
    },
    {
      note: "to_string：对象文案写错",
      file: "native/lfw/core/value.cpp",
      from: `  return u"[object Object]";
}

Value to_primitive(const Value& v) {`,
      to: `  return u"[Object]";
}

Value to_primitive(const Value& v) {`,
    },

    {
      note: "to_primitive：对象文案写错",
      file: "native/lfw/core/value.cpp",
      from: `  if (as_object(v) != nullptr) return Value(std::u16string(u"[object Object]"));`,
      to: `  if (as_object(v) != nullptr) return Value(std::u16string(u"[object object]"));`,
    },
    {
      note: "to_primitive：数组不拼串",
      file: "native/lfw/core/value.cpp",
      from: `  if (const Array* a = as_array(v)) return Value(array_join(*a));`,
      to: `  if (const Array* a = as_array(v)) return Value(std::u16string());`,
    },

    {
      note: "to_number：null 给出 NaN",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<NullTag>(v)) return 0.0;`,
      to: `  if (std::holds_alternative<NullTag>(v)) return std::numeric_limits<double>::quiet_NaN();`,
    },
    {
      note: "to_number：bool 的 1/0 取反",
      file: "native/lfw/core/value.cpp",
      from: `  if (const bool* b = std::get_if<bool>(&v)) return *b ? 1.0 : 0.0;`,
      to: `  if (const bool* b = std::get_if<bool>(&v)) return *b ? 0.0 : 1.0;`,
    },
    {
      note: "to_number：数组不走拼串",
      file: "native/lfw/core/value.cpp",
      from: `  if (const Array* a = as_array(v)) return string_to_number(array_join(*a));`,
      to: `  if (const Array* a = as_array(v)) return 0.0;`,
    },
    {
      note: "to_number：对象按 0 而不是按文案",
      file: "native/lfw/core/value.cpp",
      from: `  return string_to_number(u"[object Object]");`,
      to: `  return 0.0;`,
    },

    {
      note: "strict_equals：undefined 与任何值相等",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<std::monostate>(a)) return std::holds_alternative<std::monostate>(b);`,
      to: `  if (std::holds_alternative<std::monostate>(a)) return true;`,
    },
    {
      note: "strict_equals：null 与任何值相等",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<NullTag>(a)) return std::holds_alternative<NullTag>(b);`,
      to: `  if (std::holds_alternative<NullTag>(a)) return true;`,
    },
    {
      note: "strict_equals：只看类型不看布尔值",
      file: "native/lfw/core/value.cpp",
      from: `    const bool* y = std::get_if<bool>(&b);
    return y != nullptr && *x == *y;`,
      to: `    const bool* y = std::get_if<bool>(&b);
    return y != nullptr;`,
    },
    {
      note: "strict_equals：数字比较取反",
      file: "native/lfw/core/value.cpp",
      from: `    const double* y = std::get_if<double>(&b);
    return y != nullptr && *x == *y;`,
      to: `    const double* y = std::get_if<double>(&b);
    return y != nullptr && *x != *y;`,
    },
    {
      note: "strict_equals：字符串比较取反",
      file: "native/lfw/core/value.cpp",
      from: `    const std::u16string* y = std::get_if<std::u16string>(&b);
    return y != nullptr && *x == *y;`,
      to: `    const std::u16string* y = std::get_if<std::u16string>(&b);
    return y != nullptr && *x != *y;`,
    },
    {
      note: "strict_equals：数组只看另一边是不是数组",
      file: "native/lfw/core/value.cpp",
      from: `  if (const Array* xa = as_array(a)) return xa == as_array(b);`,
      to: `  if (const Array* xa = as_array(a)) return as_array(b) != nullptr;`,
    },
    {
      note: "strict_equals：对象只看另一边是不是对象",
      file: "native/lfw/core/value.cpp",
      from: `  if (const Object* xo = as_object(a)) return xo == as_object(b);`,
      to: `  if (const Object* xo = as_object(a)) return as_object(b) != nullptr;`,
    },
    {
      note: "strict_equals：右边是 undefined 时返回真",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<std::monostate>(b)) return false;`,
      to: `  if (std::holds_alternative<std::monostate>(b)) return true;`,
    },
    {
      note: "strict_equals：右边是 null 时返回真",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::holds_alternative<NullTag>(b)) return false;`,
      to: `  if (std::holds_alternative<NullTag>(b)) return true;`,
    },

    {
      note: "equals：同类型短路去掉",
      file: "native/lfw/core/value.cpp",
      from: `  if (ka == kb) return strict_equals(a, b);`,
      to: `  if (false) return strict_equals(a, b);`,
    },
    {
      note: "equals：undefined 与 null 不等",
      file: "native/lfw/core/value.cpp",
      from: `  if ((ka == V_UNDEF && kb == V_NULL) || (ka == V_NULL && kb == V_UNDEF)) return true;`,
      to: `  if (false) return true;`,
    },
    {
      note: "equals：左布尔不折算",
      file: "native/lfw/core/value.cpp",
      from: `  if (ka == V_BOOL) return equals(Value(to_number(a)), b);`,
      to: `  if (ka == V_BOOL) return true;`,
    },
    {
      note: "equals：右布尔不折算",
      file: "native/lfw/core/value.cpp",
      from: `  if (kb == V_BOOL) return equals(a, Value(to_number(b)));`,
      to: `  if (kb == V_BOOL) return true;`,
    },
    {
      note: "equals：数字与字符串不折算",
      file: "native/lfw/core/value.cpp",
      from: `  if (ka == V_NUM && kb == V_STR) return strict_equals(a, Value(to_number(b)));`,
      to: `  if (ka == V_NUM && kb == V_STR) return true;`,
    },
    {
      note: "equals：字符串与数字不折算",
      file: "native/lfw/core/value.cpp",
      from: `  if (ka == V_STR && kb == V_NUM) return strict_equals(Value(to_number(a)), b);`,
      to: `  if (ka == V_STR && kb == V_NUM) return true;`,
    },
    {
      note: "equals：左侧集合不 ToPrimitive",
      file: "native/lfw/core/value.cpp",
      from: `  if ((ka == V_ARR || ka == V_OBJ) && (kb == V_NUM || kb == V_STR)) return equals(to_primitive(a), b);`,
      to: `  if ((ka == V_ARR || ka == V_OBJ) && (kb == V_NUM || kb == V_STR)) return false;`,
    },
    {
      note: "equals：右侧集合不 ToPrimitive",
      file: "native/lfw/core/value.cpp",
      from: `  if ((kb == V_ARR || kb == V_OBJ) && (ka == V_NUM || ka == V_STR)) return equals(a, to_primitive(b));`,
      to: `  if ((kb == V_ARR || kb == V_OBJ) && (ka == V_NUM || ka == V_STR)) return false;`,
    },
    {
      note: "equals：兜底返回真",
      file: "native/lfw/core/value.cpp",
      from: `  if ((kb == V_ARR || kb == V_OBJ) && (ka == V_NUM || ka == V_STR)) return equals(a, to_primitive(b));

  return false;
}`,
      to: `  if ((kb == V_ARR || kb == V_OBJ) && (ka == V_NUM || ka == V_STR)) return equals(a, to_primitive(b));

  return true;
}`,
    },

    {
      note: "less_than：字符串比较取反",
      file: "native/lfw/core/value.cpp",
      from: `  if (sx != nullptr && sy != nullptr) return *sx < *sy;`,
      to: `  if (sx != nullptr && sy != nullptr) return *sx > *sy;`,
    },
    {
      note: "less_than：NaN 不再返回 nullopt",
      file: "native/lfw/core/value.cpp",
      from: `  if (std::isnan(nx) || std::isnan(ny)) return std::nullopt;`,
      to: `  if (false) return std::nullopt;`,
    },
    {
      note: "less_than：数值比较用 <=",
      file: "native/lfw/core/value.cpp",
      from: `  return nx < ny;
}`,
      to: `  return nx <= ny;
}`,
    },
    {
      note: "less_than：不先 ToPrimitive",
      file: "native/lfw/core/value.cpp",
      from: `  const Value px = to_primitive(a);
  const Value py = to_primitive(b);`,
      to: `  const Value px = a;
  const Value py = b;`,
    },

    {
      note: "gt：两个操作数写反",
      file: "native/lfw/core/value.cpp",
      from: `bool gt(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(b, a);
  return r.has_value() && *r;
}`,
      to: `bool gt(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(a, b);
  return r.has_value() && *r;
}`,
    },
    {
      note: "le：两个操作数写反",
      file: "native/lfw/core/value.cpp",
      from: `bool le(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(b, a);
  return r.has_value() && !*r;
}`,
      to: `bool le(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(a, b);
  return r.has_value() && !*r;
}`,
    },
    {
      note: "ge：忘取反",
      file: "native/lfw/core/value.cpp",
      from: `bool ge(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(a, b);
  return r.has_value() && !*r;
}`,
      to: `bool ge(const Value& a, const Value& b) {
  const std::optional<bool> r = less_than(a, b);
  return r.has_value() && *r;
}`,
    },
  ],
};
