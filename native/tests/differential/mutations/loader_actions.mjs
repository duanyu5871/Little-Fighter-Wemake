export default {
  subject: "loader_actions",
  mutations: [
    {
      note: "bd 去掉 nullish 早退",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `bool preprocess_bot_data(Value& data) {
  if (is_nullish(data)) return false;
  Object* d = as_object(data);`,
      to: `bool preprocess_bot_data(Value& data) {
  Object* d = as_object(data);`,
    },
    {
      note: "bd 的 nullish 只认 undefined",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

std::vector<std::u16string> split_commas`,
      to: `bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v);
}

std::vector<std::u16string> split_commas`,
    },
    {
      note: "假值也展开",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `    if (!ok) return;
    if (!truthy(v)) return;
    const std::vector<std::u16string> ks = split_commas(k);`,
      to: `    if (!ok) return;
    const std::vector<std::u16string> ks = split_commas(k);`,
    },
    {
      note: "无逗号的键也展开",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `    if (ks.size() <= 1) return;
    const Value source = v;`,
      to: `    if (ks.empty()) return;
    const Value source = v;`,
    },
    {
      note: "不删原键",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `    const Value source = v;
    o->remove(k);
    for (const std::u16string& key : ks) {`,
      to: `    const Value source = v;
    for (const std::u16string& key : ks) {`,
    },
    {
      note: "写回用原键",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `      o->set(key, std::move(item));`,
      to: `      o->set(k, std::move(item));`,
    },
    {
      note: "split 丢掉尾段",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `  out.push_back(cur);
  return out;
}

bool is_high_surrogate`,
      to: `  return out;
}

bool is_high_surrogate`,
    },
    {
      note: "split 的分隔符写错",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `    if (s[i] == u',') {`,
      to: `    if (s[i] == u';') {`,
    },
    {
      note: "数组也当不可展开",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `  const Array* a = as_array(v);
  if (a != nullptr) {
    out = Value(std::make_shared<Array>(*a));
    return true;
  }`,
      to: `  const Array* a = as_array(v);
  if (a != nullptr) {
    return false;
  }`,
    },
    {
      note: "非可迭代的值当空数组",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `    out = Value(std::make_shared<Array>(std::move(items)));
    return true;
  }
  return false;
}`,
      to: `    out = Value(std::make_shared<Array>(std::move(items)));
    return true;
  }
  out = Value(std::make_shared<Array>());
  return true;
}`,
    },
    {
      note: "代理对第二个码元不吞",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `        cp.push_back((*s)[i + 1]);
        ++i;
      }`,
      to: `        cp.push_back((*s)[i + 1]);
      }`,
    },
    {
      note: "高位代理判定区间写成低位",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `bool is_high_surrogate(char16_t c) { return c >= 0xD800 && c <= 0xDBFF; }`,
      to: `bool is_high_surrogate(char16_t c) { return c >= 0xDC00 && c <= 0xDFFF; }`,
    },
    {
      note: "字符串值当不可展开",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `    out = Value(std::make_shared<Array>(std::move(items)));
    return true;
  }
  return false;
}`,
      to: `    out = Value(std::make_shared<Array>(std::move(items)));
    return false;
  }
  return false;
}`,
    },
    {
      note: "frames 的抛出不传出去",
      file: "native/lfw/loader/preprocess_bot_data.cpp",
      from: `    Value holder = *frames;
    if (!expand_comma_keys(holder)) return false;`,
      to: `    Value holder = *frames;
    expand_comma_keys(holder);`,
    },
    {
      note: "type 不大写",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  const std::u16string type = to_upper_ascii(std::get<std::u16string>(*tv));
  a->set(u"type", Value(type));`,
      to: `  const std::u16string type = std::get<std::u16string>(*tv);
  a->set(u"type", Value(type));`,
    },
    {
      note: "大写偏移少一",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `      out[i] = static_cast<char16_t>(out[i] - 32);`,
      to: `      out[i] = static_cast<char16_t>(out[i] - 31);`,
    },
    {
      note: "大写的小写区间判定用或",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `    if (out[i] >= u'a' && out[i] <= u'z') {`,
      to: `    if (out[i] >= u'a' || out[i] <= u'z') {`,
    },
    {
      note: "sound 类型丢掉 V_SOUND",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `bool is_sound_type(const std::u16string& t) { return t == u"A_SOUND" || t == u"V_SOUND"; }`,
      to: `bool is_sound_type(const std::u16string& t) { return t == u"A_SOUND"; }`,
    },
    {
      note: "next-frame 类型丢掉 A_DEFEND",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  return t == u"A_NEXT_FRAME" || t == u"V_NEXT_FRAME" || t == u"A_DEFEND" ||
         t == u"V_DEFEND" || t == u"A_BROKEN_DEFEND" || t == u"V_BROKEN_DEFEND";`,
      to: `  return t == u"A_NEXT_FRAME" || t == u"V_NEXT_FRAME" ||
         t == u"V_DEFEND" || t == u"A_BROKEN_DEFEND" || t == u"V_BROKEN_DEFEND";`,
    },
    {
      note: "next-frame 类型丢掉 V_BROKEN_DEFEND",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `         t == u"V_DEFEND" || t == u"A_BROKEN_DEFEND" || t == u"V_BROKEN_DEFEND";`,
      to: `         t == u"V_DEFEND" || t == u"A_BROKEN_DEFEND";`,
    },
    {
      note: "sound 读的键名写成 paths",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  const Value path = field_of(data, u"path");
  return is_str(path) || as_array(path) != nullptr;`,
      to: `  const Value path = field_of(data, u"paths");
  return is_str(path) || as_array(path) != nullptr;`,
    },
    {
      note: "sound 路径不看字符串",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  return is_str(path) || as_array(path) != nullptr;`,
      to: `  return as_array(path) != nullptr;`,
    },
    {
      note: "sound 路径不看数组",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  return is_str(path) || as_array(path) != nullptr;`,
      to: `  return is_str(path);`,
    },
    {
      note: "type 非字符串也放行",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  const Value* tv = a->get(u"type");
  if (tv == nullptr || !is_str(*tv)) return false;
  const std::u16string type = to_upper_ascii(std::get<std::u16string>(*tv));`,
      to: `  const Value* tv = a->get(u"type");
  if (tv == nullptr) return false;
  const std::u16string type = to_upper_ascii(to_string(*tv));`,
    },
    {
      note: "不回写 type",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  const std::u16string type = to_upper_ascii(std::get<std::u16string>(*tv));
  a->set(u"type", Value(type));`,
      to: `  const std::u16string type = to_upper_ascii(std::get<std::u16string>(*tv));`,
    },
    {
      note: "分派条件写成 sound",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  if (!is_next_frame_type(type)) return true;`,
      to: `  if (!is_sound_type(type)) return true;`,
    },
    {
      note: "sound 分支排到 next-frame 之后",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  if (is_sound_type(type)) return sound_path_iterable(data);
  if (!is_next_frame_type(type)) return true;`,
      to: `  if (!is_next_frame_type(type)) return true;
  if (is_sound_type(type)) return sound_path_iterable(data);`,
    },
    {
      note: "next_frame 不调用",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  Value nf = data;
  return preprocess_next_frame(nf);`,
      to: `  (void)data;
  return true;`,
    },
    {
      note: "sound 的 data 读的键名写成 datas",
      file: "native/lfw/loader/preprocess_action.cpp",
      from: `  const Value data = field_of(action, u"data");`,
      to: `  const Value data = field_of(action, u"datas");`,
    },
    {
      note: "pnf 不再判 nullish",
      file: "native/lfw/loader/preprocess_next_frame.cpp",
      from: `  if (is_nullish(nf)) return false;
  return true;
}`,
      to: `  return true;
}`,
    },
    {
      note: "pnf 的 nullish 只认 undefined",
      file: "native/lfw/loader/preprocess_next_frame.cpp",
      from: `  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);`,
      to: `  return std::holds_alternative<std::monostate>(v);`,
    },
    {
      note: "pnf 数组不递归",
      file: "native/lfw/loader/preprocess_next_frame.cpp",
      from: `  const Array* a = as_array(nf);
  if (a != nullptr) {
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = a->at(i);
      if (!preprocess_next_frame(item)) return false;
    }
    return true;
  }`,
      to: `  const Array* a = as_array(nf);
  if (a != nullptr) {
    return true;
  }`,
    },
    {
      note: "pnf 数组只回传最后一个结果",
      file: "native/lfw/loader/preprocess_next_frame.cpp",
      from: `    for (size_t i = 0; i < n; ++i) {
      Value item = a->at(i);
      if (!preprocess_next_frame(item)) return false;
    }
    return true;`,
      to: `    bool last = true;
    for (size_t i = 0; i < n; ++i) {
      Value item = a->at(i);
      last = preprocess_next_frame(item);
    }
    return last;`,
    },
  ],
};
