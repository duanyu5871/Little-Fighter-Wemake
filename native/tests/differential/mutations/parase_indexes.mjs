export default {
  subject: "parase_indexes",
  mutations: [
    {
      note: "不再检查 text 是否为空",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `  if (!truthy(text) || !is_str(text)) {`,
      to: `  if (false) {`,
    },
    {
      note: "不再检查 [NOT_READY] 标记",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `  if (s.find(u"[NOT_READY]") != std::u16string::npos) {`,
      to: `  if (false) {`,
    },
    {
      note: "object 块起始标记写错",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `      match_block_once(s, u"<object>", u"<object_end>");`,
      to: `      match_block_once(s, u"<objects>", u"<object_end>");`,
    },
    {
      note: "objects: 不再跳过空行",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `      if (raw_line.empty()) continue;
      const std::u16string line = js_trim(raw_line);
      if (starts_with_hash(line)) continue;
      Object item;
      item.set(u"id", Value(std::u16string()));
      item.set(u"type", Value(std::u16string(dat_type_enum::kInvalid)));`,
      to: `      const std::u16string line = js_trim(raw_line);
      if (starts_with_hash(line)) continue;
      Object item;
      item.set(u"id", Value(std::u16string()));
      item.set(u"type", Value(std::u16string(dat_type_enum::kInvalid)));`,
    },
    {
      note: "objects: 不再跳过 # 注释行",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `      if (starts_with_hash(line)) continue;
      Object item;
      item.set(u"id", Value(std::u16string()));
      item.set(u"type", Value(std::u16string(dat_type_enum::kInvalid)));`,
      to: `      Object item;
      item.set(u"id", Value(std::u16string()));
      item.set(u"type", Value(std::u16string(dat_type_enum::kInvalid)));`,
    },
    {
      note: "objects: 默认 type 写成 Fighter",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `      item.set(u"type", Value(std::u16string(dat_type_enum::kInvalid)));`,
      to: `      item.set(u"type", Value(std::u16string(dat_type_enum::kFighter)));`,
    },
    {
      note: "objects: 不再交换 alias 与 id",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `      if (alias_v != nullptr && id_v != nullptr && is_non_empty_str(*alias_v) &&
          is_non_empty_str(*id_v)) {`,
      to: `      if (false) {`,
    },
    {
      note: "objects: 交换时 id 写回自己（等于没交换）",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `        item.set(u"id", Value(std::get<std::u16string>(*alias_v)));`,
      to: `        item.set(u"id", Value(keep_id));`,
    },
    {
      note: "objects: LouisArmourA 的 id 判定写成 HenryArrow1",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `        if (id == oid::kWeapon_LouisArmourA) item.set(u"hash", Value(u"louis_limbs_armour"));`,
      to: `        if (id == oid::kHenryArrow1) item.set(u"hash", Value(u"louis_limbs_armour"));`,
    },
    {
      note: "objects: Boomerang 的 hash 值写错",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `        else if (id == oid::kWeapon_Boomerang) item.set(u"hash", Value(u"boomerang"));`,
      to: `        else if (id == oid::kWeapon_Boomerang) item.set(u"hash", Value(u"boomerang2"));`,
    },
    {
      note: "objects: groups 不再按逗号切分",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `          item.set(name, make_str_array(split_commas(value)));`,
      to: `          item.set(name, make_str_array({value}));`,
    },
    {
      note: "objects: file 不再把反斜杠换成斜杠",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `          const std::u16string src = replace_all(value, u'\\\\', u'/');
          item.set(u"src", Value(src));
          item.set(name, Value(replace_dat_suffix(src, u".obj." + suffix)));`,
      to: `          const std::u16string src = value;
          item.set(u"src", Value(src));
          item.set(name, Value(replace_dat_suffix(src, u".obj." + suffix)));`,
    },
    {
      note: "replace_dat_suffix: 只切掉三个字符",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `  if (s.compare(s.size() - 3, 3, u"dat") != 0) return s;
  return s.substr(0, s.size() - 4) + ext;`,
      to: `  if (s.compare(s.size() - 3, 3, u"dat") != 0) return s;
  return s.substr(0, s.size() - 3) + ext;`,
    },
    {
      note: "backgrounds: id 不再加 bg_ 前缀",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `          item.set(name, Value(u"bg_" + value));`,
      to: `          item.set(name, Value(value));`,
    },
    {
      note: "backgrounds: 默认 type 写成 Stage",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `      item.set(u"type", Value(std::u16string(dat_type_enum::kBackground)));`,
      to: `      item.set(u"type", Value(std::u16string(dat_type_enum::kStage)));`,
    },
    {
      note: "stages: .txt 判定写成 .TXT",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `          } else if (ends_with(value, u".txt")) {`,
      to: `          } else if (ends_with(value, u".TXT")) {`,
    },
    {
      note: "stages: .txt 分支也用 dat 替换",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `            item.set(name, Value(replace_txt_suffix(src, u".stage." + suffix)));`,
      to: `            item.set(name, Value(replace_dat_suffix(src, u".stage." + suffix)));`,
    },
    {
      note: "stages: 不写 src",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `          item.set(u"src", Value(src));
          if (ends_with(value, u".dat")) {`,
      to: `          if (ends_with(value, u".dat")) {`,
    },
    {
      note: "结果里 bots 键名写成 Bots",
      file: "native/lfw/dat_translator/parase_indexes.cpp",
      from: `  lists.set(u"bots", make_str_array({}));`,
      to: `  lists.set(u"Bots", make_str_array({}));`,
    },
  ],
};
