export default {
  subject: "make_stage_info_list",
  mutations: [
    {
      note: "双反斜杠替换改成单反斜杠",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `    if (s[i] == u'\\\\' && i + 1 < s.size() && s[i + 1] == u'\\\\') {
      out.push_back(u'/');
      i += 2;`,
      to: `    if (s[i] == u'\\\\') {
      out.push_back(u'/');
      i += 1;`,
    },
    {
      note: "phase_end 合体规则失效",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `      if (s.compare(j, 7, u"<stage>") == 0) {`,
      to: `      if (false) {`,
    },
    {
      note: "stage 块起始标记写错",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `  const TakeBlocksResult r0 = take_blocks(text, u"<stage>", u"<stage_end>");`,
      to: `  const TakeBlocksResult r0 = take_blocks(text, u"<stages>", u"<stage_end>");`,
    },
    {
      note: "phase 内容不再按换行拆分",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `      for (const std::u16string& raw_line : split_ln(trimmed)) {`,
      to: `      for (const std::u16string& raw_line : std::vector<std::u16string>{trimmed}) {`,
    },
    {
      note: "bound 的非数字值也被覆盖",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `              if (is_num_value(num)) pi->set(u"bound", Value(to_number(num)));`,
      to: `              pi->set(u"bound", Value(to_number(num)));`,
    },
    {
      note: "desc 不再兜底空串",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `          pi->set(u"desc", hash.has_value() ? Value(js_trim(*hash)) : Value(u""));`,
      to: `          pi->set(u"desc", hash.has_value() ? Value(js_trim(*hash)) : Value());`,
    },
    {
      note: "bound 行的 music 不再加 .mp3",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `              pi->set(u"music", Value(replace_all(kv.second, u'\\\\', u'/') + u".mp3"));
            }
          }
          const std::optional<std::u16string> hash = match_hash_end(line);`,
      to: `              pi->set(u"music", Value(replace_all(kv.second, u'\\\\', u'/')));
            }
          }
          const std::optional<std::u16string> hash = match_hash_end(line);`,
    },
    {
      note: "object.x 不用 bound",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `          object.set(u"x", field_or_any(*pi, u"bound"));`,
      to: `          object.set(u"x", n(0));`,
    },
    {
      note: "soldier 标记判定写成 boss",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `          if (line.find(u"<soldier>") != std::u16string::npos) object.set(u"is_soldier", Value(true));`,
      to: `          if (line.find(u"<boss>") != std::u16string::npos) object.set(u"is_soldier", Value(true));`,
    },
    {
      note: "soldier 的 times 补成 51",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `            object.set(u"times", n(50));`,
      to: `            object.set(u"times", n(51));`,
    },
    {
      note: "facing 的正负反了",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `            object.set(u"facing", n(neg ? 1 : -1));`,
      to: `            object.set(u"facing", n(neg ? -1 : 1));`,
    },
    {
      note: "objects 数组不再兜底创建",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `          if (objs == nullptr || !truthy(*objs)) pi->set(u"objects", Value(std::make_shared<Array>()));`,
      to: `          if (false) pi->set(u"objects", Value(std::make_shared<Array>()));`,
    },
    {
      note: "starting_name 少加 1",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `      si->set(u"starting_name", Value(to_string(n(1 + nid / 10))));`,
      to: `      si->set(u"starting_name", Value(to_string(n(nid / 10))));`,
    },
    {
      note: "起始关卡判定改成 %5",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `    if (std::fmod(nid, 10) == 0) {`,
      to: `    if (std::fmod(nid, 5) == 0) {`,
    },
    {
      note: "enemy_r 的 +300 写成 +301",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `      p->set(u"enemy_r", n((has_bound ? to_number(bound) : 0) + 300));`,
      to: `      p->set(u"enemy_r", n((has_bound ? to_number(bound) : 0) + 301));`,
    },
    {
      note: "enemy_l 写成 -301",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `      p->set(u"enemy_l", n(-300));`,
      to: `      p->set(u"enemy_l", n(-301));`,
    },
    {
      note: "最后一阶段的 on_end 保持不变（不再用 loop）",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `      if (i == phase_count - 1) {`,
      to: `      if (false) {`,
    },
    {
      note: "第一个阶段也给 on_start",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `      } else if (i > 0) {`,
      to: `      } else if (i >= 0) {`,
    },
    {
      note: "health_up 的区间上界改成 50",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `    if (nid < 49 && phase_count > 0) {`,
      to: `    if (nid < 50 && phase_count > 0) {`,
    },
    {
      note: "stage 2 的 title 少减 10",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `      si->set(u"title", Value(u"STAGE 2-" + to_string(n(nid + 1 - 10))));`,
      to: `      si->set(u"title", Value(u"STAGE 2-" + to_string(n(nid + 1))));`,
    },
    {
      note: "chapter 分段边界改成 <= 8",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `    if (nid <= 9) {
      si->set(u"bg", Value(u"bg_2"));`,
      to: `    if (nid <= 8) {
      si->set(u"bg", Value(u"bg_2"));`,
    },
    {
      note: "排序比较器恒返回 false",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `    return av < bv;`,
      to: `    return false;`,
    },
    {
      note: "next 修正的落点写成 11",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `    if (nid <= 9) s->set(u"next", Value(u"10"));`,
      to: `    if (nid <= 9) s->set(u"next", Value(u"11"));`,
    },
    {
      note: "last_phase.on_end 不再清空",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `    last->set(u"on_end", Value());`,
      to: `    (void)last;`,
    },
    {
      note: "player_facing 写成 -1",
      file: "native/lfw/dat_translator/make_stage_info_list.cpp",
      from: `    first->set(u"player_facing", n(1));`,
      to: `    first->set(u"player_facing", n(-1));`,
    },
  ],
};
