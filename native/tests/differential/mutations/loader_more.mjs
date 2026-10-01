export default {
  subject: "loader_more",
  mutations: [
    {
      note: "ball_frame: JohnShield 写成 Normal",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `      if (kind_is(itr, ItrKind::JohnShield)) {`,
      to: `      if (kind_is(itr, ItrKind::Normal)) {`,
    },
    {
      note: "ball_frame: 动作类型写成 A_DEFEND",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `          const std::vector<Value> items = {make_obj({{u"type", s(u"A_NEXT_FRAME")},`,
      to: `          const std::vector<Value> items = {make_obj({{u"type", s(u"A_DEFEND")},`,
    },
    {
      note: "ball_frame: 条件字段用 VFALLING",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `          cm.add(Value(std::u16string(collision_val::kVictimType)), u"==",`,
      to: `          cm.add(Value(std::u16string(collision_val::kVFALLING)), u"==",`,
    },
    {
      note: "ball_frame: 目标类型用 Weapon",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `                 Value(static_cast<double>(EntityEnum::Fighter)));`,
      to: `                 Value(static_cast<double>(EntityEnum::Weapon)));`,
    },
    {
      note: "ball_frame: hit_sounds 取第二项",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `      const Value hit_sound = hs != nullptr && hs->size() > 0 ? hs->at(0) : Value();`,
      to: `      const Value hit_sound = hs != nullptr && hs->size() > 1 ? hs->at(1) : Value();`,
    },
    {
      note: "ball_frame: 排除列表漏掉 Whirlwind",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `      if (truthy(hit_sound) && !kind_is(itr, ItrKind::Whirlwind) &&
          !kind_is(itr, ItrKind::Freeze) && !kind_is(itr, ItrKind::Block) &&
          !kind_is(itr, ItrKind::Heal)) {`,
      to: `      if (truthy(hit_sound) && !kind_is(itr, ItrKind::Freeze) &&
          !kind_is(itr, ItrKind::Block) && !kind_is(itr, ItrKind::Heal)) {`,
    },
    {
      note: "ball_frame: 排除 Freeze 写成 Block",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `          !kind_is(itr, ItrKind::Freeze) && !kind_is(itr, ItrKind::Block) &&`,
      to: `          !kind_is(itr, ItrKind::Block) && !kind_is(itr, ItrKind::Block) &&`,
    },
    {
      note: "ball_frame: gravity_enabled 默认 true",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `    frame_o->set(u"gravity_enabled", Value(false));`,
      to: `    frame_o->set(u"gravity_enabled", Value(true));`,
    },
    {
      note: "ball_frame: 3005/3006 分支互换",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `  if (state_is(frame, StateEnum::Ball_Flying)) {
    dat_translator::cook_ball_frame_state_3000(ctx);
  } else if (state_is(frame, StateEnum::Ball_Hitting)) {
    dat_translator::cook_ball_frame_state_3001(ctx);
  } else if (state_is(frame, StateEnum::Ball_3005)) {
    dat_translator::cook_ball_frame_state_3005(ctx);
  } else if (state_is(frame, StateEnum::Ball_3006)) {
    dat_translator::cook_ball_frame_state_3006(ctx);
  } else {
    dat_translator::cook_ball_frame_state_15(ctx);
  }`,
      to: `  if (state_is(frame, StateEnum::Ball_Flying)) {
    dat_translator::cook_ball_frame_state_3001(ctx);
  } else if (state_is(frame, StateEnum::Ball_Hitting)) {
    dat_translator::cook_ball_frame_state_3000(ctx);
  } else if (state_is(frame, StateEnum::Ball_3005)) {
    dat_translator::cook_ball_frame_state_3006(ctx);
  } else if (state_is(frame, StateEnum::Ball_3006)) {
    dat_translator::cook_ball_frame_state_3005(ctx);
  } else {
    dat_translator::cook_ball_frame_state_15(ctx);
  }`,
    },
    {
      note: "ball_frame: 默认分支用 3000",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `  } else {
    dat_translator::cook_ball_frame_state_15(ctx);
  }`,
      to: `  } else {
    dat_translator::cook_ball_frame_state_3000(ctx);
  }`,
    },
    {
      note: "ball_frame: 第二处的 path 用整个列表",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `              make_obj({{u"type", s(u"A_SOUND")},
                        {u"data", make_obj({{u"path", hit_sounds}})}})};`,
      to: `              make_obj({{u"type", s(u"A_SOUND")},
                        {u"data", make_obj({{u"path", make_arr({hit_sounds})}})}})};`,
    },
    {
      note: "ball_frame: 第二处的类型集合变化",
      file: "native/lfw/loader/preprocess_ball_frame.cpp",
      from: `      if (kind_is(itr, ItrKind::Normal) || kind_is(itr, ItrKind::JohnShield) ||
          kind_is(itr, ItrKind::CharacterThrew) || kind_is(itr, ItrKind::WeaponSwing)) {`,
      to: `      if (kind_is(itr, ItrKind::Normal) || kind_is(itr, ItrKind::JohnShield) ||
          kind_is(itr, ItrKind::CharacterThrew) || kind_is(itr, ItrKind::Whirlwind)) {`,
    },
    {
      note: "bg_data: base 用错键表",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `    reorder_fields(base_v, bg_info_fields());`,
      to: `    reorder_fields(base_v, bg_data_fields());`,
    },
    {
      note: "bg_data: 不给 base 兜底 height",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `    if (base != nullptr) or_assign(*base, u"height", default_screen_height());`,
      to: `    (void)base;`,
    },
    {
      note: "bg_data: dataset 用错键表",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `    reorder_fields(ds, world_dataset_fields());`,
      to: `    reorder_fields(ds, bg_layer_info_fields());`,
    },
    {
      note: "bg_data: layer 用错键表",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `      reorder_fields(layer, bg_layer_info_fields());`,
      to: `      reorder_fields(layer, bg_info_fields());`,
    },
    {
      note: "bg_data: terrain 用错键表",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `      reorder_fields(t, terrain_info_fields());`,
      to: `      reorder_fields(t, bg_layer_info_fields());`,
    },
    {
      note: "bg_data: shadow_w/h 互换",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `      or_assign(*base, u"shadow_w", Value(num_or_zero(a)));
      or_assign(*base, u"shadow_h", Value(num_or_zero(b)));`,
      to: `      or_assign(*base, u"shadow_w", Value(num_or_zero(b)));
      or_assign(*base, u"shadow_h", Value(num_or_zero(a)));`,
    },
    {
      note: "bg_data: zoom 取值错位",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `      or_assign(*base, u"zoom_x", Value(num_or_zero(a)));
      or_assign(*base, u"zoom_y", Value(num_or_zero(b)));
      or_assign(*base, u"zoom_z", Value(num_or_zero(c)));`,
      to: `      or_assign(*base, u"zoom_x", Value(num_or_zero(b)));
      or_assign(*base, u"zoom_y", Value(num_or_zero(a)));
      or_assign(*base, u"zoom_z", Value(num_or_zero(c)));`,
    },
    {
      note: "bg_data: 非数字一律当 0",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `double num_or_zero(const Value& v) { return is_number(v) ? std::get<double>(v) : 0.0; }`,
      to: `double num_or_zero(const Value& v) { (void)v; return 0.0; }`,
    },
    {
      note: "bg_data: 末尾不做 delete_undefined",
      file: "native/lfw/loader/preprocess_bg_data.cpp",
      from: `  reorder_fields(data, bg_data_fields());
  dat_translator::delete_undefined(data);
  return data;`,
      to: `  reorder_fields(data, bg_data_fields());
  return data;`,
    },
    {
      note: "resolve_prefab: ref/prefab_id 优先级互换",
      file: "native/lfw/loader/resolve_prefab.cpp",
      from: `  const Value r = field_at(v, u"ref");
  if (!is_undefined(r)) return r;
  return field_at(v, u"prefab_id");`,
      to: `  const Value r = field_at(v, u"prefab_id");
  if (!is_undefined(r)) return r;
  return field_at(v, u"ref");`,
    },
    {
      note: "resolve_prefab: 不做环检测",
      file: "native/lfw/loader/resolve_prefab.cpp",
      from: `    if (chain_has(chain, key)) {
      res.ok = false;
      res.cycle = true;
      res.chain = chain_with(chain, key);
      return res;
    }`,
      to: ``,
    },
    {
      note: "resolve_prefab: 展开方向反了",
      file: "native/lfw/loader/resolve_prefab.cpp",
      from: `    base = spread_assign(prefab, has_base ? base : Value());`,
      to: `    base = spread_assign(has_base ? base : Value(), prefab);`,
    },
    {
      note: "resolve_prefab: 最终合并方向反了",
      file: "native/lfw/loader/resolve_prefab.cpp",
      from: `  res.value = has_base ? spread_assign(base, obj) : obj;`,
      to: `  res.value = has_base ? spread_assign(obj, base) : obj;`,
    },
    {
      note: "resolve_prefab: 缺失判定用 undefined",
      file: "native/lfw/loader/resolve_prefab.cpp",
      from: `    if (!truthy(prefab)) {`,
      to: `    if (is_undefined(prefab)) {`,
    },
    {
      note: "resolve_prefab: 空 ref 也进循环",
      file: "native/lfw/loader/resolve_prefab.cpp",
      from: `  while (!is_undefined(ref)) {`,
      to: `  while (truthy(ref) || is_undefined(ref)) {`,
    },
    {
      note: "resolve_prefab: missing/cycle 分支互换",
      file: "native/lfw/loader/resolve_prefab.cpp",
      from: `  if (!r.cycle) {
    const std::u16string last = r.chain.empty() ? std::u16string() : r.chain[r.chain.size() - 1];`,
      to: `  if (r.cycle) {
    const std::u16string last = r.chain.empty() ? std::u16string() : r.chain[r.chain.size() - 1];`,
    },
    {
      note: "resolve_prefab: 链连接符变了",
      file: "native/lfw/loader/resolve_prefab.cpp",
      from: `    if (i > 0) joined += u" -> ";`,
      to: `    if (i > 0) joined += u", ";`,
    },
  ],
};
