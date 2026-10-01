export default {
  subject: "entity_data",
  mutations: [
    {
      note: "traversal: 丢掉数组路径（数组不再被遍历）",
      file: "native/lfw/utils/container_help/traversal.h",
      from: `  if (Array* a = as_array(obj)) {
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = i < a->size() ? a->at(i) : Value();
      fn(number_to_string(static_cast<double>(i)), item);
    }
    return;
  }
`,
      to: ``,
    },
    {
      note: "traversal: 数组下标从 1 开始（键名错位）",
      file: "native/lfw/utils/container_help/traversal.h",
      from: `      fn(number_to_string(static_cast<double>(i)), item);`,
      to: `      fn(number_to_string(static_cast<double>(i + 1)), item);`,
    },
    {
      note: "make_frames_special: 漏掉 hit_ja",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `  take(*o, u"hit_ja");
}`,
      to: `}`,
    },
    {
      note: "make_frames_special: 数组分支只处理第一个元素",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `    for (size_t i = 0; i < n; ++i) take_frame_keys(a->at(i));`,
      to: `    take_frame_keys(a->at(0));`,
    },
    {
      note: "make_entity_data: 返回对象的键序颠倒（type 在前）",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `  out.set(u"id", id != nullptr ? *id : Value());
  out.set(u"type", en(EntityEnum::Entity));`,
      to: `  out.set(u"type", en(EntityEnum::Entity));
  out.set(u"id", id != nullptr ? *id : Value());`,
    },
    {
      note: "make_entity_data: 把空字符串也算 nullish（hash=\"\" 会回落）",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}`,
      to: `bool is_nullish(const Value& v) {
  if (std::holds_alternative<std::u16string>(v)) return std::get<std::u16string>(v).empty();
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}`,
    },
    {
      note: "make_entity_data: 不再优先用 hash",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `    if (hash != nullptr && !is_nullish(*hash)) {`,
      to: `    if (false && hash != nullptr && !is_nullish(*hash)) {`,
    },
    {
      note: "make_entity_data: 文件名字符过滤不再保留 '|'",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `                      (c >= u'0' && c <= u'9') || c == u'_' || c == u'|';`,
      to: `                      (c >= u'0' && c <= u'9') || c == u'_';`,
    },
    {
      note: "make_entity_data: 文件名字符过滤不再保留 '_'",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `                      (c >= u'0' && c <= u'9') || c == u'_' || c == u'|';`,
      to: `                      (c >= u'0' && c <= u'9') || c == u'|';`,
    },
    {
      note: "make_entity_data: 改了 base 的副本而不是共享对象",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `  Object* info_obj = as_object(info);
  if (info_obj != nullptr) {`,
      to: `  Object* info_obj = as_object(info);
  if (info_obj != nullptr) info = Value(std::make_shared<Object>(*info_obj));
  info_obj = as_object(info);
  if (info_obj != nullptr) {`,
    },
    {
      note: "make_entity_data: type 写成 EntityEnum::Ball",
      file: "native/lfw/dat_translator/entity_data.cpp",
      from: `  out.set(u"type", en(EntityEnum::Entity));`,
      to: `  out.set(u"type", en(EntityEnum::Ball));`,
    },
  ],
};
