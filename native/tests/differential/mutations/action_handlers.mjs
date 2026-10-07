// action_handlers 变异规格
//
// 覆盖 native/lfw/collision/action_handlers.cpp 的全部动作分发与 apply_buff。
//
// 按证明删除的变异（不可观测）：
// 1) `if (type == action_type::kNONE) return Value();` 整行删除：
//    该分支与函数末尾的兜底都是返回 undefined，无差异。
export default {
  subject: "action_handlers",
  mutations: [
    // ---- A_SOUND / V_SOUND ----
    {
      note: "A_SOUND 打到受击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    attacker.play_sound(field_of(data, u\"path\"), field_of(data, u\"pos\"));",
      to: "    victim.play_sound(field_of(data, u\"path\"), field_of(data, u\"pos\"));",
    },
    {
      note: "A_SOUND 取 pos 当路径",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    attacker.play_sound(field_of(data, u\"path\"), field_of(data, u\"pos\"));",
      to: "    attacker.play_sound(field_of(data, u\"pos\"), field_of(data, u\"path\"));",
    },
    {
      note: "V_SOUND 打到攻击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    victim.play_sound(field_of(data, u\"path\"), field_of(data, u\"pos\"));",
      to: "    attacker.play_sound(field_of(data, u\"path\"), field_of(data, u\"pos\"));",
    },
    // ---- A_NEXT_FRAME / V_NEXT_FRAME ----
    {
      note: "A_NEXT_FRAME 进受击方帧",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    attacker.enter_frame(data_of(action));",
      to: "    victim.enter_frame(data_of(action));",
    },
    {
      note: "A_NEXT_FRAME 用整个动作当帧",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    attacker.enter_frame(data_of(action));",
      to: "    attacker.enter_frame(action);",
    },
    {
      note: "V_NEXT_FRAME 进攻击方帧",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    victim.enter_frame(data_of(action));",
      to: "    attacker.enter_frame(data_of(action));",
    },
    // ---- A_SET_PROP / V_SET_PROP ----
    {
      note: "A_SET_PROP 空名不早退",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const Value name = field_of(data, u\"name\");\n    if (!truthy(name)) return Value();\n    attacker.set_prop(to_string(name), field_of(data, u\"value\"));",
      to: "    const Value name = field_of(data, u\"name\");\n    if (truthy(name)) return Value();\n    attacker.set_prop(to_string(name), field_of(data, u\"value\"));",
    },
    {
      note: "A_SET_PROP 写到受击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    attacker.set_prop(to_string(name), field_of(data, u\"value\"));",
      to: "    victim.set_prop(to_string(name), field_of(data, u\"value\"));",
    },
    {
      note: "A_SET_PROP 用 name 当值",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    attacker.set_prop(to_string(name), field_of(data, u\"value\"));",
      to: "    attacker.set_prop(to_string(name), name);",
    },
    {
      note: "V_SET_PROP 写到攻击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    victim.set_prop(to_string(name), field_of(data, u\"value\"));",
      to: "    attacker.set_prop(to_string(name), field_of(data, u\"value\"));",
    },
    // ---- 四个防御项 ----
    {
      note: "A_DEFEND 返回 undefined",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "  if (type == action_type::kA_BROKEN_DEFEND || type == action_type::kA_DEFEND ||\n      type == action_type::kV_BROKEN_DEFEND || type == action_type::kV_DEFEND) {\n    return Value(0.0);",
      to: "  if (type == action_type::kA_BROKEN_DEFEND || type == action_type::kA_DEFEND ||\n      type == action_type::kV_BROKEN_DEFEND || type == action_type::kV_DEFEND) {\n    return Value();",
    },
    {
      note: "V_DEFEND 漏出防御组",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "  if (type == action_type::kA_BROKEN_DEFEND || type == action_type::kA_DEFEND ||\n      type == action_type::kV_BROKEN_DEFEND || type == action_type::kV_DEFEND) {",
      to: "  if (type == action_type::kA_BROKEN_DEFEND || type == action_type::kA_DEFEND ||\n      type == action_type::kV_BROKEN_DEFEND) {",
    },
    {
      note: "A_BROKEN_DEFEND 漏出防御组",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "  if (type == action_type::kA_BROKEN_DEFEND || type == action_type::kA_DEFEND ||\n      type == action_type::kV_BROKEN_DEFEND || type == action_type::kV_DEFEND) {",
      to: "  if (type == action_type::kA_DEFEND ||\n      type == action_type::kV_BROKEN_DEFEND || type == action_type::kV_DEFEND) {",
    },
    // ---- 速度反转 ----
    {
      note: "A_REBOUND_VX 未取负",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    attacker.set_velocity_x(Value(-attacker.velocity_x()));",
      to: "    attacker.set_velocity_x(Value(attacker.velocity_x()));",
    },
    {
      note: "A_REBOUND_VX 反受击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    attacker.set_velocity_x(Value(-attacker.velocity_x()));",
      to: "    victim.set_velocity_x(Value(-victim.velocity_x()));",
    },
    {
      note: "V_REBOUND_VX 未取负",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    victim.set_velocity_x(Value(-victim.velocity_x()));",
      to: "    victim.set_velocity_x(Value(victim.velocity_x()));",
    },
    {
      note: "V_REBOUND_VX 反攻击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    victim.set_velocity_x(Value(-victim.velocity_x()));",
      to: "    attacker.set_velocity_x(Value(-attacker.velocity_x()));",
    },
    // ---- 转身 ----
    {
      note: "V_TURN_FACE 不转身",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    victim.set_facing(entity::turn_face(victim.facing()));",
      to: "    victim.set_facing(victim.facing());",
    },
    {
      note: "V_TURN_FACE 转攻击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    victim.set_facing(entity::turn_face(victim.facing()));",
      to: "    attacker.set_facing(entity::turn_face(attacker.facing()));",
    },
    // ---- 换队 ----
    {
      note: "V_TURN_TEAM 无条件用攻击方队伍",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (!truthy(team)) team = attacker.team();",
      to: "    if (truthy(team)) team = attacker.team();",
    },
    {
      note: "V_TURN_TEAM 取受击方队伍",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (!truthy(team)) team = attacker.team();",
      to: "    if (!truthy(team)) team = victim.team();",
    },
    {
      note: "V_TURN_TEAM 写攻击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    victim.set_team(team);",
      to: "    attacker.set_team(team);",
    },
    // ---- FUSION ----
    {
      note: "FUSION 不写 mt.mark",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    env.mt_set_mark(u\"cact_\" + std::u16string(action_type::kFUSION));",
      to: "    (void)env;",
    },
    {
      note: "FUSION 找不到数据仍继续",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (!env.find_data(to_string(field_of(data, u\"oid\")), oid_data)) return Value();",
      to: "    if (env.find_data(to_string(field_of(data, u\"oid\")), oid_data)) return Value();",
    },
    {
      note: "FUSION 攻击方判断取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const int a_v = attacker.is_bot_ctrl() ? 0 : 1;",
      to: "    const int a_v = attacker.is_bot_ctrl() ? 1 : 0;",
    },
    {
      note: "FUSION 受击方判断取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const int v_v = victim.is_bot_ctrl() ? 0 : 1;",
      to: "    const int v_v = victim.is_bot_ctrl() ? 1 : 0;",
    },
    {
      note: "FUSION 随机序取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    } else if (static_cast<int>(env.mt_int()) % 2) {",
      to: "    } else if (!(static_cast<int>(env.mt_int()) % 2)) {",
    },
    {
      note: "FUSION 随机分支主客互换",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    } else if (static_cast<int>(env.mt_int()) % 2) {\n      f1 = &attacker;\n      f2 = &victim;",
      to: "    } else if (static_cast<int>(env.mt_int()) % 2) {\n      f1 = &victim;\n      f2 = &attacker;",
    },
    {
      note: "FUSION 血量只取 f1",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const double hp = to_number(f1->hp()) + to_number(f2->hp());",
      to: "    const double hp = to_number(f1->hp());",
    },
    {
      note: "FUSION hp_r 用 min",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const double hp_r = max({hp, to_number(f1->hp_r()), to_number(f2->hp_r())});",
      to: "    const double hp_r = min({hp, to_number(f1->hp_r()), to_number(f2->hp_r())});",
    },
    {
      note: "FUSION 丢掉 f2.hp_r",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const double hp_r = max({hp, to_number(f1->hp_r()), to_number(f2->hp_r())});",
      to: "    const double hp_r = max({hp, to_number(f1->hp_r())});",
    },
    {
      note: "FUSION dismiss_data 取 f2",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f1->set_dismiss_data(f1->data());",
      to: "    f1->set_dismiss_data(f2->data());",
    },
    {
      note: "FUSION transform 用自身数据",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f1->transform(oid_data);",
      to: "    f1->transform(f1->data());",
    },
    {
      note: "FUSION hp 用 max",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f1->set_hp(Value(min(hp, to_number(f1->hp_max()))));",
      to: "    f1->set_hp(Value(max(hp, to_number(f1->hp_max()))));",
    },
    {
      note: "FUSION hp_r 上限用自身 hp_r",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f1->set_hp_r(Value(min(hp_r, to_number(f1->hp_max()))));",
      to: "    f1->set_hp_r(Value(min(hp_r, to_number(f1->hp_r()))));",
    },
    {
      note: "FUSION fuse_bys 记 f1 的 id",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f1->set_fuse_bys(ensure(fuse, Value(f2->id())));",
      to: "    f1->set_fuse_bys(ensure(fuse, Value(f1->id())));",
    },
    {
      note: "FUSION dismiss_time 缺省取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f1->set_dismiss_time(missing(time_v) ? Value(NullTag{}) : time_v);",
      to: "    f1->set_dismiss_time(missing(time_v) ? time_v : Value(NullTag{}));",
    },
    {
      note: "FUSION mp 取 f2 上限",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f1->set_mp(f1->mp_max());",
      to: "    f1->set_mp(f2->mp_max());",
    },
    {
      note: "FUSION 漏设 motionless",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f2->set_invulnerable(1000000);\n    f2->set_motionless(1000000);\n    f2->set_invisible(1000000);",
      to: "    f2->set_invulnerable(1000000);\n    f2->set_invisible(1000000);",
    },
    {
      note: "FUSION 三连赋值次序反了",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    f2->set_invulnerable(1000000);\n    f2->set_motionless(1000000);",
      to: "    f2->set_motionless(1000000);\n    f2->set_invulnerable(1000000);",
    },
    {
      note: "FUSION act 判定取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (truthy(act)) f1->enter_frame(act);",
      to: "    if (!truthy(act)) f1->enter_frame(act);",
    },
    {
      note: "FUSION 进帧用整个 data",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (truthy(act)) f1->enter_frame(act);",
      to: "    if (truthy(act)) f1->enter_frame(data);",
    },
    // ---- BROADCAST / ERROR ----
    {
      note: "BROADCAST 取错字段",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    env.broadcast(to_string(field_of(data_of(action), u\"msg\")));",
      to: "    env.broadcast(to_string(field_of(data_of(action), u\"path\")));",
    },
    {
      note: "ERROR 改走广播",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    env.alert(to_string(field_of(data_of(action), u\"msg\")));",
      to: "    env.broadcast(to_string(field_of(data_of(action), u\"msg\")));",
    },
    // ---- VALUE_STEAL ----
    {
      note: "STEAL 伤害取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const Value itr_value = truthy(over_injury) ? injury : real_injury;",
      to: "    const Value itr_value = truthy(over_injury) ? real_injury : injury;",
    },
    {
      note: "STEAL 伤害为假仍继续",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (!truthy(itr_value)) return Value();",
      to: "    if (truthy(itr_value)) return Value();",
    },
    {
      note: "STEAL target1 当 target2",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (target == 1) {\n      const Value se = attacker.src_emitter();",
      to: "    if (target == 2) {\n      const Value se = attacker.src_emitter();",
    },
    {
      note: "STEAL target3 不取 bearer",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    } else if (target == 3) {\n      t = attacker.bearer();\n    } else {",
      to: "    } else if (target == 3) {\n      t = &attacker;\n    } else {",
    },
    {
      note: "STEAL 找不到目标仍继续",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (t == nullptr) return Value();",
      to: "    if (t == nullptr) t = &attacker;",
    },
    {
      note: "STEAL revive 判定取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (!truthy(field_of(d, u\"revive\")) && to_number(t->hp()) <= 0) return Value();",
      to: "    if (truthy(field_of(d, u\"revive\")) && to_number(t->hp()) <= 0) return Value();",
    },
    {
      note: "STEAL 死亡判定用小于",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (!truthy(field_of(d, u\"revive\")) && to_number(t->hp()) <= 0) return Value();",
      to: "    if (!truthy(field_of(d, u\"revive\")) && to_number(t->hp()) < 0) return Value();",
    },
    {
      note: "STEAL mp 用 max",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "      t->set_mp(Value(min(to_number(t->mp()) + to_number(mp_v), to_number(t->mp_max()))));",
      to: "      t->set_mp(Value(max(to_number(t->mp()) + to_number(mp_v), to_number(t->mp_max()))));",
    },
    {
      note: "STEAL mp 上限用 hp_max",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "      t->set_mp(Value(min(to_number(t->mp()) + to_number(mp_v), to_number(t->mp_max()))));",
      to: "      t->set_mp(Value(min(to_number(t->mp()) + to_number(mp_v), to_number(t->hp_max()))));",
    },
    {
      note: "STEAL itr_mp_ratio 换了系数",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "      t->set_mp(Value(min(to_number(t->mp()) + round(to_number(itr_value) * to_number(itr_mp_ratio)),\n                          to_number(t->mp_max()))));",
      to: "      t->set_mp(Value(min(to_number(t->mp()) + round(to_number(itr_value) * to_number(field_of(d, u\"itr_hp_ratio\"))),\n                          to_number(t->mp_max()))));",
    },
    {
      note: "STEAL hp_r 上限用 hp_r",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "      t->set_hp_r(Value(min(to_number(t->hp_r()) + to_number(hp_r_v), to_number(t->hp_max()))));",
      to: "      t->set_hp_r(Value(min(to_number(t->hp_r()) + to_number(hp_r_v), to_number(t->hp_r()))));",
    },
    {
      note: "STEAL over_hp_r 分支用 hp_max",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "        t->set_hp(Value(min(to_number(t->hp()) + to_number(hp_v), to_number(t->hp_r()))));",
      to: "        t->set_hp(Value(min(to_number(t->hp()) + to_number(hp_v), to_number(t->hp_max()))));",
    },
    {
      note: "STEAL 非 over_hp_r 分支用 hp_r",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "        t->set_hp(Value(min(to_number(t->hp()) + to_number(hp_v), to_number(t->hp_max()))));",
      to: "        t->set_hp(Value(min(to_number(t->hp()) + to_number(hp_v), to_number(t->hp_r()))));",
    },
    {
      note: "STEAL itr_hp_ratio 换成 itr_hp_r_ratio",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "      const Value itr_hp_ratio = field_of(d, u\"itr_hp_ratio\");\n      if (truthy(itr_hp_ratio))",
      to: "      const Value itr_hp_ratio = field_of(d, u\"itr_hp_r_ratio\");\n      if (truthy(itr_hp_ratio))",
    },
    {
      note: "STEAL 收尾 hp_r 用 min",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    t->set_hp_r(Value(max(to_number(t->hp_r()), to_number(t->hp()))));",
      to: "    t->set_hp_r(Value(min(to_number(t->hp_r()), to_number(t->hp()))));",
    },
    {
      note: "STEAL 无 data 仍继续",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const Value d = data_of(action);\n    if (!truthy(d)) return Value();",
      to: "    const Value d = data_of(action);\n    if (truthy(d)) return Value();",
    },
    // ---- BUFF ----
    {
      note: "BUFF 目标类型位用攻击方",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (!bit_and(hf, to_number(victim.data_type()))) return;",
      to: "    if (!bit_and(hf, to_number(attacker.data_type()))) return;",
    },
    {
      note: "BUFF 队伍标志取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    const double ally_flag = env.is_ally(attacker, victim) ? static_cast<double>(HitFlag::Ally)\n                                                          : static_cast<double>(HitFlag::Enemy);",
      to: "    const double ally_flag = env.is_ally(attacker, victim) ? static_cast<double>(HitFlag::Enemy)\n                                                          : static_cast<double>(HitFlag::Ally);",
    },
    {
      note: "BUFF 队伍位判定取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    if (!bit_and(hf, ally_flag)) return;",
      to: "    if (bit_and(hf, ally_flag)) return;",
    },
    {
      note: "BUFF 数字判定取反",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "  if (is_number(hitflag)) {",
      to: "  if (!is_number(hitflag)) {",
    },
    {
      note: "BUFF duration 缺省值错",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "  const double duration = missing(duration_v) ? 0 : to_number(duration_v);",
      to: "  const double duration = missing(duration_v) ? 1 : to_number(duration_v);",
    },
    {
      note: "BUFF kind 缺省值错",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "  const std::u16string kind = missing(buff_v) ? std::u16string() : to_string(buff_v);",
      to: "  const std::u16string kind = missing(buff_v) ? std::u16string(u\"x\") : to_string(buff_v);",
    },
    {
      note: "BUFF duration 用 hitflag 字段",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "  const double duration = missing(duration_v) ? 0 : to_number(duration_v);",
      to: "  const double duration = missing(duration_v) ? 0 : to_number(hitflag);",
    },
    {
      note: "A_BUFF 主客互换",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    apply_buff(env, action, victim, attacker);",
      to: "    apply_buff(env, action, attacker, victim);",
    },
    {
      note: "V_BUFF 主客互换",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "    apply_buff(env, action, attacker, victim);",
      to: "    apply_buff(env, action, victim, attacker);",
    },
    {
      note: "BUFF 无 data 仍继续",
      file: "native/lfw/collision/action_handlers.cpp",
      from: "  const Value data = data_of(action);\n  if (!truthy(data)) return;",
      to: "  const Value data = data_of(action);\n  if (truthy(data)) return;",
    },
  ],
};
