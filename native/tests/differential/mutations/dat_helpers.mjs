export default {
  subject: "dat_helpers",
  mutations: [
    {
      note: "take 只取值不删除",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  Value ret = v != nullptr ? *v : Value();
  any.remove(key);
  return ret;`,
      to: `  return v != nullptr ? *v : Value();`,
    },
    {
      note: "take_str 在值不是字符串时也删除",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  const std::u16string* s = std::get_if<std::u16string>(v);
  if (s == nullptr) return std::nullopt;
  const std::u16string ret = *s;
  any.remove(key);`,
      to: `  const std::u16string* s = std::get_if<std::u16string>(v);
  const std::u16string ret = s != nullptr ? *s : std::u16string();
  any.remove(key);
  if (s == nullptr) return std::nullopt;`,
    },
    {
      note: "take_num 不再过滤 0xCDCDCDCD 哨兵值",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  if (n == -842150451) return std::nullopt;`,
      to: `  if (false) return std::nullopt;`,
    },
    {
      note: "is_num 不再要求有限（Infinity 也算数字）",
      file: "native/lfw/utils/type_check.h",
      from: `inline bool is_num(double d) { return !std::isnan(d) && std::isfinite(d); }`,
      to: `inline bool is_num(double d) { return !std::isnan(d); }`,
    },
    {
      note: "is_positive 允许 0",
      file: "native/lfw/utils/type_check.h",
      from: `  return d != nullptr && is_num(*d) && *d > 0;`,
      to: `  return d != nullptr && is_num(*d) && *d >= 0;`,
    },
    {
      note: "not_zero_num 允许 0",
      file: "native/lfw/utils/type_check.h",
      from: `  return d != nullptr && is_num(*d) && *d != 0;`,
      to: `  return d != nullptr && is_num(*d);`,
    },
    {
      note: "frame.mp 的 ±1000 边界写成闭区间",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  if (mp < 1000 && mp > -1000) return {mp, 0};`,
      to: `  if (mp <= 1000 && mp >= -1000) return {mp, 0};`,
    },
    {
      note: "frame.mp 拆 HP 时除法用 1000",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  const double hp = (mp - rem) / 100;`,
      to: `  const double hp = (mp - rem) / 1000;`,
    },
    {
      note: "fixed_float 平局一律进位（丢掉 fma 精确残差判断）",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  } else if (frac == 0.5) {
    if (!(err < 0)) k += 1;
  }`,
      to: `  } else if (frac == 0.5) {
    k += 1;
  }`,
    },
    {
      note: "fixed_float 直接用乘积四舍五入（回到不可靠的 x*scale）",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  const double p = a * scale;
  const double err = std::fma(a, scale, -p);`,
      to: `  const double p = a * scale;
  const double err = 0;`,
    },
    {
      note: "fixed_float 丢掉负数符号处理",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  return n < 0 ? -out : out;`,
      to: `  return out;`,
    },
    {
      note: "fixed_float 的 1e21 阈值写成开区间",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  if (std::abs(n) >= 1e21) return n;`,
      to: `  if (std::abs(n) > 1e21) return n;`,
    },
    {
      note: "find_float 把任何数字都当成非整数",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  if (const double* d = std::get_if<double>(&v)) return {!is_int(*d), path};`,
      to: `  if (const double* d = std::get_if<double>(&v)) {\n    (void)d;\n    return {true, path};\n  }`,
    },
    {
      note: "find_float 的数组下标路径分隔符写错",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `            find_float((*a)->at(k), path + u"/" + number_to_string(static_cast<double>(k)));`,
      to: `            find_float((*a)->at(k), path + u"#" + number_to_string(static_cast<double>(k)));`,
    },
    {
      note: "copy_bdy_info 改用 JSON5（NaN/Infinity 不再变 null）",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `Value copy_bdy_info(const Value& src, const Object& edit) { return merge_copy(src, false, edit); }`,
      to: `Value copy_bdy_info(const Value& src, const Object& edit) { return merge_copy(src, true, edit); }`,
    },
    {
      note: "copy_itr_info 改用 JSON",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `Value copy_itr_info(const Value& src, const Object& edit) { return merge_copy(src, true, edit); }`,
      to: `Value copy_itr_info(const Value& src, const Object& edit) { return merge_copy(src, false, edit); }`,
    },
    {
      note: "合并时 edit 被 src 覆盖（顺序反了）",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  for (const std::u16string& k : edit.keys()) {
    const Value* v = edit.get(k);
    if (v != nullptr) out.set(k, *v);
  }`,
      to: `  (void)edit;`,
    },
    {
      note: "set_hit_flag 先写 name 再写 flag（键序变了）",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  info.set(u"hit_flag", value);
  info.set(u"hit_flag_name", Value(defines::get_hit_flag_name(value)));`,
      to: `  info.set(u"hit_flag_name", Value(defines::get_hit_flag_name(value)));
  info.set(u"hit_flag", value);`,
    },
    {
      note: "set_bdy_kind 的 kind_name 用短名而不是全名",
      file: "native/lfw/dat_translator/helpers.cpp",
      from: `  bdy.set(u"kind_name", Value(defines::bdy_kind_full_name(kind)));`,
      to: `  bdy.set(u"kind_name", Value(to_string(defines::bdy_kind_name(kind))));`,
    },
  ],
};
