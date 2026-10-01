export default {
  subject: "ball_bdy",
  mutations: [
    {
      note: "_20: FreezeColumn 改用严格相等（数字 212 不再命中）",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `  if (equals(id, s(oid::kFreezeColumn))) {`,
      to: `  if (strict_equals(id, s(oid::kFreezeColumn))) {`,
    },
    {
      note: "_20: FreezeBall 改用宽松相等（数字 209 也命中）",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `  if (strict_equals(id, s(oid::kFreezeBall))) {`,
      to: `  if (equals(id, s(oid::kFreezeBall))) {`,
    },
    {
      note: "_20: 冰柱分支丢掉 a_emitter != v_emitter",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `    co.add(s(collision_val::kAEmitter), u"!=", s(collision_val::kVEmitter));
`,
      to: ``,
    },
    {
      note: "_20: 被武器打的条件里丢掉 attacker_state != Weapon_OnHand",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `        c2.and_(s(collision_val::kAttackerState), u"!=", n(state_num(StateEnum::Weapon_OnHand)));`,
      to: `        (void)0;`,
    },
    {
      note: "_20: AttackerType == Entity 写成 Ball",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `      cc.or_(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Entity)));`,
      to: `      cc.or_(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Ball)));`,
    },
    {
      note: "_20: 被火跑打的条件整块丢掉",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `      cc.or_([&](CondMaker& c2) {
        c2.add(s(collision_val::kAttackerType), u"==", n(ent_num(EntityEnum::Fighter)));
        c2.and_(s(collision_val::kAttackerState), u"==", n(state_num(StateEnum::BurnRun)));
        return &c2;
      });
`,
      to: ``,
    },
    {
      note: "_30: hit_flag 写成 Fighter",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `  fields.set(u"hit_flag", n(static_cast<double>(HitFlag::AllBoth)));`,
      to: `  fields.set(u"hit_flag", n(static_cast<double>(HitFlag::Fighter)));`,
    },
    {
      note: "_30: one_of 列表漏掉 WeaponSwing",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `              {n(itr_num(ItrKind::Normal)), n(itr_num(ItrKind::CharacterThrew)),
               n(itr_num(ItrKind::WeaponSwing))});`,
      to: `              {n(itr_num(ItrKind::Normal)), n(itr_num(ItrKind::CharacterThrew))});`,
    },
    {
      note: "_30: 第一支的 same_team 写成 1",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `        c3.add(s(collision_val::kSameTeam), u"==", n(0));`,
      to: `        c3.add(s(collision_val::kSameTeam), u"==", n(1));`,
    },
    {
      note: "_30: 第二支的 same_facing 写成 1",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `        c3.and_(s(collision_val::kSameFacing), u"==", n(0));`,
      to: `        c3.and_(s(collision_val::kSameFacing), u"==", n(1));`,
    },
    {
      note: "_30: JohnShield 写成 Normal",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `  co.add(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::JohnShield)));`,
      to: `  co.add(s(collision_val::kItrKind), u"==", n(itr_num(ItrKind::Normal)));`,
    },
    {
      note: "clone: 退化成浅拷贝（会污染源 bdy）",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `  Value copy = deep_copy(bdy);`,
      to: `  Value copy = bdy;`,
    },
    {
      note: "deco: kind_name 用 full_name 版",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `    o->set(u"kind_name", Value(defines::bdy_kind_name(*kind)));`,
      to: `    o->set(u"kind_name", Value(defines::bdy_kind_full_name(*kind)));`,
    },
    {
      note: "deco: hit_flag 的 undefined 判定改成 nullish",
      file: "native/lfw/dat_translator/ball_bdy.cpp",
      from: `  if (hf != nullptr && !is_undefined(*hf)) {`,
      to: `  if (hf != nullptr && !std::holds_alternative<NullTag>(*hf)) {`,
    },
  ],
};
