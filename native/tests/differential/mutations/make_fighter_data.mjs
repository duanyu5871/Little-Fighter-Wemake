export default {
  subject: "make_fighter_data",
  mutations: [
    {
      note: "walking_frame_rate 默认值 3 -> 4",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `  const double walking_frame_rate = to_number(take_number(*base, u"walking_frame_rate", n(3)));`,
      to: `  const double walking_frame_rate = to_number(take_number(*base, u"walking_frame_rate", n(4)));`,
    },
    {
      note: "walking_speedz 默认值 0 -> 1",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `  const double walking_speedz = to_number(take_number(*base, u"walking_speedz", n(0)));`,
      to: `  const double walking_speedz = to_number(take_number(*base, u"walking_speedz", n(1)));`,
    },
    {
      note: "heavy_running_speedz 读错键",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `  const double heavy_running_speedz = to_number(take_number(*base, u"heavy_running_speedz", n(0)));`,
      to: `  const double heavy_running_speedz = to_number(take_number(*base, u"heavy_running_speed", n(0)));`,
    },
    {
      note: "帧消耗表 mp/hp 互换",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `    frame_mp_hp_map.set(frame_id, make_obj({{u"mp", n(v.first)}, {u"hp", n(v.second)}}));`,
      to: `    frame_mp_hp_map.set(frame_id, make_obj({{u"mp", n(v.second)}, {u"hp", n(v.first)}}));`,
    },
    {
      note: "next 的 cook 类型写成 hit",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      for (size_t i = 0; i < na->size(); ++i) {
        Value item = na->at(i);
        cook_next_frame_cost(item, u"next", &frame_mp_hp_map);
      }`,
      to: `      for (size_t i = 0; i < na->size(); ++i) {
        Value item = na->at(i);
        cook_next_frame_cost(item, u"hit", &frame_mp_hp_map);
      }`,
    },
    {
      note: "单值 next 的 cook 类型写成 hit",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `    } else {
      Value item = next_v;
      cook_next_frame_cost(item, u"next", &frame_mp_hp_map);
    }`,
      to: `    } else {
      Value item = next_v;
      cook_next_frame_cost(item, u"hit", &frame_mp_hp_map);
    }`,
    },
    {
      note: "k9 少写一个键",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `    const char16_t* k9[7] = {u"Fa", u"Fj", u"Da", u"Dj", u"Ua", u"Uj", u"ja"};`,
      to: `    const char16_t* k9[7] = {u"Fa", u"Fj", u"Da", u"Dj", u"Ua", u"Uj", u"jz"};`,
    },
    {
      note: "hit_* 的 0 例外不再跳过",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      if (strict_equals(nxt, s(u"0")) || strict_equals(nxt, n(0))) continue;
      editing.seq(k, {nxt});`,
      to: `      editing.seq(k, {nxt});`,
    },
    {
      note: "walking 分支少认一个 id",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `    } else if (fid == 5 || fid == 6 || fid == 7 || fid == 8) {`,
      to: `    } else if (fid == 5 || fid == 6 || fid == 7 || fid == 18) {`,
    },
    {
      note: "walking 分支 ctrl 写入顺序互换",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      frame->set(u"ctrl_z", n(1));
      frame->set(u"ctrl_x", n(1));
    } else if (fid == 9 || fid == 10 || fid == 11) {`,
      to: `      frame->set(u"ctrl_x", n(1));
      frame->set(u"ctrl_z", n(1));
    } else if (fid == 9 || fid == 10 || fid == 11) {`,
    },
    {
      note: "running 分支抄成 ctrl_x",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      frame->set(u"ctrl_z", n(1));
    } else if (fid == 12 || fid == 13 || fid == 14 || fid == 15) {`,
      to: `      frame->set(u"ctrl_x", n(1));
    } else if (fid == 12 || fid == 13 || fid == 14 || fid == 15) {`,
    },
    {
      note: "heavy_walking_speedz 用成 speed",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      frame->set(u"dvz", n(heavy_walking_speedz));`,
      to: `      frame->set(u"dvz", n(heavy_walking_speed));`,
    },
    {
      note: "defend ensure 两项顺序互换",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `          const std::vector<Value> items = {
              make_obj({{u"type", s(u"V_BROKEN_DEFEND")},
                        {u"data", make_obj({{u"id", s(u"112")}})}}),
              make_obj({{u"type", s(u"V_DEFEND")}, {u"data", make_obj({{u"id", s(u"111")}})}})};`,
      to: `          const std::vector<Value> items = {
              make_obj({{u"type", s(u"V_DEFEND")}, {u"data", make_obj({{u"id", s(u"111")}})}}),
              make_obj({{u"type", s(u"V_BROKEN_DEFEND")},
                        {u"data", make_obj({{u"id", s(u"112")}})}})};`,
    },
    {
      note: "jump_flag 写到了 210",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      if (frame_id == u"211") frame->set(u"jump_flag", n(1));`,
      to: `      if (frame_id == u"210") frame->set(u"jump_flag", n(1));`,
    },
    {
      note: "dash 回头的存在性检查看错帧",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      if (frame_id == u"213" && frames->get(u"214") != nullptr) {`,
      to: `      if (frame_id == u"213" && frames->get(u"215") != nullptr) {`,
    },
    {
      note: "dash 长按 a 改成 hit",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `        nexts.push_back(make_obj({{u"id", s(u"90")}}));
        editing.keydown(s(u"a"), nexts);`,
      to: `        nexts.push_back(make_obj({{u"id", s(u"90")}}));
        editing.hit(s(u"a"), nexts);`,
    },
    {
      note: "crouch 的 trend 比较取 +1",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `                                   cadd(cm, ev(entity_val::kPressLR), u"==", n(0));
                                   cand(cm, ev(entity_val::kTrendX), u"==", n(-1));`,
      to: `                                   cadd(cm, ev(entity_val::kPressLR), u"==", n(0));
                                   cand(cm, ev(entity_val::kTrendX), u"==", n(1));`,
    },
    {
      note: "200 帧的状态写错",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      frame->set(u"state", se(StateEnum::Frozen));`,
      to: `      frame->set(u"state", se(StateEnum::Injured));`,
    },
    {
      note: "220 段状态写成 Tired",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      frame->set(u"state", se(StateEnum::Injured));
      const char16_t* name = state_enum_name_of(static_cast<int>(StateEnum::Injured));`,
      to: `      frame->set(u"state", se(StateEnum::Tired));
      const char16_t* name = state_enum_name_of(static_cast<int>(StateEnum::Injured));`,
    },
    {
      note: "220 段的 state_name 用了 Tired",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      frame->set(u"state", se(StateEnum::Injured));
      const char16_t* name = state_enum_name_of(static_cast<int>(StateEnum::Injured));`,
      to: `      frame->set(u"state", se(StateEnum::Injured));
      const char16_t* name = state_enum_name_of(static_cast<int>(StateEnum::Tired));`,
    },
    {
      note: "回头分支漏掉 Standing",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `    if (state_after == static_cast<double>(StateEnum::Standing) ||
        state_after == static_cast<double>(StateEnum::Jump) ||`,
      to: `    if (state_after == static_cast<double>(StateEnum::Tired) ||
        state_after == static_cast<double>(StateEnum::Jump) ||`,
    },
    {
      note: "defend 回头判定用 != 排除",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `    } else if (state_after == static_cast<double>(StateEnum::Defend)) {
      const Array* defenders = as_array(bots_frames_defends());
      bool found = false;`,
      to: `    } else if (state_after == static_cast<double>(StateEnum::Dash)) {
      const Array* defenders = as_array(bots_frames_defends());
      bool found = false;`,
    },
    {
      note: "standing 的 j/d 互换",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      editing.hit(s(u"j"), spread_of(hit_next_frame_jump()));
      editing.hit(s(u"d"), spread_of(hit_next_frame_defend()));
      editing.hit(s(u"FF"), {s(u"running_0")});
      editing.keydown(make_arr({s(u"U"), s(u"D"), s(u"L"), s(u"R")}),`,
      to: `      editing.hit(s(u"j"), spread_of(hit_next_frame_defend()));
      editing.hit(s(u"d"), spread_of(hit_next_frame_jump()));
      editing.hit(s(u"FF"), {s(u"running_0")});
      editing.keydown(make_arr({s(u"U"), s(u"D"), s(u"L"), s(u"R")}),`,
    },
    {
      note: "standing 的方向键少一个",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      editing.keydown(make_arr({s(u"U"), s(u"D"), s(u"L"), s(u"R")}),`,
      to: `      editing.keydown(make_arr({s(u"U"), s(u"D"), s(u"Q"), s(u"R")}),`,
    },
    {
      note: "BurnRun 用成 running_speed",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      frame->set(u"dvz", n(running_speedz));
      frame->set(u"ctrl_z", n(1));
    } else if (state3 == static_cast<double>(StateEnum::Defend)) {`,
      to: `      frame->set(u"dvz", n(running_speed));
      frame->set(u"ctrl_z", n(1));
    } else if (state3 == static_cast<double>(StateEnum::Defend)) {`,
    },
    {
      note: "walking 的 wait 倍数写错",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `        frame->set(u"wait", n(walking_frame_rate * 2));`,
      to: `        frame->set(u"wait", n(walking_frame_rate * 3));`,
    },
    {
      note: "walking 排除的 heavy 段编号写错",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `          frame_id != u"15") {`,
      to: `          frame_id != u"16") {`,
    },
    {
      note: "running 的 wait 用 walking_frame_rate",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `        frame->set(u"wait", n(running_frame_rate * 2));`,
      to: `        frame->set(u"wait", n(walking_frame_rate * 2));`,
    },
    {
      note: "round_trip 长度上界少 1",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `    for (size_t i = 0; i < 2 * src_len - 2; ++i) {`,
      to: `    for (size_t i = 0; i < 2 * src_len - 1; ++i) {`,
    },
    {
      note: "round_trip 回环点算错",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      const double next_index = static_cast<double>(i == 2 * src_len - 3 ? 0 : i + 1);`,
      to: `      const double next_index = static_cast<double>(i == 2 * src_len - 2 ? 0 : i + 1);`,
    },
    {
      note: "round_trip 镜像下标偏移",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `        const Object* ro = as_object(src->at(2 * (src_len - 1) - i));`,
      to: `        const Object* ro = as_object(src->at(2 * (src_len - 2) - i));`,
    },
    {
      note: "round_trip 的 facing 只在 Standing",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `      const double fstate = to_number(field_at(frame_v, u"state"));
      if (fstate == static_cast<double>(StateEnum::Standing) ||
          fstate == static_cast<double>(StateEnum::Walking)) {
        as_mut(next_obj)->set(u"facing", ff(FacingFlag::Ctrl));
      }`,
      to: `      const double fstate = to_number(field_at(frame_v, u"state"));
      if (fstate == static_cast<double>(StateEnum::Standing)) {
        as_mut(next_obj)->set(u"facing", ff(FacingFlag::Ctrl));
      }`,
    },
    {
      note: "lying 的左向帧写错",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `  indexes.set(u"lying", make_obj({{u"-1", s(u"230")}, {u"1", s(u"231")}}));`,
      to: `  indexes.set(u"lying", make_obj({{u"-1", s(u"231")}, {u"1", s(u"231")}}));`,
    },
    {
      note: "landing_1 指到 219",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `  indexes.set(u"landing_1", s(u"215"));`,
      to: `  indexes.set(u"landing_1", s(u"219"));`,
    },
    {
      note: "marker 不再校验 id 是否在同一帧表",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `    if (tframes.get(to_string(*idv)) == nullptr) continue;`,
      to: `    if (tframes.keys().empty()) continue;`,
    },
    {
      note: "bot_id 写入条件恒真",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `  if (truthy(field_at(dat_index, u"bot"))) {`,
      to: `  if (true) {`,
    },
    {
      note: "processed 写成 true",
      file: "native/lfw/dat_translator/make_fighter_data.cpp",
      from: `                          {u"processed", Value(false)}});`,
      to: `                          {u"processed", Value(true)}});`,
    },
  ],
};
