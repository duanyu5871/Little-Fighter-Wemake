export default {
  subject: "cook_frames",
  mutations: [
    {
      note: "frame 起始标记写错",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    const size_t p0 = text.find(u"<frame>", search);`,
      to: `    const size_t p0 = text.find(u"<frames>", search);`,
    },
    {
      note: "frame 标记后不要求空白",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    if (j == i) {
      search = p0 + 1;
      continue;
    }`,
      to: `    if (false) {
      search = p0 + 1;
      continue;
    }`,
    },
    {
      note: "content 至少两个字符",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    if (e == std::u16string::npos || e < m + 1) {`,
      to: `    if (e == std::u16string::npos || e < m + 2) {`,
    },
    {
      note: "frame_name 允许跨行",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    const size_t name_len = line_len < limit ? line_len : limit;`,
      to: `    const size_t name_len = limit;`,
    },
    {
      note: "wait 的 +2 写成 +1",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    const double wait = to_number(fields != nullptr ? take(*fields, u"wait") : Value()) * 2 + 2;`,
      to: `    const double wait = to_number(fields != nullptr ? take(*fields, u"wait") : Value()) * 2 + 1;`,
    },
    {
      note: "next 的 zero_as 写成 frame",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    const Value next = get_next_frame_by_raw_id(raw_next, u"repeat", u"", nullptr);`,
      to: `    const Value next = get_next_frame_by_raw_id(raw_next, u"frame", u"", nullptr);`,
    },
    {
      note: "pic 递减判定漏掉 row",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `        if (to_number(pic) < row * col) break;`,
      to: `        if (to_number(pic) < col) break;`,
    },
    {
      note: "pic 的 x 用 cell_w + 2",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `                                 {u"x", n((cell_w + 1) * std::fmod(to_number(pic), row))},`,
      to: `                                 {u"x", n((cell_w + 2) * std::fmod(to_number(pic), row))},`,
    },
    {
      note: "pic 的 y 不再除以 row",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `                                 {u"y", n((cell_h + 1) * js_floor(to_number(pic) / row))},`,
      to: `                                 {u"y", n((cell_h + 1) * js_floor(to_number(pic)))},`,
    },
    {
      note: "width 取 h",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    frame_o.set(u"width", pic_obj != nullptr ? field_of(*pic_obj, u"w") : n(0));`,
      to: `    frame_o.set(u"width", pic_obj != nullptr ? field_of(*pic_obj, u"h") : n(0));`,
    },
    {
      note: "wait/next 的键插入序交换",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    frame_o.set(u"wait", n(wait));
    frame_o.set(u"next", next);`,
      to: `    frame_o.set(u"next", next);
    frame_o.set(u"wait", n(wait));`,
    },
    {
      note: "state_name 前缀写错",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `        fo->set(u"state_name", Value(u"StateEnum." + to_string(state_name)));`,
      to: `        fo->set(u"state_name", Value(u"States." + to_string(state_name)));`,
    },
    {
      note: "__ERROR__ 键名写错",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    if (truthy(frame_error)) fo->set(u"__ERROR__", frame_error);`,
      to: `    if (truthy(frame_error)) fo->set(u"__ERR__", frame_error);`,
    },
    {
      note: "隐身基数用 1000",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `      const double vv = 2 * (js_abs(raw_next_num) - 1100);`,
      to: `      const double vv = 2 * (js_abs(raw_next_num) - 1000);`,
    },
    {
      note: "blinking 的 +120 写成 +100",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `      fo->set(u"blinking", n(vv + 120));`,
      to: `      fo->set(u"blinking", n(vv + 100));`,
    },
    {
      note: "空 itr 不再删除",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    if (itr_field == nullptr || !array_has_len(*itr_field)) fo->remove(u"itr");`,
      to: `    if (itr_field != nullptr && array_has_len(*itr_field)) fo->remove(u"itr");`,
    },
    {
      note: "wpoint 误用 itr_list",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    if (!wpoint_list.empty()) fo->set(u"wpoint", wpoint_list[0]);`,
      to: `    if (!wpoint_list.empty()) fo->set(u"wpoint", itr_list[0]);`,
    },
    {
      note: "sound 不再加 .mp3",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `              Value(replace_all(std::get<std::u16string>(sound), u'\\\\', u'/') + u".mp3"));`,
      to: `              Value(replace_all(std::get<std::u16string>(sound), u'\\\\', u'/')));`,
    },
    {
      note: "dircontrol 的 1 判定写成 2",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `      if (hit_o != nullptr && strict_equals(dircontrol, n(1))) {`,
      to: `      if (hit_o != nullptr && strict_equals(dircontrol, n(2))) {`,
    },
    {
      note: "dircontrol 的动作键序交换",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `      const Value item = make_obj({{u"wait", s(u"i")}, {u"facing", en(FacingFlag::Backward)}});`,
      to: `      const Value item = make_obj({{u"facing", en(FacingFlag::Backward)}, {u"wait", s(u"i")}});`,
    },
    {
      note: "vrest 的 itr 类型判定写成 Normal",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `          if (kind == nullptr || !strict_equals(*kind, en(ItrKind::SuperPunchMe))) continue;`,
      to: `          if (kind == nullptr || !strict_equals(*kind, en(ItrKind::Normal))) continue;`,
    },
    {
      note: "vrest 的比较方向反向",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `          if (vrest == nullptr || !truthy(*vrest) || to_number(*vrest) < frame_wait) {`,
      to: `          if (vrest == nullptr || !truthy(*vrest) || to_number(*vrest) > frame_wait) {`,
    },
    {
      note: "cook_dvxyz 的 550 判定写成 551",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `  if (strict_equals(v, n(550))) {`,
      to: `  if (strict_equals(v, n(551))) {`,
    },
    {
      note: "cook_dvxyz 的 Fixed 写成 Acc",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `    return Dvxyz{en(SpeedMode::Fixed), n(0), en(SpeedCtrl::None)};`,
      to: `    return Dvxyz{en(SpeedMode::Acc), n(0), en(SpeedCtrl::None)};`,
    },
    {
      note: "cook_dvxyz 只要真值就处理（零值不再跳过）",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `  if (!not_zero_num(v)) return Dvxyz{Value(), Value(), Value()};`,
      to: `  if (!truthy(v)) return Dvxyz{Value(), Value(), Value()};`,
    },
    {
      note: "cook_dvxyz 不做 round_float",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `  return Dvxyz{Value(), n(round_float(to_number(v))), Value()};`,
      to: `  return Dvxyz{Value(), n(to_number(v)), Value()};`,
    },
    {
      note: "缺 pic 的描述文案写错",
      file: "native/lfw/dat_translator/cook_frames.cpp",
      from: `      err.set(u"msg", s(u"entity_pic_info not found!"));`,
      to: `      err.set(u"msg", s(u"entity_pic_info not found"));`,
    },
  ],
};
