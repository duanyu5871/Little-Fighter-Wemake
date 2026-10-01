export default {
  subject: "hit_next_frame",
  mutations: [
    {
      note: "drink: 武器类型枚举写成 Stick",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  cond.one_of(s(entity_val::kHolding_W_Type), {n(wt(WeaponEnum::Drink))});`,
      to: `  cond.one_of(s(entity_val::kHolding_W_Type), {n(wt(WeaponEnum::Stick))});`,
    },
    {
      note: "drink: id 写成 56",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  return make_arr({make_obj({{u"id", s(u"55")},
                             {u"desc", s(u"drink")},`,
      to: `  return make_arr({make_obj({{u"id", s(u"56")},
                             {u"desc", s(u"drink")},`,
    },
    {
      note: "drink: mp_mode 写成 0",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `                             {u"desc", s(u"drink")},
                             {u"mp_mode", n(1)},`,
      to: `                             {u"desc", s(u"drink")},
                             {u"mp_mode", n(0)},`,
    },
    {
      note: "super_punch: 比较符写成 >=",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  cond.add(s(entity_val::kRequireSuperPunch), u">", n(0));`,
      to: `  cond.add(s(entity_val::kRequireSuperPunch), u">=", n(0));`,
    },
    {
      note: "super_punch: 比较值写成 1",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  cond.add(s(entity_val::kRequireSuperPunch), u">", n(0));`,
      to: `  cond.add(s(entity_val::kRequireSuperPunch), u">", n(1));`,
    },
    {
      note: "super_punch: facing 写成 Backward",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `                             {u"desc", s(u"super_punch")},`,
      to: `                             {u"desc", s(u"super_punch2")},`,
    },
    {
      note: "punch: id 数组漏掉一个",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `                             {u"id", make_arr({s(u"60"), s(u"65")})},`,
      to: `                             {u"id", make_arr({s(u"60")})},`,
    },
    {
      note: "punch: 丢掉 desc",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `                             {u"desc", s(u"punch")}})});`,
      to: `                             {u"desc2", s(u"punch")}})});`,
    },
    {
      note: "turn_back: void 0 判定改成严格相等（null 不再走 facing 分支）",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  if (equals(back_frame, Value())) {`,
      to: `  if (strict_equals(back_frame, Value())) {`,
    },
    {
      note: "turn_back: 无 back_frame 时 facing 写成 Backward",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `    fo->set(u"facing", en(FacingFlag::Ctrl));`,
      to: `    fo->set(u"facing", en(FacingFlag::Backward));`,
    },
    {
      note: "turn_back: 外层键 B 写成小写 b",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  const Value back = make_obj({{u"B", inner}});`,
      to: `  const Value back = make_obj({{u"b", inner}});`,
    },
    {
      note: "turn_back: wait 写成大写 I",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `                                {u"wait", s(u"i")},`,
      to: `                                {u"wait", s(u"I")},`,
    },
    {
      note: "turn_back: key_down 合并时丢掉已有内容",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  fo->set(u"key_down", assign(key_down, back));`,
      to: `  fo->set(u"key_down", assign(Value(), back));`,
    },
    {
      note: "turn_back: hit 不做合并，直接写原值",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  fo->set(u"hit", assign(hit_v, back));`,
      to: `  fo->set(u"hit", hit_v);`,
    },
    {
      note: "turn_back: 先写 hit 再写 key_down（键插入序变化）",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  const Value* kd = fo->get(u"key_down");
  const Value key_down = kd != nullptr ? *kd : Value();
  fo->set(u"key_down", assign(key_down, back));
  const Value* hit = fo->get(u"hit");
  const Value hit_v = hit != nullptr ? *hit : Value();
  fo->set(u"hit", assign(hit_v, back));`,
      to: `  const Value* hit = fo->get(u"hit");
  const Value hit_v = hit != nullptr ? *hit : Value();
  fo->set(u"hit", assign(hit_v, back));
  const Value* kd = fo->get(u"key_down");
  const Value key_down = kd != nullptr ? *kd : Value();
  fo->set(u"key_down", assign(key_down, back));`,
    },
    {
      note: "jump: id 写成 211",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  return make_arr({make_obj({{u"id", s(u"210")},
                             {u"facing", en(FacingFlag::Ctrl)},`,
      to: `  return make_arr({make_obj({{u"id", s(u"211")},
                             {u"facing", en(FacingFlag::Ctrl)},`,
    },
    {
      note: "defend: id 写成 100",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  return make_arr({make_obj({{u"id", s(u"110")},
                             {u"facing", en(FacingFlag::Ctrl)},`,
      to: `  return make_arr({make_obj({{u"id", s(u"100")},
                             {u"facing", en(FacingFlag::Ctrl)},`,
    },
    {
      note: "weapon_atk: 第一支的武器写成 Drink",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  c1.add(s(entity_val::kHolding_W_Type), u"==", n(wt(WeaponEnum::Baseball)));`,
      to: `  c1.add(s(entity_val::kHolding_W_Type), u"==", n(wt(WeaponEnum::Drink)));`,
    },
    {
      note: "weapon_atk: press_F_B 判定写成 == 0",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `    cc.and_(s(entity_val::kPressFB), u"!=", n(0));`,
      to: `    cc.and_(s(entity_val::kPressFB), u"==", n(0));`,
    },
    {
      note: "weapon_atk: 第二支的 id 数组退化成标量",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `                             {u"id", make_arr({s(u"20"), s(u"25")})},`,
      to: `                             {u"id", s(u"20")},`,
    },
    {
      note: "jump_atk: 第三支加上 facing（多出字段）",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `                   make_obj({{u"id", s(u"80")},
                             {u"desc", s(u"\\u8df3\\u8dc3\\u653b\\u51fb")},
                             {u"facing", en(FacingFlag::Ctrl)}})});`,
      to: `                   make_obj({{u"id", s(u"80")},
                             {u"desc", s(u"\\u8df3\\u8dc3\\u653b\\u51fb")},
                             {u"facing", en(FacingFlag::L)}})});`,
    },
    {
      note: "jump_atk: 第一支的 or 改成 and",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  c1.or_([&](CondMaker& cc) {
    cc.add(s(entity_val::kPressFB), u"!=", n(0));
    cc.and_(s(entity_val::kHolding_W_Type), u"!=", n(wt(WeaponEnum::None)));`,
      to: `  c1.and_([&](CondMaker& cc) {
    cc.add(s(entity_val::kPressFB), u"!=", n(0));
    cc.and_(s(entity_val::kHolding_W_Type), u"!=", n(wt(WeaponEnum::None)));`,
    },
    {
      note: "jump_atk: 第二支的武器列表漏掉 Stick",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `  c2.one_of(s(entity_val::kHolding_W_Type),
            {n(wt(WeaponEnum::Knife)), n(wt(WeaponEnum::Stick))});
  return make_arr({make_obj({{u"id", s(u"52")},`,
      to: `  c2.one_of(s(entity_val::kHolding_W_Type),
            {n(wt(WeaponEnum::Knife))});
  return make_arr({make_obj({{u"id", s(u"52")},`,
    },
    {
      note: "wt 辅助加上偏移",
      file: "native/lfw/dat_translator/hit_next_frame.cpp",
      from: `double wt(WeaponEnum v) { return static_cast<double>(v); }`,
      to: `double wt(WeaponEnum v) { return static_cast<double>(v) + 1; }`,
    },
    {
      note: "assign: 丢掉 output 原值（总是新建）",
      file: "native/lfw/utils/container_help/assign.h",
      from: `  Value target = truthy(output) ? output : Value(std::make_shared<Object>());`,
      to: `  Value target = Value(std::make_shared<Object>());`,
    },
    {
      note: "assign: 拷贝时把值写成 undefined",
      file: "native/lfw/utils/container_help/assign.h",
      from: `    if (v != nullptr) t->set(k, *v);`,
      to: `    if (v != nullptr) t->set(k, Value());`,
    },
  ],
};
