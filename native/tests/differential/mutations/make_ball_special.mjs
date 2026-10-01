export default {
  subject: "make_ball_special",
  mutations: [
    {
      note: "FirenFlame 的判定写成 FirzenBall",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  if (id == oid::kFirenFlame) {`,
      to: `  if (id == oid::kFirzenBall) {`,
    },
    {
      note: "FirenFlame: hit_flag 写成 AllyFighter",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `          if (io != nullptr) set_hit_flag(*io, en(HitFlag::AllEnemy));`,
      to: `          if (io != nullptr) set_hit_flag(*io, en(HitFlag::AllyFighter));`,
    },
    {
      note: "FirenFlame: 遍历 bdy 而不是 itr",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `        Array* arr = array_of(*fo, u"itr");
        if (arr == nullptr) return;
        for (size_t i = 0; i < arr->size(); ++i) {
          Object* io = as_object(arr->at(i));
          if (io != nullptr) set_hit_flag(*io, en(HitFlag::AllEnemy));`,
      to: `        Array* arr = array_of(*fo, u"bdy");
        if (arr == nullptr) return;
        for (size_t i = 0; i < arr->size(); ++i) {
          Object* io = as_object(arr->at(i));
          if (io != nullptr) set_hit_flag(*io, en(HitFlag::AllEnemy));`,
    },
    {
      note: "FirzenBall/BatBall: no_shadow 键名写错",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  if (id == oid::kFirzenBall) {
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* fo = as_object(frame);
        if (fo == nullptr) return;
        fo->set(u"no_shadow", n(1));`,
      to: `  if (id == oid::kFirzenBall) {
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* fo = as_object(frame);
        if (fo == nullptr) return;
        fo->set(u"no_shadow2", n(1));`,
    },
    {
      note: "FirzenBall/BatBall: vzm 写成 Acc",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  if (id == oid::kBatBall) {
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* fo = as_object(frame);
        if (fo == nullptr) return;
        fo->set(u"no_shadow", n(1));
        fo->set(u"dvz", n(0));
        fo->set(u"vzm", en(SpeedMode::Fixed));`,
      to: `  if (id == oid::kBatBall) {
    if (frames_v != nullptr) {
      traversal(*frames_v, [](const std::u16string&, Value& frame) {
        Object* fo = as_object(frame);
        if (fo == nullptr) return;
        fo->set(u"no_shadow", n(1));
        fo->set(u"dvz", n(0));
        fo->set(u"vzm", en(SpeedMode::Acc));`,
    },
    {
      note: "BatBall 的 case 被删掉",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  if (id == oid::kBatBall) {`,
      to: `  if (false) {`,
    },
    {
      note: "JanChase: drop_sounds 赋成空值",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  if (id == oid::kJanChase) {
    if (base != nullptr) {
      base->set(u"drop_sounds", field_or_any(*base, u"hit_sounds"));
    }`,
      to: `  if (id == oid::kJanChase) {
    if (base != nullptr) {
      base->set(u"drop_sounds", Value());
    }`,
    },
    {
      note: "JanChase: frames 少写 52",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  if (id == oid::kJanChase) {
    if (base != nullptr) {
      base->set(u"drop_sounds", field_or_any(*base, u"hit_sounds"));
    }
    for (const char16_t* key : {u"50", u"51", u"52"}) {`,
      to: `  if (id == oid::kJanChase) {
    if (base != nullptr) {
      base->set(u"drop_sounds", field_or_any(*base, u"hit_sounds"));
    }
    for (const char16_t* key : {u"50", u"51"}) {`,
    },
    {
      note: "JanChase: behavior 判定写成 Bat",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `          const Value item = make_obj({{u"oid", s(oid::kJanChase)},
                                       {u"action", make_obj({{u"id", s(u"40")}})},
                                       {u"x", field_or_any(*f, u"centerx")},
                                       {u"y", field_or_any(*f, u"centery")},`,
      to: `          const Value item = make_obj({{u"oid", s(oid::kJanChase)},
                                       {u"action", make_obj({{u"id", s(u"40")}})},
                                       {u"x", field_or_any(*f, u"centery")},
                                       {u"y", field_or_any(*f, u"centerx")},`,
    },
    {
      note: "JanChase: opoint 的 interval_id 写成 2",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `                                       {u"x", field_or_any(*f, u"centerx")},
                                       {u"y", field_or_any(*f, u"centery")},
                                       {u"kind", en(OpointKind::Normal)},
                                       {u"ghost", n(1)},
                                       {u"speedz", n(0)},
                                       {u"dvx", n(0)},
                                       {u"dvy", n(0)},
                                       {u"dvz", n(0)},
                                       {u"interval", n(1)},
                                       {u"interval_id", s(u"1")},`,
      to: `                                       {u"x", field_or_any(*f, u"centerx")},
                                       {u"y", field_or_any(*f, u"centery")},
                                       {u"kind", en(OpointKind::Normal)},
                                       {u"ghost", n(1)},
                                       {u"speedz", n(0)},
                                       {u"dvx", n(0)},
                                       {u"dvy", n(0)},
                                       {u"dvz", n(0)},
                                       {u"interval", n(1)},
                                       {u"interval_id", s(u"2")},`,
    },
    {
      note: "edit_tail: oid 写死成 220（JanChaseh 组不再命中）",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `    if (oid_v == nullptr || !strict_equals(*oid_v, s(tail_oid))) return false;`,
      to: `    if (oid_v == nullptr || !strict_equals(*oid_v, s(u"220"))) return false;`,
    },
    {
      note: "edit_tail: action id 判定写成 41",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `    return id_v != nullptr && strict_equals(*id_v, s(u"40"));`,
      to: `    return id_v != nullptr && strict_equals(*id_v, s(u"41"));`,
    },
    {
      note: "edit_tail: ghost 写成 0",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  t->set(u"speedz", n(0));
  t->set(u"ghost", n(1));`,
      to: `  t->set(u"speedz", n(0));
  t->set(u"ghost", n(0));`,
    },
    {
      note: "edit_tail: speedz 链的键序写成重要在先",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  t->set(u"unimportant", n(1));
  t->set(u"dvz", n(0));
  t->set(u"dvy", n(0));
  t->set(u"dvx", n(0));
  t->set(u"speedz", n(0));`,
      to: `  t->set(u"unimportant", n(1));
  t->set(u"speedz", n(0));
  t->set(u"dvz", n(0));
  t->set(u"dvy", n(0));
  t->set(u"dvx", n(0));`,
    },
    {
      note: "JanChaseh: invisible 的 nullish 判定写成 falsy",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `    for (const char16_t* key : {u"50", u"51", u"52"}) {
      Object* fr = frame_at(frames, key);
      if (fr == nullptr) continue;
      const Value* inv_v = fr->get(u"invisible");
      if (inv_v == nullptr || is_nullish(*inv_v)) {`,
      to: `    for (const char16_t* key : {u"50", u"51", u"52"}) {
      Object* fr = frame_at(frames, key);
      if (fr == nullptr) continue;
      const Value* inv_v = fr->get(u"invisible");
      if (inv_v == nullptr || !truthy(*inv_v)) {`,
    },
    {
      note: "JanChaseh: invulnerable 直接赋 wait",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `    for (const char16_t* key : {u"50", u"51", u"52"}) {
      Object* fr = frame_at(frames, key);
      if (fr == nullptr) continue;
      const Value* inv_v = fr->get(u"invisible");
      if (inv_v == nullptr || is_nullish(*inv_v)) {
        fr->set(u"invisible", field_or_any(*fr, u"wait"));
      }
      const Value* invl_v = fr->get(u"invulnerable");
      if (invl_v == nullptr || is_nullish(*invl_v)) {
        fr->set(u"invulnerable", field_or_any(*fr, u"invisible"));
      }`,
      to: `    for (const char16_t* key : {u"50", u"51", u"52"}) {
      Object* fr = frame_at(frames, key);
      if (fr == nullptr) continue;
      const Value* inv_v = fr->get(u"invisible");
      if (inv_v == nullptr || is_nullish(*inv_v)) {
        fr->set(u"invisible", field_or_any(*fr, u"wait"));
      }
      const Value* invl_v = fr->get(u"invulnerable");
      if (invl_v == nullptr || is_nullish(*invl_v)) {
        fr->set(u"invulnerable", field_or_any(*fr, u"wait"));
      }`,
    },
    {
      note: "FirzenChasef: frames 少写 81",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `    for (const char16_t* key : {u"59", u"80", u"81"}) {`,
      to: `    for (const char16_t* key : {u"59", u"80"}) {`,
    },
    {
      note: "FirzenChasef: x 取 h",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `          const Value item = make_obj({{u"oid", s(oid::kFirzenChasef)},
                                       {u"action", make_obj({{u"id", s(u"40")}})},
                                       {u"x", n(pic_half(*f, u"w"))},`,
      to: `          const Value item = make_obj({{u"oid", s(oid::kFirzenChasef)},
                                       {u"action", make_obj({{u"id", s(u"40")}})},
                                       {u"x", n(pic_half(*f, u"h"))},`,
    },
    {
      note: "pic_half: 除以 3",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `  return floor(v != nullptr ? to_number(*v) / 2 : 0.0);`,
      to: `  return floor(v != nullptr ? to_number(*v) / 3 : 0.0);`,
    },
    {
      note: "FreezableBall 组写成 Freezer",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `      base->set(u"group", ensure(cur, s(entity_group::kFreezableBall)));`,
      to: `      base->set(u"group", ensure(cur, s(entity_group::kFreezer)));`,
    },
    {
      note: "FreezeBall 组写成 FreezableBall",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `      base->set(u"group", ensure(cur, s(entity_group::kFreezer)));`,
      to: `      base->set(u"group", ensure(cur, s(entity_group::kFreezableBall)));`,
    },
    {
      note: "BatChase: behavior 判定写成 ChasingSameEnemy",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `        const Value* beh = fo->get(u"behavior");
        if (beh == nullptr || !strict_equals(*beh, en(FrameBehavior::Bat))) return;`,
      to: `        const Value* beh = fo->get(u"behavior");
        if (beh == nullptr || !strict_equals(*beh, en(FrameBehavior::ChasingSameEnemy))) return;`,
    },
    {
      note: "BatChase: itr_hp_ratio 数值写成 0.3",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `                                   {u"itr_hp_ratio", n(0.2)},`,
      to: `                                   {u"itr_hp_ratio", n(0.3)},`,
    },
    {
      note: "BatChase: 动作类型写成 V_NEXT_FRAME",
      file: "native/lfw/dat_translator/make_ball_special.cpp",
      from: `              {{u"type", s(action_type::kVALUE_STEAL)},`,
      to: `              {{u"type", s(action_type::kV_NEXT_FRAME)},`,
    },
  ],
};
