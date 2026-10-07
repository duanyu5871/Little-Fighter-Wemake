// `utils/schema`（SchemaValidator / check_stage_info）的变异档。
//
// 用例：`cases/schema/{terrain,real,adhoc}.txt`（任一锁住即可）。
//
// 有意不覆盖（不可观察 / 不建形）：
//   * 类类型分支（`typeof type === 'function'` ⇒ default 分支的「值必须是字符串」、
//     `Object.defineProperty` 惰性属性、`instance_getter` / `instance_setter` 钩子）——
//     端口未建形（Value 装不下函数；schema 表是纯数据）。
//   * 浅拷贝的**别名语义**：数组项 / 对象属性的 `[...x]` 拷贝与「成功才写回」在渲染台面上
//     同观（换的拷贝内容相同）⇒ 只有「拷贝成空」这类**改内容**的变异才可观察。
//   * `schema.oneof` 不是数组时 TS 会 TypeError —— 端口按没有 oneof 处理（同观）。
export default {
  subject: "schema",
  cases: ["terrain", "real", "adhoc"],
  mutations: [
    // ------------------------------------------------------------ nullish 早退
    {
      note: "nullish 接受条件：漏掉 `type == 'null'` 那一半",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    const bool accept =
        truthy(field(schema, u"nullable")) || strict_equals(type, Value(u"null"));`,
      to: `    const bool accept =
        truthy(field(schema, u"nullable"));`,
    },
    {
      note: "nullish 接受条件：漏掉 `nullable` 那一半",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    const bool accept =
        truthy(field(schema, u"nullable")) || strict_equals(type, Value(u"null"));`,
      to: `    const bool accept =
        strict_equals(type, Value(u"null"));`,
    },
    {
      note: "is_nullish：只认 undefined（丢 null）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);`,
      to: `  return std::holds_alternative<std::monostate>(v);`,
    },
    {
      note: "is_nullish：只认 null（丢 undefined）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);`,
      to: `  return std::holds_alternative<NullTag>(v);`,
    },
    {
      note: "type 读成 `key`（所有分派落空）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  const Value& type = field(schema, u"type");`,
      to: `  const Value& type = field(schema, u"key");`,
    },

    // ------------------------------------------------------------------ boolean
    {
      note: "boolean 分派的关键字拼错（分支落空）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  if (t == u"boolean") {`,
      to: `  if (t == u"bool") {`,
    },
    {
      note: "boolean 判定取反（该报错的不报、不该报的报）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (!std::holds_alternative<bool>(value)) return wrong(value, schema, _errors);
  } else if (t == u"string") {`,
      to: `    if (std::holds_alternative<bool>(value)) return wrong(value, schema, _errors);
  } else if (t == u"string") {`,
    },

    // ------------------------------------------------------------------- string
    {
      note: "string 分派的关键字拼错（分支落空）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  } else if (t == u"string") {`,
      to: `  } else if (t == u"str") {`,
    },
    {
      note: "not_blank 判定不 trim",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (truthy(field(s, u"not_blank")) && js_trim(text).empty()) {`,
      to: `    if (truthy(field(s, u"not_blank")) && text.empty()) {`,
    },
    {
      note: "not_empty 判定去看 not_blank 旗标",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (truthy(field(s, u"not_empty")) && text.empty()) return wrong(value, schema, _errors);`,
      to: `    if (truthy(field(s, u"not_blank")) && text.empty()) return wrong(value, schema, _errors);`,
    },

    // ------------------------------------------------------------------- number
    {
      note: "number/integer 分派只认 'number'（type 'integer' 落空）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  } else if (t == u"number" || t == u"integer") {`,
      to: `  } else if (t == u"number") {`,
    },
    {
      note: "nan 守卫取反（nan 旗标变成「只准 NaN」）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (!truthy(field(n, u"nan")) && std::isnan(*d)) return wrong(value, schema, _errors);`,
      to: `    if (truthy(field(n, u"nan")) && std::isnan(*d)) return wrong(value, schema, _errors);`,
    },
    {
      note: "int 判定取反（整数反而不合法）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (truthy(field(n, u"int")) && !is_js_integer(*d)) return wrong(value, schema, _errors);`,
      to: `    if (truthy(field(n, u"int")) && is_js_integer(*d)) return wrong(value, schema, _errors);`,
    },
    {
      note: "nagetive 的边界 `>= 0` 变 `> 0`（0 放行）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (truthy(field(n, u"nagetive")) && *d >= 0) return wrong(value, schema, _errors);`,
      to: `    if (truthy(field(n, u"nagetive")) && *d > 0) return wrong(value, schema, _errors);`,
    },
    {
      note: "positive 的边界 `<= 0` 变 `< 0`（0 放行）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (truthy(field(n, u"positive")) && *d <= 0) return wrong(value, schema, _errors);`,
      to: `    if (truthy(field(n, u"positive")) && *d < 0) return wrong(value, schema, _errors);`,
    },
    {
      note: "nagetive == false 的守卫方向反了",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (loose_equals_false(field(n, u"nagetive")) && *d < 0) {`,
      to: `    if (loose_equals_false(field(n, u"nagetive")) && *d > 0) {`,
    },
    {
      note: "positive == false 的守卫方向反了",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (loose_equals_false(field(n, u"positive")) && *d > 0) {`,
      to: `    if (loose_equals_false(field(n, u"positive")) && *d < 0) {`,
    },
    {
      note: "is_js_integer 丢掉 isfinite（±Infinity 也算整数）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `bool is_js_integer(double v) { return std::isfinite(v) && std::floor(v) == v; }`,
      to: `bool is_js_integer(double v) { return std::floor(v) == v; }`,
    },
    {
      note: "`== !1` 的布尔真值取反",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  if (const bool* b = std::get_if<bool>(&v)) return !*b;`,
      to: `  if (const bool* b = std::get_if<bool>(&v)) return *b;`,
    },

    // -------------------------------------------------------------------- array
    {
      note: "array 分派的关键字拼错（分支落空）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  } else if (t == u"array") {`,
      to: `  } else if (t == u"arr") {`,
    },
    {
      note: "array 不再要求 items（缺 items 直接放行）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    const Value& items = field(schema, u"items");
    if (!truthy(items)) return wrong(value, schema, _errors);`,
      to: `    const Value& items = field(schema, u"items");
    if (false) return wrong(value, schema, _errors);`,
    },
    {
      note: "数组项的浅拷贝换成空数组（内容都丢了）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `        mut->at(i) = Value(std::make_shared<Array>(inner->items()));`,
      to: `        mut->at(i) = Value(std::make_shared<Array>());`,
    },

    // ------------------------------------------------------------------- object
    {
      note: "object 分派的关键字拼错（分支落空）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  } else if (t == u"object") {`,
      to: `  } else if (t == u"obj") {`,
    },
    {
      note: "object 判定不认数组（typeof [] === 'object' 丢了）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    const bool is_object = as_object(value) != nullptr || as_array(value) != nullptr;`,
      to: `    const bool is_object = as_object(value) != nullptr;`,
    },
    {
      note: "object 判定写成 `&&`（对象反而不合法）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    const bool is_object = as_object(value) != nullptr || as_array(value) != nullptr;`,
      to: `    const bool is_object = as_object(value) != nullptr && as_array(value) != nullptr;`,
    },
    {
      note: "未知键的警告条件反了（已知键才警告）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `      const bool known = properties != nullptr && properties->has(k);
      if (!known) {`,
      to: `      const bool known = properties != nullptr && properties->has(k);
      if (known) {`,
    },
    {
      note: "未知键消息里的 `in` 换成 `at`",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `        _warnings.push_back(u"unexpected key '" + k + u"' in '" +`,
      to: `        _warnings.push_back(u"unexpected key '" + k + u"' at '" +`,
    },
    {
      note: "properties 的逐键校验整段跳过",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (properties != nullptr) {
      for (const std::u16string& k : properties->keys()) {`,
      to: `    if (properties != nullptr && false) {
      for (const std::u16string& k : properties->keys()) {`,
    },
    {
      note: "属性值不再从 value 里读（恒 undefined）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `        Value prop_value = get_prop(value, k);`,
      to: `        Value prop_value;`,
    },
    {
      note: "对象属性的浅拷贝换成空数组（写回的内容丢了）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `          prop_value = Value(std::make_shared<Array>(inner->items()));`,
      to: `          prop_value = Value(std::make_shared<Array>());`,
    },

    // -------------------------------------------------------------------- oneof
    {
      note: "oneof 命中判定取反",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (!found) {`,
      to: `    if (found) {`,
    },
    {
      note: "oneof 消息文字换词",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `      _errors.push_back(u"'" + to_string(field(schema, u"path")) +
                        u"' should be one of the options: " + json_of(oneof) + u", but got " +
                        to_string(value));`,
      to: `      _errors.push_back(u"'" + to_string(field(schema, u"path")) +
                        u"' should be in options: " + json_of(oneof) + u", but got " +
                        to_string(value));`,
    },
    {
      note: "oneof 用 JS `String()` 而不是 `JSON.stringify` 渲染",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `                        u"' should be one of the options: " + json_of(oneof) + u", but got " +`,
      to: `                        u"' should be one of the options: " + to_string(oneof) + u", but got " +`,
    },

    // --------------------------------------------------------------- 返回值语义
    {
      note: "validate 恒返回 true（残留错误被吞）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  return _errors.empty();`,
      to: `  return true;`,
    },
    {
      note: "validate 返回「有没有错误」的反（干净也 false）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  return _errors.empty();`,
      to: `  return !_errors.empty();`,
    },

    // ------------------------------------------------------------------- _wrong
    {
      note: "_wrong 的消息尾巴换词（`, but got ` → `, got `）",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    e.push_back(u"type of '" + to_string(field(s, u"path")) + u"' must be " + type +
                u", but got " + to_string(v));`,
      to: `    e.push_back(u"type of '" + to_string(field(s, u"path")) + u"' must be " + type +
                u", got " + to_string(v));`,
    },
    {
      note: "_wrong 的 `must be` 换成 `should be`",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    e.push_back(u"type of '" + to_string(field(s, u"path")) + u"' must be " + type +`,
      to: `    e.push_back(u"type of '" + to_string(field(s, u"path")) + u"' should be " + type +`,
    },
    {
      note: "_wrong 读 `key` 而不是 `path`",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  const auto abc = [&](const std::u16string& type) {
    e.push_back(u"type of '" + to_string(field(s, u"path")) + u"' must be " + type +`,
      to: `  const auto abc = [&](const std::u16string& type) {
    e.push_back(u"type of '" + to_string(field(s, u"key")) + u"' must be " + type +`,
    },
    {
      note: "_wrong 的 string 两条消息次序对调",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    if (truthy(field(field(s, u"string"), u"not_blank"))) return abc(u"non-blank string");
    if (truthy(field(field(s, u"string"), u"not_empty"))) return abc(u"non-empty string");`,
      to: `    if (truthy(field(field(s, u"string"), u"not_empty"))) return abc(u"non-empty string");
    if (truthy(field(field(s, u"string"), u"not_blank"))) return abc(u"non-blank string");`,
    },
    {
      note: "_wrong 的 number 子类型名 int/number 对调",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `    const std::u16string sub = truthy(field(n, u"int")) ? u"integer" : u"number";`,
      to: `    const std::u16string sub = truthy(field(n, u"int")) ? u"number" : u"integer";`,
    },
    {
      note: "_wrong 的 `items not set!` 标点变体",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `      e.push_back(u"items not set! " + to_string(field(s, u"path")));`,
      to: `      e.push_back(u"items not set: " + to_string(field(s, u"path")));`,
    },
    {
      note: "_wrong 的 boolean 消息换词",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  if (type == u"boolean") return abc(u"boolean");`,
      to: `  if (type == u"boolean") return abc(u"bool");`,
    },
    {
      note: "_wrong 的 object 消息换词",
      file: "native/lfw/utils/schema/validate_schema.cpp",
      from: `  if (type == u"object") return abc(u"object");`,
      to: `  if (type == u"object") return abc(u"obj");`,
    },

    // ------------------------------------------------------------ check_stage_info
    {
      note: "check_stage_info 拿错了 schema（phase 顶 stage）",
      file: "native/lfw/loader/check_stage_info.cpp",
      from: `  const bool result = v.validate(info, schema_i_stage_info());`,
      to: `  const bool result = v.validate(info, schema_i_stage_phase_info());`,
    },
    {
      note: "check_stage_info 不再回填 errors",
      file: "native/lfw/loader/check_stage_info.cpp",
      from: `  if (errors != nullptr) errors->insert(errors->end(), v.errors().begin(), v.errors().end());`,
      to: `  if (errors == nullptr) errors->insert(errors->end(), v.errors().begin(), v.errors().end());`,
    },
    {
      note: "check_phase_info 拿错了 schema（stage 顶 phase）",
      file: "native/lfw/loader/check_stage_info.cpp",
      from: `  const bool result = v.validate(info, schema_i_stage_phase_info());`,
      to: `  const bool result = v.validate(info, schema_i_stage_info());`,
    },
    {
      note: "check_phase_info 的 sink 选反（有 errors 也丢进局部）",
      file: "native/lfw/loader/check_stage_info.cpp",
      from: `  std::vector<std::u16string>& sink = errors != nullptr ? *errors : local;`,
      to: `  std::vector<std::u16string>& sink = errors != nullptr ? local : *errors;`,
    },
  ],
};
