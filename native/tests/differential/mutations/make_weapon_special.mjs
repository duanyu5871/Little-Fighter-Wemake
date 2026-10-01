export default {
  subject: "make_weapon_special",
  mutations: [
    {
      note: "range: 上界少 1",
      file: "native/lfw/dat_translator/broken_piece_frames.cpp",
      from: `  static const Value v = piece_range(0, 3);`,
      to: `  static const Value v = piece_range(0, 2);`,
    },
    {
      note: "piece_range: 元素写成数字而不是字符串",
      file: "native/lfw/dat_translator/broken_piece_frames.cpp",
      from: `    for (double v : *nums) a.push_back(to_string(Value(v)));`,
      to: `    for (double v : *nums) a.push_back(Value(v));`,
    },
    {
      note: "1100~199 的下界写成 101",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `  if (base != nullptr && num_data_id >= 100 && num_data_id <= 199) {`,
      to: `  if (base != nullptr && num_data_id >= 101 && num_data_id <= 199) {`,
    },
    {
      note: "1100~199 的上界写成 198",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `  if (base != nullptr && num_data_id >= 100 && num_data_id <= 199) {`,
      to: `  if (base != nullptr && num_data_id >= 100 && num_data_id <= 198) {`,
    },
    {
      note: "group 的 ensure 少写一个元素",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    base->set(u"group", ensure(cur, std::vector<Value>{s(entity_group::kVsWeapon),
                                                       s(entity_group::kStageWeapon)}));`,
      to: `    base->set(u"group", ensure(cur, std::vector<Value>{s(entity_group::kVsWeapon)}));`,
    },
    {
      note: "Heavy 分支的判定写成 Stick",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    if (strict_equals(type_v, en(WeaponEnum::Heavy))) {
      if (is_nullish(field_or_any(*base, u"w_atk_m_x"))) base->set(u"w_atk_m_x", n(-1));
      if (is_nullish(field_or_any(*base, u"w_atk_r_x"))) base->set(u"w_atk_r_x", n(200));`,
      to: `    if (strict_equals(type_v, en(WeaponEnum::Stick))) {
      if (is_nullish(field_or_any(*base, u"w_atk_m_x"))) base->set(u"w_atk_m_x", n(-1));
      if (is_nullish(field_or_any(*base, u"w_atk_r_x"))) base->set(u"w_atk_r_x", n(200));`,
    },
    {
      note: "Heavy 的 w_atk_r_x 写成 201",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    if (strict_equals(type_v, en(WeaponEnum::Heavy))) {
      if (is_nullish(field_or_any(*base, u"w_atk_m_x"))) base->set(u"w_atk_m_x", n(-1));
      if (is_nullish(field_or_any(*base, u"w_atk_r_x"))) base->set(u"w_atk_r_x", n(200));`,
      to: `    if (strict_equals(type_v, en(WeaponEnum::Heavy))) {
      if (is_nullish(field_or_any(*base, u"w_atk_m_x"))) base->set(u"w_atk_m_x", n(-1));
      if (is_nullish(field_or_any(*base, u"w_atk_r_x"))) base->set(u"w_atk_r_x", n(201));`,
    },
    {
      note: "Baseball/Drink 分支漏掉 Drink",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    } else if (strict_equals(type_v, en(WeaponEnum::Baseball)) ||
               strict_equals(type_v, en(WeaponEnum::Drink))) {`,
      to: `    } else if (strict_equals(type_v, en(WeaponEnum::Baseball))) {`,
    },
    {
      note: "??= 写成真值判定（0 也会被覆盖）",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `void set_or_keep_brokens(Object& base, const std::vector<Value>& ids) {
  if (is_nullish(field_or_any(base, u"brokens"))) {`,
      to: `void set_or_keep_brokens(Object& base, const std::vector<Value>& ids) {
  if (!truthy(field_or_any(base, u"brokens"))) {`,
    },
    {
      note: "HenryArrow1 的判定写成 RudolfWeapon",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `  if (id == oid::kHenryArrow1) {`,
      to: `  if (id == oid::kRudolfWeapon) {`,
    },
    {
      note: "HenryArrow1 的 weight 用 HEAVY",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `  if (id == oid::kHenryArrow1) {
    if (base != nullptr) {
      base->set(u"weight", n(defines::num(u"Defines.WEAPON_WEIGHT_ARROW")));
      base->remove(u"group");
    }`,
      to: `  if (id == oid::kHenryArrow1) {
    if (base != nullptr) {
      base->set(u"weight", n(defines::num(u"Defines.WEAPON_WEIGHT_HEAVY")));
      base->remove(u"group");
    }`,
    },
    {
      note: "RudolfWeapon 不再删除 group",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `  if (id == oid::kRudolfWeapon) {
    if (base != nullptr) {
      base->set(u"weight", n(defines::num(u"Defines.WEAPON_WEIGHT_ARROW")));
      base->remove(u"group");
    }`,
      to: `  if (id == oid::kRudolfWeapon) {
    if (base != nullptr) {
      base->set(u"weight", n(defines::num(u"Defines.WEAPON_WEIGHT_ARROW")));
    }`,
    },
    {
      note: "Weapon_Rebounding 判定写成 Defend",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `        const Value* state_v = frame->get(u"state");
        if (state_v != nullptr && strict_equals(*state_v, en(StateEnum::Weapon_Rebounding))) {
          frame->remove(u"itr");
          return;
        }`,
      to: `        const Value* state_v = frame->get(u"state");
        if (state_v != nullptr && strict_equals(*state_v, en(StateEnum::Defend))) {
          frame->remove(u"itr");
          return;
        }`,
    },
    {
      note: "动作类型写成 V_NEXT_FRAME",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `          const Value item = make_obj({{u"type", s(action_type::kA_NEXT_FRAME)},
                                       {u"data", opoint_frame(u"Defines.NEXT_FRAME_GONE")},
                                       {u"pretest", Value(true)},`,
      to: `          const Value item = make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},
                                       {u"data", opoint_frame(u"Defines.NEXT_FRAME_GONE")},
                                       {u"pretest", Value(true)},`,
    },
    {
      note: "pretest 写成 false",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `                                       {u"pretest", Value(true)},`,
      to: `                                       {u"pretest", Value(false)},`,
    },
    {
      note: "brokens 的动作 id 总是取第一个 frame",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    o.set(u"action", make_obj({{u"id", frame_ids[idx]}}));`,
      to: `    o.set(u"action", make_obj({{u"id", frame_ids[0]}}));`,
    },
    {
      note: "aa 的取模写成 3",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `  switch (idx % 10) {`,
      to: `  switch (idx % 3) {`,
    },
    {
      note: "aa[0] 的 dvx 符号反了",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    case 0:
      return make_obj({{u"dvy", n(5)}, {u"dvx", n(-1)}});`,
      to: `    case 0:
      return make_obj({{u"dvy", n(5)}, {u"dvx", n(1)}});`,
    },
    {
      note: "inherit_speed_z 写成 0.6",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    o.set(u"inherit_speed_z", n(0.5));`,
      to: `    o.set(u"inherit_speed_z", n(0.6));`,
    },
    {
      note: "brokens 的 oid 写成 998",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    o.set(u"oid", s(u"999"));`,
      to: `    o.set(u"oid", s(u"998"));`,
    },
    {
      note: "brokens 的 pos_type 写成 2",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    o.set(u"pos_type", n(1));`,
      to: `    o.set(u"pos_type", n(2));`,
    },
    {
      note: "Stick 的判定写成 Hoe",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `  if (id == oid::kWeapon_Stick) {`,
      to: `  if (id == oid::kWeapon_Hoe) {`,
    },
    {
      note: "Hoe 的片段列表退化",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    set_or_keep_brokens(*base, {k_hoe(), k_hoe(), hoe(), hoe(), s_hoe(), s_hoe()});`,
      to: `    set_or_keep_brokens(*base, {hoe(), hoe(), s_hoe(), s_hoe()});`,
    },
    {
      note: "baseball 的片段只有 4 个",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    set_or_keep_brokens(*base, repeat_of(5, baseball()));`,
      to: `    set_or_keep_brokens(*base, repeat_of(4, baseball()));`,
    },
    {
      note: "milk 的第二段数量写成 4",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    for (const Value& v : repeat_of(5, milk2())) ids.push_back(v);`,
      to: `    for (const Value& v : repeat_of(4, milk2())) ids.push_back(v);`,
    },
    {
      note: "milk 的 drink.hp_h_total 写成 161",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    base->set(u"drink", make_obj({{u"hp_h_total", n(160)},`,
      to: `    base->set(u"drink", make_obj({{u"hp_h_total", n(161)},`,
    },
    {
      note: "Beer 的 group 不再 ensure",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    {
      Value cur = field_or_any(*base, u"group");
      base->set(u"group", ensure(cur, s(entity_group::kVsWeapon)));
    }
    base->set(u"drink", make_obj({{u"mp_h_total", n(750)},`,
      to: `    base->set(u"drink", make_obj({{u"mp_h_total", n(750)},`,
    },
    {
      note: "LouisArmourA 的片段只有 4 个",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `    base->set(u"brokens",
              broken_pieces_opoints({armour(), armour(), armour(), armour(), armour()}));`,
      to: `    base->set(u"brokens", broken_pieces_opoints({armour(), armour(), armour(), armour()}));`,
    },
    {
      note: "IceSword 的 group 用 VsWeapon",
      file: "native/lfw/dat_translator/make_weapon_special.cpp",
      from: `      base->set(u"group", ensure(cur, s(entity_group::kFreezer)));`,
      to: `      base->set(u"group", ensure(cur, s(entity_group::kVsWeapon)));`,
    },
  ],
};
