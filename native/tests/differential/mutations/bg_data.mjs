export default {
  subject: "bg_data",
  mutations: [
    {
      note: "bg_color_translate: 4706 的映射值写错",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  if (key == u"4706") return u"rgb(16,79,16)";`,
      to: `  if (key == u"4706") return u"rgb(16,79,17)";`,
    },
    {
      note: "bg_color_translate: 漏掉 40179b 的映射",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  if (key == u"40179b") return u"rgb(159,163,159)";`,
      to: ``,
    },
    {
      note: "bg_color_translate: 非数字直接用字符串时不返回原串",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  if (is_str(rect)) return std::get<std::u16string>(rect);`,
      to: `  if (is_str(rect)) return u"";`,
    },
    {
      note: "bg_color_translate: r 为 0 时不再 +7",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  const int32_t r2 = r + ((r > 64 || r == 0) ? 7 : 0);`,
      to: `  const int32_t r2 = r + ((r > 64) ? 7 : 0);`,
    },
    {
      note: "bg_color_translate: 丢掉 bit5 && g>80 的 +4",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  const int32_t g2 = g + ((g > 64 || g == 0) ? 7 : 0) + ((bit5 != 0 && g > 80) ? 4 : 0);`,
      to: `  const int32_t g2 = g + ((g > 64 || g == 0) ? 7 : 0);`,
    },
    {
      note: "整串的双反斜杠替换改成空操作",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  const std::u16string text = replace_double_back_slash(to_string(full_str));`,
      to: `  const std::u16string text = to_string(full_str);`,
    },
    {
      note: "单个反斜杠也当成一对来替换",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `    if (s[i] == u'\\\\' && i + 1 < s.size() && s[i + 1] == u'\\\\') {`,
      to: `    if (s[i] == u'\\\\') {`,
    },
    {
      note: "bmp 后缀替换不再锚定结尾",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  if (s.size() >= 4 && s.compare(s.size() - 3, 3, u"bmp") == 0) {`,
      to: `  if (s.size() >= 4 && s.find(u"bmp") != std::u16string::npos) {`,
    },
    {
      note: "bmp 后缀替换要求至少 5 个字符（x.bmp 不再命中）",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  if (s.size() >= 4 && s.compare(s.size() - 3, 3, u"bmp") == 0) {`,
      to: `  if (s.size() >= 5 && s.compare(s.size() - 3, 3, u"bmp") == 0) {`,
    },
    {
      note: "make_bg_layer: 不再过滤空行",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `    if (!p.empty()) kept.push_back(trim_str(p));`,
      to: `    kept.push_back(trim_str(p));`,
    },
    {
      note: "layer.z 用 0 而不是 length 差值",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `    if (lo != nullptr) lo->set(u"z", n(static_cast<double>(layers.size()) -
                                     static_cast<double>(blocks.blocks.size())));`,
      to: `    if (lo != nullptr) lo->set(u"z", n(0));`,
    },
    {
      note: "far/near 的 zboundary 下标互换",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `    io->set(u"far", n(2 * (array_num(zb, 0) - screen)));
    io->set(u"near", n(2 * (array_num(zb, 1) - screen)));`,
      to: `    io->set(u"far", n(2 * (array_num(zb, 1) - screen)));
    io->set(u"near", n(2 * (array_num(zb, 0) - screen)));`,
    },
    {
      note: "far/near 不再乘 2",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `    io->set(u"far", n(2 * (array_num(zb, 0) - screen)));`,
      to: `    io->set(u"far", n(array_num(zb, 0) - screen));`,
    },
    {
      note: "layer.width 缺省时不再补 0",
      file: "native/lfw/dat_translator/bg_data.cpp",
      from: `  lo->set(u"width", is_nullish(width) ? n(0) : width);`,
      to: `  lo->set(u"width", width);`,
    },
    {
      note: "layer.x 不再做 typeof number 判定",
      file: `native/lfw/dat_translator/bg_data.cpp`,
      from: `  lo->set(u"x", is_num(x) ? x : n(0));`,
      to: `  lo->set(u"x", x);`,
    },
    {
      note: "layer.absolute 写成 0",
      file: `native/lfw/dat_translator/bg_data.cpp`,
      from: `    lo->set(u"absolute", n(1));`,
      to: `    lo->set(u"absolute", n(0));`,
    },
    {
      note: "layer.cc 不再乘 2",
      file: `native/lfw/dat_translator/bg_data.cpp`,
      from: `  lo->set(u"cc", is_num(cc) ? n(to_number(cc) * 2) : Value());`,
      to: `  lo->set(u"cc", is_num(cc) ? n(to_number(cc)) : Value());`,
    },
    {
      note: "layer.c2 少加 1",
      file: `native/lfw/dat_translator/bg_data.cpp`,
      from: `  lo->set(u"c2", is_num(c2) ? n(to_number(c2) * 2 + 1) : Value());`,
      to: `  lo->set(u"c2", is_num(c2) ? n(to_number(c2) * 2) : Value());`,
    },
    {
      note: "bg/template 判定放宽成 bg/",
      file: `native/lfw/dat_translator/bg_data.cpp`,
      from: `    if (starts_with(file, u"bg/template")) {`,
      to: `    if (starts_with(file, u"bg/")) {`,
    },
    {
      note: "name 里的下划线不再换成空格",
      file: `native/lfw/dat_translator/bg_data.cpp`,
      from: `    if (is_str(nm)) base_obj->set(u"name", Value(replace_all(std::get<std::u16string>(nm), u'_', u' ')));`,
      to: ``,
    },
    {
      note: "index 缺 id 时不再回落到 fields.name",
      file: `native/lfw/dat_translator/bg_data.cpp`,
      from: `  ro->set(u"id", is_nullish(index_id) ? field_value(fields, u"name") : index_id);`,
      to: `  ro->set(u"id", index_id);`,
    },
    {
      note: "CLASSIC_SCREEN_HEIGHT 当成 0",
      file: `native/lfw/dat_translator/bg_data.cpp`,
      from: `  lo->set(u"y", n(defines::num(u"Defines.CLASSIC_SCREEN_HEIGHT") - num_of(y)));`,
      to: `  lo->set(u"y", n(-num_of(y)));`,
    },
  ],
};
