export default {
  subject: "itr_prefabs",
  mutations: [
    {
      note: "entry: 数字与名字的分割改成从短到长（id 只取一位）",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `    for (size_t m = dz; m > d0; --m) {`,
      to: `    for (size_t m = d0 + 1; m <= dz; ++m) {`,
    },
    {
      note: "entry: 的数字段改成非空白段（非数字也当 id）",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `    while (dz < t.size() && t[dz] >= u'0' && t[dz] <= u'9') ++dz;`,
      to: `    while (dz < t.size() && !is_ws(t[dz])) ++dz;
    dz = dz > d0 ? dz : d0;`,
    },
    {
      note: "entry: 的 name 一直取到行尾（含 remain）",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `      out.name = t.substr(j, jn - j);`,
      to: `      out.name = t.substr(j, re - j);`,
    },
    {
      note: "块标记写反（<weapon_strength_list> 与 _end 互换）",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `      match_block_once(to_string(full_str), u"<weapon_strength_list>",
                       u"<weapon_strength_list_end>");`,
      to: `      match_block_once(to_string(full_str), u"<weapon_strength_list_end>",
                       u"<weapon_strength_list>");`,
    },
    {
      note: "冒号键值改成从整块解析（每个 entry 都拿到全部键值）",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `    const std::vector<std::pair<std::u16string, std::u16string>> kvs = match_colon_value(m.remain);`,
      to: `    const std::vector<std::pair<std::u16string, std::u16string>> kvs = match_colon_value(text);`,
    },
    {
      note: "冒号值不再尝试转数字",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `      const std::optional<double> n = to_num(v);
      entry.set(kv.first, n.has_value() ? Value(*n) : v);`,
      to: `      entry.set(kv.first, v);`,
    },
    {
      note: "entry 的 kind 初值写成 1",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `    entry.set(u"kind", Value(0.0));`,
      to: `    entry.set(u"kind", Value(1.0));`,
    },
    {
      note: "空白集合只认空格与制表符",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `bool is_ws(char16_t c) { return is_str_white_space(c); }`,
      to: `bool is_ws(char16_t c) { return c == u' ' || c == u'\\t'; }`,
    },
    {
      note: "最终映射的键写成固定串",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `    ret.set(id != nullptr ? to_string(*id) : std::u16string(), item);`,
      to: `    ret.set(u"itr", item);`,
    },
    {
      note: "空列表时不再返回 undefined（改成空对象）",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `  if (list.empty()) return Value();`,
      to: ``,
    },
    {
      note: "entry 不再交给 cook_itr",
      file: "native/lfw/dat_translator/itr_prefabs.cpp",
      from: `    cook_itr(item, Value());`,
      to: ``,
    },
  ],
};
