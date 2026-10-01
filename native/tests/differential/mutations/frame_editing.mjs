export default {
  subject: "frame_editing",
  mutations: [
    {
      note: "keydown: zero_as 写成 repeat",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `Value cook_one(const Value& v, const Object* costs) {
  if (is_str(v) || is_num(v)) {
    return get_next_frame_by_raw_id(to_string(v), u"frame", u"hit", costs);
  }`,
      to: `Value cook_one(const Value& v, const Object* costs) {
  if (is_str(v) || is_num(v)) {
    return get_next_frame_by_raw_id(to_string(v), u"repeat", u"hit", costs);
  }`,
    },
    {
      note: "keydown: cook 类型写成 next",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `  Object r = *as_object(v);
  Value out = Value(std::make_shared<Object>(r));
  cook_next_frame_cost(out, u"hit", costs);
  return out;`,
      to: `  Object r = *as_object(v);
  Value out = Value(std::make_shared<Object>(r));
  cook_next_frame_cost(out, u"next", costs);
  return out;`,
    },
    {
      note: "keydown: 写到了 hit 上",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `  const Value* kd = fo->get(u"key_down");
  if (kd == nullptr || !truthy(*kd)) fo->set(u"key_down", Value(std::make_shared<Object>()));`,
      to: `  const Value* kd = fo->get(u"hit");
  if (kd == nullptr || !truthy(*kd)) fo->set(u"hit", Value(std::make_shared<Object>()));`,
    },
    {
      note: "hit: 写到了 key_down 上",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `  const Value* hv = fo->get(u"hit");
  if (hv == nullptr || !truthy(*hv)) fo->set(u"hit", Value(std::make_shared<Object>()));`,
      to: `  const Value* hv = fo->get(u"key_down");
  if (hv == nullptr || !truthy(*hv)) fo->set(u"key_down", Value(std::make_shared<Object>()));`,
    },
    {
      note: "seq: 不再跳过 falsy 的 next",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `    if (!truthy(any)) continue;
    if (is_str(any) || is_num(any)) {`,
      to: `    if (is_str(any) || is_num(any)) {`,
    },
    {
      note: "seq: F 判定写成 L",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `  if (key.size() > 0 && key[0] == u'F') {`,
      to: `  if (key.size() > 0 && key[0] == u'L') {`,
    },
    {
      note: "seq: k2 也用 L 前缀",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `    const std::u16string k2 = u"R" + key.substr(1);`,
      to: `    const std::u16string k2 = u"L" + key.substr(1);`,
    },
    {
      note: "seq: B 时的左右朝向互换",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `      l.set(u"facing", en(is_b ? FacingFlag::R : FacingFlag::L));`,
      to: `      l.set(u"facing", en(is_b ? FacingFlag::L : FacingFlag::R));`,
    },
    {
      note: "seq: facing 判定改成严格相等",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `      const bool is_b = o != nullptr && equals(field_or_any(*o, u"facing"), en(FacingFlag::B));`,
      to: `      const bool is_b =
          o != nullptr && strict_equals(field_or_any(*o, u"facing"), en(FacingFlag::B));`,
    },
    {
      note: "seq: 普通分支丢掉已有的 next 列表",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `    const Value* cur = target->get(key);
    target->set(key, add_next_frame(cur != nullptr ? *cur : Value(), cookeds));`,
      to: `    target->set(key, add_next_frame(Value(), cookeds));`,
    },
    {
      note: "seq: seqs 键名写成 seq",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `  const Value* sv = fo->get(u"seqs");
  if (sv == nullptr || !truthy(*sv)) fo->set(u"seqs", Value(std::make_shared<Object>()));`,
      to: `  const Value* sv = fo->get(u"seq");
  if (sv == nullptr || !truthy(*sv)) fo->set(u"seq", Value(std::make_shared<Object>()));`,
    },
    {
      note: "seq: 数字 next 不再 cook（当成对象）",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `    if (is_str(any) || is_num(any)) {
      cookeds.push_back(get_next_frame_by_raw_id(to_string(any), u"frame", u"hit", costs));
      continue;
    }`,
      to: `    if (is_str(any)) {
      cookeds.push_back(get_next_frame_by_raw_id(to_string(any), u"frame", u"hit", costs));
      continue;
    }`,
    },
    {
      note: "对象 next 不再计算消耗",
      file: "native/lfw/dat_translator/frame_editing.cpp",
      from: `      Value t = Value(std::make_shared<Object>(*ao));
      cook_next_frame_cost(t, u"hit", costs);
      cookeds.push_back(t);`,
      to: `      Value t = Value(std::make_shared<Object>(*ao));
      cookeds.push_back(t);`,
    },
  ],
};
