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
      note: "float_scaling_itr 用 round 而不是 floor",
      file: "native/lfw/dat_translator/cookers.cpp",
      from: `      o->set(std::u16string(k), Value(js_floor(10000 * std::get<double>(*x))));`,
      to: `      o->set(std::u16string(k), Value(js_round(10000 * std::get<double>(*x))));`,
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
  ],
};
