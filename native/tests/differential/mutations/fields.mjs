export default {
  subject: "fields",
  mutations: [
    {
      note: "w: 第二个字符串参数不进 desc 分支",
      file: "native/lfw/fields.cpp",
      from: `      } else if (s == 1) {`,
      to: `      } else if (s == 2) {`,
    },
    {
      note: "w: desc 追加改成覆盖",
      file: "native/lfw/fields.cpp",
      from: `        ret->set(u"desc", Value(to_string(d != nullptr ? *d : Value()) + u"\\n" + *p));`,
      to: `        ret->set(u"desc", Value(*p));`,
    },
    {
      note: "fields: order 从 1 开始",
      file: "native/lfw/fields.cpp",
      from: `    info->set(u"order", Value(order));`,
      to: `    info->set(u"order", Value(order + 1));`,
    },
    {
      note: "fields: 先补 key/order 再合并源对象",
      file: "native/lfw/fields.cpp",
      from: `    auto info = std::make_shared<Object>();
    if (v != nullptr) assign_all(*info, *v);
    info->set(u"key", Value(k));
    info->set(u"order", Value(order));`,
      to: `    auto info = std::make_shared<Object>();
    info->set(u"key", Value(k));
    info->set(u"order", Value(order));
    if (v != nullptr) assign_all(*info, *v);`,
    },
    {
      note: "reorder: 排序方向反过来",
      file: "native/lfw/fields.cpp",
      from: `                     return na - nb < 0;`,
      to: `                     return nb - na < 0;`,
    },
    {
      note: "reorder: order 为 undefined 也算已知",
      file: "native/lfw/fields.cpp",
      from: `    if (order != nullptr && !is_undefined(*order)) known.push_back(k);`,
      to: `    if (order != nullptr) known.push_back(k);`,
    },
    {
      note: "reorder: 不先清空就重排（set 原地更新）",
      file: "native/lfw/fields.cpp",
      from: `  for (const std::u16string& k : all_keys) o->remove(k);
`,
      to: ``,
    },
    {
      note: "reorder: known 初始顺序反转",
      file: "native/lfw/fields.cpp",
      from: `    if (order != nullptr && !is_undefined(*order)) known.push_back(k);`,
      to: `    if (order != nullptr && !is_undefined(*order)) known.insert(known.begin(), k);`,
    },
    {
      note: "to_array: 只看 null 不看 undefined",
      file: "native/lfw/fields.cpp",
      from: `  if (is_null(v) || is_undefined(v)) return Value(std::make_shared<Array>());`,
      to: `  if (is_null(v)) return Value(std::make_shared<Array>());`,
    },
    {
      note: "to_array: 数组也拷贝一份",
      file: "native/lfw/fields.cpp",
      from: `  if (is_array(v)) return v;`,
      to: `  if (is_array(v)) return Value(std::make_shared<Array>(as_array(v)->items()));`,
    },
    {
      note: "assign: 字符串索引键全取第 0 个",
      file: "native/lfw/fields.cpp",
      from: `      target.set(number_to_string(static_cast<double>(i)), Value(std::u16string(1, (*s)[i])));`,
      to: `      target.set(number_to_string(static_cast<double>(i)), Value(std::u16string(1, (*s)[0])));`,
    },
    {
      note: "validate: nullable 不看真值",
      file: "native/lfw/fields.cpp",
      from: `    if (nullable_v != nullptr && truthy(*nullable_v)) return true;`,
      to: `    if (nullable_v != nullptr) return true;`,
    },
    {
      note: "validate: array === 'auto' 失效",
      file: "native/lfw/fields.cpp",
      from: `  const bool array_is_auto = array_v != nullptr && is_str(*array_v) && as_str(*array_v) == u"auto";`,
      to: `  const bool array_is_auto = false;`,
    },
    {
      note: "validate: array === true 用真值判定",
      file: "native/lfw/fields.cpp",
      from: `  const bool array_is_true = array_v != nullptr && is_bool_true(*array_v);`,
      to: `  const bool array_is_true = array_v != nullptr && truthy(*array_v);`,
    },
    {
      note: "validate: int 丢掉整数判定",
      file: "native/lfw/fields.cpp",
      from: `    if (!is_number(value) || !is_integer_number(value)) {
      push_error(errors, u"[validate_fields] " + path + u" 应为整数，实际为 " + json_text(value));`,
      to: `    if (!is_number(value)) {
      push_error(errors, u"[validate_fields] " + path + u" 应为整数，实际为 " + json_text(value));`,
    },
    {
      note: "validate: int 的 min 用 <=",
      file: "native/lfw/fields.cpp",
      from: `      push_error(errors, u"[validate_fields] " + path + u" 应为整数，实际为 " + json_text(value));
      return false;
    }
    if (present(field, u"min") && lt(value, *min_v)) {`,
      to: `      push_error(errors, u"[validate_fields] " + path + u" 应为整数，实际为 " + json_text(value));
      return false;
    }
    if (present(field, u"min") && le(value, *min_v)) {`,
    },
    {
      note: "validate: options 用宽松相等",
      file: "native/lfw/fields.cpp",
      from: `        if (ov != nullptr && strict_equals(*ov, value)) found = true;`,
      to: `        if (ov != nullptr && equals(*ov, value)) found = true;`,
    },
    {
      note: "validate: options 列表里 undefined 用 json_text（应该用 join 语义）",
      file: "native/lfw/fields.cpp",
      from: `        list += json_join_item(ov != nullptr ? *ov : missing);`,
      to: `        list += json_text(ov != nullptr ? *ov : missing);`,
    },
    {
      note: "validate: 未知字段不告警",
      file: "native/lfw/fields.cpp",
      from: `    if (map == nullptr || !map->has(key)) {`,
      to: `    if (false) {`,
    },
  ],
};
