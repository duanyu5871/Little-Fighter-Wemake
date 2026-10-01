export default {
  subject: "obj_dat_to_json",
  mutations: [
    {
      note: "is_ns 逻辑取反",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `bool is_ns(char16_t c) { return !is_str_white_space(c); }`,
      to: `bool is_ns(char16_t c) { return is_str_white_space(c); }`,
    },
    {
      note: "按 \\r 切行",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `    if (c == u'\\n') {
      out.push_back(cur);
      cur.clear();`,
      to: `    if (c == u'\\r') {
      out.push_back(cur);
      cur.clear();`,
    },
    {
      note: "name/head/small 不跳过冒号后的空白",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  size_t i = p + k.size();
  while (i < s.size() && is_str_white_space(s[i])) ++i;
  size_t j = i;
  while (j < s.size() && is_ns(s[j])) ++j;
  out = s.substr(i, j - i);`,
      to: `  size_t i = p + k.size();
  size_t j = i;
  while (j < s.size() && is_ns(s[j])) ++j;
  out = s.substr(i, j - i);`,
    },
    {
      note: "name/head/small 取到行尾",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  size_t j = i;
  while (j < s.size() && is_ns(s[j])) ++j;
  out = s.substr(i, j - i);
  return true;`,
      to: `  size_t j = i;
  while (j < s.size() && is_ns(s[j])) ++j;
  out = s.substr(i);
  return true;`,
    },
    {
      note: "bmp 后缀长度门槛放宽到 3",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  return s.size() >= 4 && s.compare(s.size() - 3, 3, u"bmp") == 0;`,
      to: `  return s.size() >= 3 && s.compare(s.size() - 3, 3, u"bmp") == 0;`,
    },
    {
      note: "bmp 替换时少删一个字符",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  return s.substr(0, s.size() - 4) + std::u16string(u".png");`,
      to: `  return s.substr(0, s.size() - 3) + std::u16string(u".png");`,
    },
    {
      note: "反斜杠不再转正斜杠",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  std::u16string t = s;
  for (char16_t& c : t) {
    if (c == u'\\\\') c = u'/';
  }
  return t;`,
      to: `  return s;`,
    },
    {
      note: "两个反斜杠只吃掉一个",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `    if (i + 1 < s.size() && s[i] == u'\\\\' && s[i + 1] == u'\\\\') {
      t.push_back(u'/');
      i += 2;`,
      to: `    if (i + 1 < s.size() && s[i] == u'\\\\' && s[i + 1] == u'\\\\') {
      t.push_back(u'/');
      i += 1;`,
    },
    {
      note: "数字不接受正负号",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  size_t i = pos;
  if (i < s.size() && (s[i] == u'+' || s[i] == u'-')) ++i;
  const size_t sign_end = i;`,
      to: `  size_t i = pos;
  const size_t sign_end = i;`,
    },
    {
      note: "数字不再尝试带小数点的形式",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  if (k < s.size() && s[k] == u'.') {
    size_t d = k + 1;
    size_t d_end = d;
    while (d_end < s.size() && is_digit(s[d_end])) ++d_end;
    if (d_end > d) {
      end = d_end;
      return true;
    }
  }`,
      to: ``,
    },
    {
      note: "key:number 不跳过冒号后的空白",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `      if (i < s.size() && s[i] == u':') {
        ++i;
        while (i < s.size() && is_str_white_space(s[i])) ++i;
        size_t e = 0;
        if (match_number(s, i, e)) {`,
      to: `      if (i < s.size() && s[i] == u':') {
        ++i;
        size_t e = 0;
        if (match_number(s, i, e)) {`,
    },
    {
      note: "key:value 的 value 取到行尾",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `        ++i;
        while (i < s.size() && is_str_white_space(s[i])) ++i;
        size_t j = i;
        while (j < s.size() && is_ns(s[j])) ++j;
        key = s.substr(p, g);
        out = s.substr(i, j - i);`,
      to: `        ++i;
        while (i < s.size() && is_str_white_space(s[i])) ++i;
        key = s.substr(p, g);
        out = s.substr(i);`,
    },
    {
      note: "key:value 与 key number 的顺序互换",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `    std::u16string value;
    if (match_key_colon_value(line, key, value)) {
      base_o->set(key, Value(value));
      continue;
    }
    if (match_key_number(line, key, ns, ne)) {
      base_o->set(key, to_number(Value(line.substr(ns, ne - ns))));
      continue;
    }`,
      to: `    std::u16string value;
    if (match_key_number(line, key, ns, ne)) {
      base_o->set(key, to_number(Value(line.substr(ns, ne - ns))));
      continue;
    }
    if (match_key_colon_value(line, key, value)) {
      base_o->set(key, Value(value));
      continue;
    }`,
    },
    {
      note: "file 行的前缀判定放宽",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `    if (starts_with(line, u"file(")) {`,
      to: `    if (starts_with(line, u"file")) {`,
    },
    {
      note: "file 内部键的前缀判定改严格相等",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `        if (starts_with(key, u"file")) {`,
      to: `        if (key == u"file") {`,
    },
    {
      note: "file 的 id 从 1 开始",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `      const size_t file_id = files_o != nullptr ? files_o->keys().size() : 0;`,
      to: `      const size_t file_id = files_o != nullptr ? files_o->keys().size() + 1 : 0;`,
    },
    {
      note: "file 的 path 不做 bmp 转换",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `        if (starts_with(key, u"file")) {
          file.set(u"path", Value(slash_to_forward(bmp_to_png(value))));
        } else if (key == u"w") {`,
      to: `        if (starts_with(key, u"file")) {
          file.set(u"path", Value(slash_to_forward(value)));
        } else if (key == u"w") {`,
    },
    {
      note: "file 的宽高键名写错",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `        } else if (key == u"w") {
          file.set(u"cell_w", to_number(Value(value)));`,
      to: `        } else if (key == u"W") {
          file.set(u"cell_w", to_number(Value(value)));`,
    },
    {
      note: "file 的初始键序交换",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `      file.set(u"row", Value(0.0));
      file.set(u"col", Value(0.0));`,
      to: `      file.set(u"col", Value(0.0));
      file.set(u"row", Value(0.0));`,
    },
    {
      note: "base 的初始键序交换",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  base.set(u"name", Value(std::u16string()));
  base.set(u"files", Value(std::make_shared<Object>()));`,
      to: `  base.set(u"files", Value(std::make_shared<Object>()));
  base.set(u"name", Value(std::u16string()));`,
    },
    {
      note: "块标记本末倒置",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `      match_block_once(text, std::u16string(u"<bmp_begin>"), std::u16string(u"<bmp_end>"));`,
      to: `      match_block_once(text, std::u16string(u"<bmp_end>"), std::u16string(u"<bmp_begin>"));`,
    },
    {
      note: "找不到块时不再报错",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  if (!block.has_value()) {
    res.error = u"[dat_to_json] failed, 3";
    return res;
  }`,
      to: `  if (!block.has_value()) {
    res.error = u"[dat_to_json] failed, 3";
  }`,
    },
    {
      note: "不再 trim 整块",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  const std::u16string body = js_trim(*block);`,
      to: `  const std::u16string body = *block;`,
    },
    {
      note: "type 0 走武器分支",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  if (type == u"0") {
    data = make_fighter_data(ctx);
  } else if (type == u"1" || type == u"2" || type == u"4" || type == u"6") {`,
      to: `  if (type == u"1") {
    data = make_fighter_data(ctx);
  } else if (type == u"0" || type == u"2" || type == u"4" || type == u"6") {`,
    },
    {
      note: "type 3 走实体分支",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  } else if (type == u"3") {
    data = make_ball_data(ctx);
  } else {`,
      to: `  } else if (type == u"9") {
    data = make_ball_data(ctx);
  } else {`,
    },
    {
      note: "key:number 写成字符串",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `    if (match_key_colon_number(line, key, ns, ne)) {
      base_o->set(key, to_number(Value(line.substr(ns, ne - ns))));
      continue;
    }
    std::u16string value;`,
      to: `    if (match_key_colon_number(line, key, ns, ne)) {
      base_o->set(key, Value(line.substr(ns, ne - ns)));
      continue;
    }
    std::u16string value;`,
    },
    {
      note: "返回值改成整个 ctx",
      file: "native/lfw/dat_translator/obj_dat_to_json.cpp",
      from: `  res.ok = true;
  res.data = data;
  return res;`,
      to: `  res.ok = true;
  res.data = ctx;
  return res;`,
    },
  ],
};
