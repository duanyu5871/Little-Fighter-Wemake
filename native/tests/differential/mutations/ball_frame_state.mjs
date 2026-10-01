export default {
  subject: "ball_frame_state",
  mutations: [
    {
      note: "bdy kind 判定改成严格比较（字符串 \"0\" 不再算 Normal）",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `  return kind != nullptr && equals(*kind, n(static_cast<double>(BdyKind::Normal)));`,
      to: `  return kind != nullptr && strict_equals(*kind, n(static_cast<double>(BdyKind::Normal)));`,
    },
    {
      note: "3000: FreezeColumn 改成严格比较",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `    if (equals(id, s(oid::kFreezeColumn))) edit_bdy_hit_flag_all_both(bdy);`,
      to: `    if (strict_equals(id, s(oid::kFreezeColumn))) edit_bdy_hit_flag_all_both(bdy);`,
    },
    {
      note: "3000: itr kind 判定改成宽松比较（字符串 \"0\" 也命中）",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `    const Value* kind = io->get(u"kind");
    if (kind == nullptr || !strict_equals(*kind, n(itr_num(ItrKind::Normal)))) continue;
    Value actions = field_or_any(*io, u"actions");`,
      to: `    const Value* kind = io->get(u"kind");
    if (kind == nullptr || !equals(*kind, n(itr_num(ItrKind::Normal)))) continue;
    Value actions = field_or_any(*io, u"actions");`,
    },
    {
      note: "15: 误开 itr 处理",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `  cook_shrink_and_bounce(ctx, *f, false);`,
      to: `  cook_shrink_and_bounce(ctx, *f, true);`,
    },
    {
      note: "3001: FreezeBall 改成宽松比较",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `    if (strict_equals(id, s(oid::kFreezeBall))) {`,
      to: `    if (equals(id, s(oid::kFreezeBall))) {`,
    },
    {
      note: "3001: itr kind 判定改成宽松比较",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `    if (kind == nullptr || !strict_equals(*kind, n(itr_num(ItrKind::Normal)))) continue;
    CondMaker cond;
    cond.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Ball)));`,
      to: `    if (kind == nullptr || !equals(*kind, n(itr_num(ItrKind::Normal)))) continue;
    CondMaker cond;
    cond.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Ball)));`,
    },
    {
      note: "3001: and_not_in 丢掉 PickSecretly",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `                     n(itr_num(ItrKind::MagicFlute2)), n(itr_num(ItrKind::Pick)),
                     n(itr_num(ItrKind::PickSecretly))});`,
      to: `                     n(itr_num(ItrKind::MagicFlute2)), n(itr_num(ItrKind::Pick))});`,
    },
    {
      note: "3001: and_not_in(ItrEffect) 丢掉 MFire1",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `                    {n(effect_num(ItrEffect::Ice2)), n(effect_num(ItrEffect::MFire1))});`,
      to: `                    {n(effect_num(ItrEffect::Ice2))});`,
    },
    {
      note: "3001: 被武器打判定的 AttackerType 写成 Entity",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `        c2.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Weapon)));`,
      to: `        c2.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Entity)));`,
    },
    {
      note: "3001: itr 动作 id 写成 20",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `    item.set(u"type", s(action_type::kA_NEXT_FRAME));
    item.set(u"test", Value(cond.done()));
    item.set(u"data", make_obj({{u"id", s(u"10")}}));`,
      to: `    item.set(u"type", s(action_type::kA_NEXT_FRAME));
    item.set(u"test", Value(cond.done()));
    item.set(u"data", make_obj({{u"id", s(u"20")}}));`,
    },
    {
      note: "ensure_field_array 对 falsy 值不再新建",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `  if (v != nullptr && truthy(*v)) return *v;`,
      to: `  if (v != nullptr) return *v;`,
    },
    {
      note: "3005: 总是丢弃已有 actions 新建空数组",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `        if (!truthy(actions)) {`,
      to: `        if (true) {`,
    },
    {
      note: "3005: bdy 动作类型写成 A_NEXT_FRAME",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `        item.set(u"type", s(action_type::kV_NEXT_FRAME));`,
      to: `        item.set(u"type", s(action_type::kA_NEXT_FRAME));`,
    },
    {
      note: "3005: AttackerState 判定用 Ball_3006",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `        cond.add(s(collision_val::kAttackerState), u"==", n(state_num(StateEnum::Ball_3005)));`,
      to: `        cond.add(s(collision_val::kAttackerState), u"==", n(state_num(StateEnum::Ball_3006)));`,
    },
    {
      note: "3006: special 判定恒为假",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `  const bool special = equals(behavior, n(static_cast<double>(FrameBehavior::JohnChase))) ||`,
      to: `  const bool special = false ||`,
    },
    {
      note: "3006: special 判定丢掉 JohnBiscuitLeaving",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `                       equals(behavior, n(static_cast<double>(FrameBehavior::JohnBiscuitLeaving))) ||`,
      to: `                       equals(behavior, n(static_cast<double>(FrameBehavior::JohnChase))) ||`,
    },
    {
      note: "3006: special 分支 V_REBOUND_VX 写成 V_TURN_TEAM",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `      list.push_back(make_obj({{u"type", s(action_type::kV_REBOUND_VX)},`,
      to: `      list.push_back(make_obj({{u"type", s(action_type::kV_TURN_TEAM)},`,
    },
    {
      note: "3006: special 分支第 4 项 itr_kind 判定写成 == 9",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `      c4.and_(s(collision_val::kItrKind), u"!=", n(itr_num(ItrKind::JohnShield)));`,
      to: `      c4.and_(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));`,
    },
    {
      note: "3006: 普通分支的 itr_kind 判定写成 != 9",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `      cond.one_of(s(collision_val::kAttackerState),
                  {n(state_num(StateEnum::Ball_3005)), n(state_num(StateEnum::Ball_3006))});
      cond.or_(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));`,
      to: `      cond.one_of(s(collision_val::kAttackerState),
                  {n(state_num(StateEnum::Ball_3005)), n(state_num(StateEnum::Ball_3006))});
      cond.or_(s(collision_val::kItrKind), u"!=", n(itr_num(ItrKind::JohnShield)));`,
    },
    {
      note: "3006: itr 的 VictimState 只留 Ball_3005",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `    cond.one_of(s(collision_val::kVictimState),
                {n(state_num(StateEnum::Ball_3005)), n(state_num(StateEnum::Ball_3006))});`,
      to: `    cond.one_of(s(collision_val::kVictimState),
                {n(state_num(StateEnum::Ball_3005))});`,
    },
    {
      note: "3006: itr kind 判定改成宽松比较",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `    const Value* kind = io->get(u"kind");
    if (kind == nullptr || !strict_equals(*kind, n(itr_num(ItrKind::Normal)))) return;
    CondMaker cond;
    cond.one_of(s(collision_val::kVictimState),`,
      to: `    const Value* kind = io->get(u"kind");
    if (kind == nullptr || !equals(*kind, n(itr_num(ItrKind::Normal)))) return;
    CondMaker cond;
    cond.one_of(s(collision_val::kVictimState),`,
    },
    {
      note: "3006: special 分支丢弃已存在的 actions 之外还漏掉 V_TURN_FACE",
      file: "native/lfw/dat_translator/ball_frame_state.cpp",
      from: `      list.push_back(make_obj({{u"type", s(action_type::kV_TURN_FACE)},`,
      to: `      list.push_back(make_obj({{u"type", s(action_type::kV_NEXT_FRAME)},`,
    },
  ],
};
