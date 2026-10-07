export default {
  subject: "cookers",
  mutations: [
    {
      note: "cook_bdy 不再按字段表重排",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  reorder_fields(bdy, bdy_info_fields());`,
      to: ``,
    },
    {
      note: "cook_bdy 的 kind 归一化失效（直接当 NaN）",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  const double kind = to_number(take(*o, u"kind"));`,
      to: `  take(*o, u"kind");\n  const double kind = to_number(Value());`,
    },
    {
      note: "cook_wpoint 的 x/y/z 不再保留原值（一律归零）",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  const Value x = take(*o, u"x");
  o->set(u"x", truthy(x) ? x : Value(0.0));`,
      to: `  (void)take(*o, u"x");
  o->set(u"x", Value(0.0));`,
    },
    {
      note: "cook_wpoint 的 kind == 1 改成严格比较",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  if (kindv != nullptr && equals(*kindv, Value(1.0))) {`,
      to: `  if (kindv != nullptr && strict_equals(*kindv, Value(1.0))) {`,
    },
    {
      note: "cook_wpoint 的 cover 判断失效（z 一律 2）",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `    o->set(u"z", Value(cover.has_value() && *cover == 1.0 ? -2.0 : 2.0));`,
      to: `    o->set(u"z", Value(2.0));`,
    },
    {
      note: "cook_wpoint 的 weaponact 不再转字符串",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  o->set(u"weaponact", Value(to_string(take(*o, u"weaponact"))));`,
      to: `  o->set(u"weaponact", take(*o, u"weaponact"));`,
    },
    {
      note: "cook_cpoint 的 cover 11/10 不再覆盖 facing",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `    if (cover.has_value() && (*cover == 11.0 || *cover == 10.0)) {
      if (Object* nfo = as_object(nf)) {
        nfo->set(u"facing", Value(static_cast<double>(FacingFlag::SameAsCatcher)));
      }
    }`,
      to: ``,
    },
    {
      note: "cook_cpoint 有 throwvx 时不再删 facing",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `    const Value* throwvx = o->get(u"throwvx");
    if (throwvx != nullptr && truthy(*throwvx)) {
      if (Object* nfo = as_object(nf)) nfo->remove(u"facing");
    }`,
      to: ``,
    },
    {
      note: "cook_cpoint 不再过滤 NaN 的 injury",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  if (injury.has_value() && truthy(Value(*injury))) o->set(u"injury", Value(abs(*injury)));`,
      to: `  if (injury.has_value()) o->set(u"injury", Value(abs(*injury)));`,
    },
    {
      note: "cook_itr 的 vrest 不再乘 2",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  set_opt(*o, u"vrest", take_positive_num(*o, u"vrest", [](double n) { return 2 * n; }));`,
      to: `  set_opt(*o, u"vrest", take_positive_num(*o, u"vrest", [](double n) { return n; }));`,
    },
    {
      note: "cook_itr 的 dv* 保留位数改成 2",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  set_opt(*o, u"dvx", take_not_zero_num(*o, u"dvx", [](double n) { return fixed_float(n, 4); }));`,
      to: `  set_opt(*o, u"dvx", take_not_zero_num(*o, u"dvx", [](double n) { return fixed_float(n, 2); }));`,
    },
    {
      note: "cook_itr 的 zwidth 换算系数写错",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `    o->set(u"l", Value(4 * *zwidth));`,
      to: `    o->set(u"l", Value(2 * *zwidth));`,
    },
    {
      note: "cook_itr 不再克隆 catchingact/caughtact 的来源（污染共享常量）",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `    Value nf = shallow_clone(get_next_frame_by_raw_id(caughtact, u"frame", u"", nullptr));`,
      to: `    Value nf = get_next_frame_by_raw_id(caughtact, u"frame", u"", nullptr);`,
    },
    {
      note: "cook_itr 的 catchingact 判定写成 is_str",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  const Value catchingact = take(*o, u"catchingact");
  if (is_num(catchingact)) {`,
      to: `  const Value catchingact = take(*o, u"catchingact");
  if (is_str(catchingact)) {`,
    },
    {
      note: "float_scaling_itr 漏掉 dvz",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  static const char16_t* const kKeys[] = {u"dvx", u"dvy", u"dvz"};`,
      to: `  static const char16_t* const kKeys[] = {u"dvx", u"dvy"};`,
    },
    {
      note: "cook_opoint 的 facing 奇偶判定取反",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `      face = std::fmod(f, 2.0) != 0 ? static_cast<double>(FacingFlag::Backward)`,
      to: `      face = std::fmod(f, 2.0) == 0 ? static_cast<double>(FacingFlag::Backward)`,
    },
    {
      note: "cook_opoint 的 2..19 区间下界写成开区间",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `      if (f >= 2 && f <= 19) face = static_cast<double>(FacingFlag::Right);`,
      to: `      if (f > 2 && f <= 19) face = static_cast<double>(FacingFlag::Right);`,
    },
    {
      note: "cook_opoint 的 2..19 区间上界写成开区间",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `      if (f >= 2 && f <= 19) face = static_cast<double>(FacingFlag::Right);`,
      to: `      if (f >= 2 && f < 19) face = static_cast<double>(FacingFlag::Right);`,
    },
    {
      note: "cook_opoint 的 >=20 写成 >20",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `      else if (f >= 20) o->set(u"multi", Value(js_round(f / 10)));`,
      to: `      else if (f > 20) o->set(u"multi", Value(js_round(f / 10)));`,
    },
    {
      note: "cook_opoint 的 multi 用 floor 而不是 round",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `      else if (f >= 20) o->set(u"multi", Value(js_round(f / 10)));`,
      to: `      else if (f >= 20) o->set(u"multi", Value(js_floor(f / 10)));`,
    },
    {
      note: "cook_opoint 的 dvx 不再减半",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  if (not_zero_num(dvx)) o->set(u"dvx", Value(std::get<double>(dvx) * 0.5));`,
      to: `  if (not_zero_num(dvx)) o->set(u"dvx", dvx);`,
    },
    {
      note: "cook_opoint 的 dvy 符号写错",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  if (not_zero_num(dvy)) o->set(u"dvy", Value(std::get<double>(dvy) * -0.5));`,
      to: `  if (not_zero_num(dvy)) o->set(u"dvy", Value(std::get<double>(dvy) * 0.5));`,
    },
    {
      note: "cook_opoint 的 dvx 缺省不再补 0",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  else o->set(u"dvx", Value(0.0));`,
      to: ``,
    },
    {
      note: "cook_opoint 的 dvz 用 is_num 而不是 not_zero_num",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  if (not_zero_num(dvz)) o->set(u"dvz", Value(std::get<double>(dvz) * 0.5));`,
      to: `  if (is_num(dvz)) o->set(u"dvz", Value(std::get<double>(dvz) * 0.5));`,
    },
    {
      note: "cook_opoint 的 frame.state 用宽松比较",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  return s != nullptr && strict_equals(*s, Value(static_cast<double>(want)));`,
      to: `  return s != nullptr && equals(*s, Value(static_cast<double>(want)));`,
    },
    {
      note: "cook_opoint 漏掉 Weapon_Throwing 状态",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `      state_is(frame, StateEnum::Weapon_Throwing) ||`,
      to: `      false ||`,
    },
    {
      note: "cook_opoint 漏掉 Ball_3006 状态",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  if (state_is(frame, StateEnum::Ball_Flying) || state_is(frame, StateEnum::Ball_3006) ||`,
      to: `  if (state_is(frame, StateEnum::Ball_Flying) ||`,
    },
    {
      note: "cook_opoint 的硬编码帧列表漏掉 109",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  return id == u"50" || id == u"54" || id == u"109";`,
      to: `  return id == u"50" || id == u"54";`,
    },
    {
      note: "cook_opoint 的 oid 不再归一化字符串",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `  o->set(u"oid", Value(to_string(take(*o, u"oid"))));`,
      to: `  o->set(u"oid", take(*o, u"oid"));`,
    },
    {
      note: "cook_opoint 的 FirenFlame 速度读错常量",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `                                 : Value(defines::num(u"Defines.DEFAULT_FIREN_FLAME_SPEED_Z")));`,
      to: `                                 : Value(defines::num(u"Defines.DEFAULT_OPOINT_SPEED_Z")));`,
    },
    {
      note: "cook_opoint 的零速 oid 列表漏掉 BatBall",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `             oid == std::u16string(oid::kBatBall) || oid == std::u16string(oid::kJanChase) ||`,
      to: `             oid == std::u16string(oid::kJanChase) ||`,
    },
    {
      note: "make_frame_state: Ball_3005 不再设 no_shadow",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `  if (is_state(state, StateEnum::Ball_3005)) {
    o->set(u"no_shadow", num(1));
  } else if (is_state(state, StateEnum::HeavyWeapon_OnHand) ||`,
      to: `  if (is_state(state, StateEnum::Ball_3005)) {
  } else if (is_state(state, StateEnum::HeavyWeapon_OnHand) ||`,
    },
    {
      note: "make_frame_state: gravity_enabled 写反",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `    o->set(u"gravity_enabled", Value(false));`,
      to: `    o->set(u"gravity_enabled", Value(true));`,
    },
    {
      note: "make_frame_state: 漏掉 HeavyWeapon_OnHand 分支",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `  } else if (is_state(state, StateEnum::HeavyWeapon_OnHand) ||
             is_state(state, StateEnum::Weapon_OnHand)) {`,
      to: `  } else if (is_state(state, StateEnum::Weapon_OnHand)) {`,
    },
    {
      note: "make_frame_state: Burning 不再写 hit_flag",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `    if (const Value* itr = o->get(u"itr")) {
      Value arr = *itr;
      fill_hit_flag(arr);
    }`,
      to: `    if (const Value* itr = o->get(u"itr")) {
      Value arr = *itr;
    }`,
    },
    {
      note: "make_frame_state: Frozen 不再写 hit_flag",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `    if (const Value* bdy = o->get(u"bdy")) {
      Value arr = *bdy;
      fill_hit_flag(arr);
    }`,
      to: `    if (const Value* bdy = o->get(u"bdy")) {
      Value arr = *bdy;
    }`,
    },
    {
      note: "make_frame_state: state 判定用宽松相等（字符串 \"3005\" 也命中）",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `bool is_state(const Value& state, StateEnum want) { return strict_equals(state, state_value(want)); }`,
      to: `bool is_state(const Value& state, StateEnum want) { return equals(state, state_value(want)); }`,
    },
    {
      note: "make_frame_state: Falling 的 kind 判定改用宽松相等（字符串 \"0\" 也命中）",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `    if (kind == nullptr || !strict_equals(*kind, num(static_cast<double>(BdyKind::Normal)))) return;`,
      to: `    if (kind == nullptr || !equals(*kind, num(static_cast<double>(BdyKind::Normal)))) return;`,
    },
    {
      note: "make_frame_state: Falling 的比较量写成 itr_kind",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `    cm.add(text(collision_val::kItrFall), u">=",`,
      to: `    cm.add(text(collision_val::kItrKind), u">=",`,
    },
    {
      note: "make_frame_state: Falling 的比较符 >= 写成 >",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `    cm.add(text(collision_val::kItrFall), u">=",`,
      to: `    cm.add(text(collision_val::kItrFall), u">",`,
    },
    {
      note: "make_frame_state: Falling 丢掉 MagicFlute2 条件",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `    cm.or_(text(collision_val::kItrKind), u"==",
           num(static_cast<double>(ItrKind::MagicFlute2)));
`,
      to: ``,
    },
    {
      note: "make_frame_state: Falling 写死的 test 常量",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `    b->set(u"test", Value(cm.done()));`,
      to: `    b->set(u"test", Value(u"x"));`,
    },
    {
      note: "make_frame_state: LouisCastOff 的 state 写成 Frozen",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `  o.set(u"state", state_value(StateEnum::Attacking));`,
      to: `  o.set(u"state", state_value(StateEnum::Frozen));`,
    },
    {
      note: "make_frame_state: LouisCastOff 第 2 个 opoint 的 x 符号写反",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `                            {u"x", num(39 + offset_z)},
                            {u"y", num(y_b)},
                            {u"z", num(30)},
                            {u"oid", text(oid::kWeapon_LouisArmourA)},
                            {u"dvy", num(dvy_b)},
                            {u"dvx", num(-dvx_b)},
                            {u"dvz", num(dvx_z)},`,
      to: `                            {u"x", num(39 - offset_z)},
                            {u"y", num(y_b)},
                            {u"z", num(30)},
                            {u"oid", text(oid::kWeapon_LouisArmourA)},
                            {u"dvy", num(dvy_b)},
                            {u"dvx", num(-dvx_b)},
                            {u"dvz", num(dvx_z)},`,
    },
    {
      note: "make_frame_state: LouisCastOff 第 1 个 opoint 的 oid 写成 A",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `                            {u"oid", text(oid::kWeapon_LouisArmourB)},`,
      to: `                            {u"oid", text(oid::kWeapon_LouisArmourA)},`,
    },
    {
      note: "make_frame_state: LouisCastOff 第 1 个 opoint 的字段顺序颠倒（x/y 互换）",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `  items.push_back(make_obj({{u"kind", num(static_cast<double>(OpointKind::Normal))},
                            {u"x", num(39)},
                            {u"y", num(79)},`,
      to: `  items.push_back(make_obj({{u"kind", num(static_cast<double>(OpointKind::Normal))},
                            {u"y", num(79)},
                            {u"x", num(39)},`,
    },
    {
      note: "make_frame_state: Message 不再设 no_shadow",
      file: "native/lfw/dat_translator/make_frame_state.cpp",
      from: `  } else if (is_state(state, StateEnum::Message)) {
    o->set(u"no_shadow", num(1));
  }`,
      to: `  } else if (is_state(state, StateEnum::Message)) {
  }`,
    },
    {
      note: "ensure: 为假值时忘记写回新数组",
      file: "native/lfw/utils/container_help/ensure.h",
      from: `  Array fresh;
  for (const Value& v : items) fresh.push_back(v);
  output = Value(std::make_shared<Array>(fresh));
  return output;`,
      to: `  Array fresh;
  for (const Value& v : items) fresh.push_back(v);
  return output;`,
    },
    {
      note: "fb: 默认速度表把 acc_z 写错（bat_chase）",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  set_default_speed(o, 14, 0.25, 0.125, 0.125);`,
      to: `  set_default_speed(o, 14, 0.25, 0.125, 0.25);`,
    },
    {
      note: "fb: 默认速度表里 ctrl_z/ctrl_x 的插入顺序颠倒",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  o.set(u"ctrl_z", n(1));
  o.set(u"ctrl_y", n(1));
  o.set(u"ctrl_x", n(1));
}

void put_opoint`,
      to: `  o.set(u"ctrl_x", n(1));
  o.set(u"ctrl_y", n(1));
  o.set(u"ctrl_z", n(1));
}

void put_opoint`,
    },
    {
      note: "fb: boomerang 的 ctrl_x 写成 None",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  o.set(u"ctrl_x", en(SpeedCtrl::Control));
  o.set(u"dvz", n(1.8));`,
      to: `  o.set(u"ctrl_x", en(SpeedCtrl::None));
  o.set(u"dvz", n(1.8));`,
    },
    {
      note: "fb: chasing_same_enemy 的 on_hit_ground 两个 id 互换",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `    it->set(u"on_hit_ground", make_obj({{u"id", s(firzen ? u"60" : u"10")}}));`,
      to: `    it->set(u"on_hit_ground", make_obj({{u"id", s(firzen ? u"10" : u"60")}}));`,
    },
    {
      note: "fb: chasing_same_enemy 的 dvy 写成 -0.5",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  o.set(u"dvy", n(8));
  o.set(u"acc_y", n(-0.25));`,
      to: `  o.set(u"dvy", n(-0.5));
  o.set(u"acc_y", n(-0.25));`,
    },
    {
      note: "fb: same_str 改成宽松相等（数字 id 也命中分支）",
      file: "native/lfw/dat_translator/value_builder.h",
      from: `  return strict_equals(v, Value(std::u16string(str)));`,
      to: `  return equals(v, Value(std::u16string(str)));`,
    },
    {
      // `field_or` 原来内联在 `value_builder.h`，后来抽到独立头文件，故改锚。
      note: "fb: field_or 缺键时返回 0 而不是 undefined",
      file: "native/lfw/utils/container_help/field_or.h",
      from: `  const Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : Value();`,
      to: `  const Value* p = o->get(std::u16string(key));
  return p != nullptr ? *p : Value(0.0);`,
    },
    {
      note: "fb: firzen_disater_start 的 min 写成 3",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `                           {u"min", n(4)},
                           {u"skip_zero", Value(true)}})},`,
      to: `                           {u"min", n(3)},
                           {u"skip_zero", Value(true)}})},`,
    },
    {
      note: "fb: jan_angle_blessing 的 itr kind 写成 MagicFlute",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `      {u"kind", en(ItrKind::Heal)},
      {u"x", n(25)},`,
      to: `      {u"kind", en(ItrKind::MagicFlute)},
      {u"x", n(25)},`,
    },
    {
      note: "fb: jan_angle_blessing 的 itr x 写成 24",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `      {u"kind", en(ItrKind::Heal)},
      {u"x", n(25)},
      {u"y", n(13)},`,
      to: `      {u"kind", en(ItrKind::Heal)},
      {u"x", n(24)},
      {u"y", n(13)},`,
    },
    {
      note: "fb: victim_chasing 的条件常量写成 0",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  cm.and_(s(collision_val::kVictimIsChasing), u"==", n(1));`,
      to: `  cm.and_(s(collision_val::kVictimIsChasing), u"==", n(0));`,
    },
    {
      note: "fb: hp_gt_0 的比较符写成 <",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  cm.and_(s(entity_val::kHP), u">", n(0));`,
      to: `  cm.and_(s(entity_val::kHP), u"<", n(0));`,
    },
    {
      note: "fb: jan_chaseh_start 第二个 opoint 的 y 写成减 40",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `      {u"y", n(add(yv, 40))},`,
      to: `      {u"y", n(sub(yv, 40))},`,
    },
    {
      note: "fb: bat_chase_start 的 oid 写成 JanChase",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `      {u"oid", s(oid::kBatChase)},`,
      to: `      {u"oid", s(oid::kJanChase)},`,
    },
    {
      note: "fb: julian_ball 的 id 上界写成 58",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  if (fid >= 50 && fid <= 59) {`,
      to: `  if (fid >= 50 && fid <= 58) {`,
    },
    {
      note: "fb: julian_ball 的 key_down id 偏移方向写反",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `          make_obj({{u"F", make_obj({{u"id", Value(number_to_string(fid - 50))},`,
      to: `          make_obj({{u"F", make_obj({{u"id", Value(number_to_string(fid + 50))},`,
    },
    {
      note: "fb: julian_ball_start 的 spreading 写成 Spreading",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `      {u"spreading", en(OpointSpreading::FloatRange)},`,
      to: `      {u"spreading", en(OpointSpreading::Spreading)},`,
    },
    {
      note: "fb: volcano 的第 2 个 opoint x 写成 134",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `                            {u"oid", s(oid::kFreezeColumn)},
                            {u"x", n(135)},
                            {u"y", n(24)},
                            {u"action", make_obj({{u"id", s(u"100")}})}}));`,
      to: `                            {u"oid", s(oid::kFreezeColumn)},
                            {u"x", n(134)},
                            {u"y", n(24)},
                            {u"action", make_obj({{u"id", s(u"100")}})}}));`,
    },
    {
      note: "fb: volcano 的 +38 写成 +39",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `                            {u"x", n(add(cx, 38))},
                            {u"y", n(26)},
                            {u"z", n(-15)},
                            {u"action", make_obj({{u"id", s(u"54")}, {u"facing", n(2)}})}}));`,
      to: `                            {u"x", n(add(cx, 39))},
                            {u"y", n(26)},
                            {u"z", n(-15)},
                            {u"action", make_obj({{u"id", s(u"54")}, {u"facing", n(2)}})}}));`,
    },
    {
      note: "fb: 分发器把 AngelBlessingStart / DevilJudgementStart 映射互换",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  } else if (strict_equals(behavior, en(FrameBehavior::AngelBlessingStart))) {
    make_fb_jan_chaseh_start(frame, std::nullopt, std::nullopt);
  } else if (strict_equals(behavior, en(FrameBehavior::DevilJudgementStart))) {
    make_fb_jan_chase_start(frame, std::nullopt, std::nullopt);`,
      to: `  } else if (strict_equals(behavior, en(FrameBehavior::AngelBlessingStart))) {
    make_fb_jan_chase_start(frame, std::nullopt, std::nullopt);
  } else if (strict_equals(behavior, en(FrameBehavior::DevilJudgementStart))) {
    make_fb_jan_chaseh_start(frame, std::nullopt, std::nullopt);`,
    },
    {
      note: "fb: 分发器的 behavior 判定改成宽松相等（字符串 \"4\" 也命中）",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  if (strict_equals(behavior, en(FrameBehavior::AngelBlessing))) {`,
      to: `  if (equals(behavior, en(FrameBehavior::AngelBlessing))) {`,
    },
    {
      note: "fb: 分发器漏掉 JulianBall 分支",
      file: "native/lfw/dat_translator/frame_behavior.cpp",
      from: `  } else if (strict_equals(behavior, en(FrameBehavior::JulianBall))) {
    make_fb_julian_ball(frame);
  }`,
      to: `  }`,
    },
  ],
};
