export default {
  subject: "bots_data",
  mutations: [
    {
      note: "BotMaker 构造时 oid 写成 id",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `  o.set(u"id", Value(std::u16string(oid)));
  o.set(u"oid", Value(std::u16string(oid)));`,
      to: `  o.set(u"id", Value(std::u16string(oid)));
  o.set(u"id", Value(std::u16string(oid)));`,
    },
    {
      note: "BotMaker 构造时少了 actions",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `  o.set(u"actions", Value(std::make_shared<Object>()));
  _bot = Value(std::make_shared<Object>(o));`,
      to: `  _bot = Value(std::make_shared<Object>(o));`,
    },
    {
      note: "BotMaker 构造的 id/oid 键序互换",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `  Object o;
  o.set(u"id", Value(std::u16string(oid)));
  o.set(u"oid", Value(std::u16string(oid)));`,
      to: `  Object o;
  o.set(u"oid", Value(std::u16string(oid)));
  o.set(u"id", Value(std::u16string(oid)));`,
    },
    {
      note: "as_action(Value) 直接丢值",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `Value as_action(const Value& v) { return v; }`,
      to: `Value as_action(const Value& v) { return Value(); }`,
    },
    {
      note: "as_action(func) 不调用构建器",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `Value as_action(const EditBotActionFunc& f) { return f(nullptr); }`,
      to: `Value as_action(const EditBotActionFunc& f) { return Value(); }`,
    },
    {
      note: "set_actions 的键写死",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `    act->set(to_string(field_of(a, u"action_id")), a);`,
      to: `    act->set(u"x", a);`,
    },
    {
      note: "set_frames 的键用 value 而不是 id",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `BotMaker& BotMaker::set_frames(const Value& frame_ids, const Value& action_ids) {
  frames().set(to_string(frame_ids), action_ids);
  return *this;
}`,
      to: `BotMaker& BotMaker::set_frames(const Value& frame_ids, const Value& action_ids) {
  frames().set(to_string(action_ids), frame_ids);
  return *this;
}`,
    },
    {
      note: "set_states 写成 frames",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `BotMaker& BotMaker::set_states(const Value& state_ids, const Value& action_ids) {
  states().set(to_string(state_ids), action_ids);`,
      to: `BotMaker& BotMaker::set_states(const Value& state_ids, const Value& action_ids) {
  frames().set(to_string(state_ids), action_ids);`,
    },
    {
      note: "set_dataset 的键名写错",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `  b->set(u"dataset", spread_assign(field_of(_bot, u"dataset"), dataset));`,
      to: `  b->set(u"dataset2", spread_assign(field_of(_bot, u"dataset"), dataset));`,
    },
    {
      note: "frames() 取值器建的是 states",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `Object& BotMaker::frames() { return child_object(_bot, u"frames"); }`,
      to: `Object& BotMaker::frames() { return child_object(_bot, u"states"); }`,
    },
    {
      note: "states() 取值器建的是 frames",
      file: "native/lfw/dat_translator/bots/bot_maker.cpp",
      from: `Object& BotMaker::states() { return child_object(_bot, u"states"); }`,
      to: `Object& BotMaker::states() { return child_object(_bot, u"frames"); }`,
    },

    {
      note: "bat 的 d>a 最少 mp 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_ball_dfa(num(25), Value(), num(80))),
      as_action(bot_ball_dfj(num(50), Value(), num(0), num(120))),`,
      to: `      as_action(bot_ball_dfa(num(30), Value(), num(80))),
      as_action(bot_ball_dfj(num(50), Value(), num(0), num(120))),`,
    },
    {
      note: "bat 的 d>j 最大 x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_ball_dfj(num(50), Value(), num(0), num(120))),`,
      to: `      as_action(bot_ball_dfj(num(50), Value(), num(0), num(100))),`,
    },
    {
      note: "bat 的 d^j 动作 id 被默认值覆盖",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_chasing_skill_action(u"d^j", Value(), num(200), num(0.05))),`,
      to: `      as_action(bot_chasing_skill_action(u"dva", Value(), num(200), num(0.05))),`,
    },
    {
      note: "bat 的 dva 动作 id 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_chasing_skill_action(u"dva", Value())),`,
      to: `      as_action(bot_chasing_skill_action(u"dvaj", Value())),`,
    },
    {
      note: "bat 的 states 用错状态",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_states(arr({num(static_cast<double>(StateEnum::Catching))}), arr({sv(u"dva")}));`,
      to: `  m.set_states(arr({num(static_cast<double>(StateEnum::Rowing))}), arr({sv(u"dva")}));`,
    },
    {
      note: "bat 的 states 动作写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_states(arr({num(static_cast<double>(StateEnum::Catching))}), arr({sv(u"dva")}));`,
      to: `  m.set_states(arr({num(static_cast<double>(StateEnum::Catching))}), arr({sv(u"d^a")}));`,
    },
    {
      note: "bat 的 frames 少一组 runnings",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings"),
                       frames_field(u"runnings")}),
               arr({sv(u"d^j"), sv(u"d>j"), sv(u"d>a")}));`,
      to: `  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d^j"), sv(u"d>j"), sv(u"d>a")}));`,
    },
    {
      note: "bat 的 frames 少一个动作 id",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `               arr({sv(u"d^j"), sv(u"d>j"), sv(u"d>a")}));`,
      to: `               arr({sv(u"d^j"), sv(u"d>j")}));`,
    },
    {
      note: "frames_field 忽略传入的名字",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `Value frames_field(const char16_t* name) { return field_of(frames_object(), name); }`,
      to: `Value frames_field(const char16_t* name) { return field_of(frames_object(), u"standings"); }`,
    },
    {
      note: "hunter 的 w_atk_x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_dataset(obj({{u"w_atk_m_x", num(79)},
                     {u"w_atk_r_x", num(200)},
                     {u"w_atk_x", num(200)},
                     {u"j_atk_x", num(200)}}));`,
      to: `  m.set_dataset(obj({{u"w_atk_m_x", num(79)},
                     {u"w_atk_r_x", num(200)},
                     {u"w_atk_x", num(201)},
                     {u"j_atk_x", num(200)}}));`,
    },
    {
      note: "hunter 的 dataset 键序互换",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_dataset(obj({{u"w_atk_m_x", num(79)},
                     {u"w_atk_r_x", num(200)},`,
      to: `  m.set_dataset(obj({{u"w_atk_r_x", num(79)},
                     {u"w_atk_m_x", num(200)},`,
    },
    {
      note: "hunter 的 oid 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `BotMaker make_bot_data_hunter() {
  BotMaker m(oid::kHunter);`,
      to: `BotMaker make_bot_data_hunter() {
  BotMaker m(oid::kKnight);`,
    },
    {
      note: "jan 的 z_len 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_explosion_dua(num(150), Value(), num(50), num(400), num(160000))),`,
      to: `      as_action(bot_explosion_dua(num(150), Value(), num(50), num(400), num(16000))),`,
    },
    {
      note: "jan 的 dua 换成 duj",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_explosion_dua(num(150), Value(), num(50), num(400), num(160000))),`,
      to: `      as_action(bot_explosion_duj(num(150), Value(), num(50), num(400), num(160000))),`,
    },
    {
      note: "jan 的 frames 动作互换",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d^a"), sv(u"d^j")}));`,
      to: `  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d^j"), sv(u"d^a")}));`,
    },
    {
      note: "knight 的 d_atk_max_x 键名写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `                     {u"d_atk_max_x", num(200)},
                     {u"r_atk_x", num(150)}}));`,
      to: `                     {u"d_atk_x", num(200)},
                     {u"r_atk_x", num(150)}}));`,
    },
    {
      note: "knight 的 r_atk_x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `                     {u"r_atk_x", num(150)}}));`,
      to: `                     {u"r_atk_x", num(151)}}));`,
    },
    {
      note: "monk 的 desire 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_ball_dfa(num(100), num(0.5), num(0), num(400))(edit_monk_keys())),`,
      to: `      as_action(bot_ball_dfa(num(100), num(0.6), num(0), num(400))(edit_monk_keys())),`,
    },
    {
      note: "monk 的 edit 不再改写按键",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"keys", arr({sv(gk::kDefend), sv(u"F"), sv(gk::kAttack)}));
    }
    return action;`,
      to: `    Object* o = as_object(action);
    if (o != nullptr) {
      o->set(u"keys", arr({sv(u"F"), sv(gk::kDefend), sv(gk::kAttack)}));
    }
    return action;`,
    },
    {
      note: "monk 的 edit 按键用错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      o->set(u"keys", arr({sv(gk::kDefend), sv(u"F"), sv(gk::kAttack)}));`,
      to: `      o->set(u"keys", arr({sv(gk::kU), sv(u"F"), sv(gk::kAttack)}));`,
    },
    {
      note: "monk 的第三个 set_frames 用了 punchs",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(frames_field(u"punchs"), arr({sv(u"d>a")}));
  m.set_frames(range_array(240.0, 248.0), arr({sv(u"d>a+d>a")}));`,
      to: `  m.set_frames(range_array(240.0, 248.0), arr({sv(u"d>a")}));
  m.set_frames(frames_field(u"punchs"), arr({sv(u"d>a+d>a")}));`,
    },
    {
      note: "monk 的 range 上限写小",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(range_array(240.0, 248.0), arr({sv(u"d>a+d>a")}));`,
      to: `  m.set_frames(range_array(240.0, 247.0), arr({sv(u"d>a+d>a")}));`,
    },
    {
      note: "monk 的 frames 动作写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>a")}));
  m.set_frames(frames_field(u"punchs"), arr({sv(u"d>a")}));`,
      to: `  m.set_frames(concat({frames_field(u"standings"), frames_field(u"walkings")}),
               arr({sv(u"d>a+d>a")}));
  m.set_frames(frames_field(u"punchs"), arr({sv(u"d>a")}));`,
    },
    {
      note: "range_array 拒绝生成",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  const std::optional<std::vector<double>> r = range(from, to);
  if (r.has_value()) {
    for (double d : *r) out.push_back(Value(d));
  }`,
      to: `  const std::optional<std::vector<double>> r = range(from, to);
  if (!r.has_value()) {
    for (double d : *r) out.push_back(Value(d));
  }`,
    },
    {
      note: "concat 只取第一组",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  std::vector<Value> out;
  for (const Value& a : arrays) {
    const Array* src = as_array(a);
    if (src == nullptr) continue;
    for (size_t i = 0; i < src->size(); ++i) out.push_back(src->at(i));
  }
  return arr(std::move(out));`,
      to: `  std::vector<Value> out;
  for (const Value& a : arrays) {
    const Array* src = as_array(a);
    if (src == nullptr) continue;
    for (size_t i = 0; i < src->size(); ++i) out.push_back(src->at(i));
    break;
  }
  return arr(std::move(out));`,
    },
    {
      note: "monk 的 oid 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `BotMaker make_bot_data_monk() {
  BotMaker m(oid::kMonk);`,
      to: `BotMaker make_bot_data_monk() {
  BotMaker m(oid::kJack);`,
    },

    {
      note: "davis 的 d>a 最小 x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_ball_dfa(num(50), Value(), num(50))),
      as_action(bot_ball_continuation(u"d>a+a", num(probability(3.0, 0.8)), num(50))),`,
      to: `      as_action(bot_ball_dfa(num(50), Value(), num(60))),
      as_action(bot_ball_continuation(u"d>a+a", num(probability(3.0, 0.8)), num(50))),`,
    },
    {
      note: "davis 的 d>a+a 概率写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_ball_continuation(u"d>a+a", num(probability(3.0, 0.8)), num(50))),`,
      to: `      as_action(bot_ball_continuation(u"d>a+a", num(probability(3.0, 0.9)), num(50))),`,
    },
    {
      note: "davis 的 d^a mp 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_uppercut_dua(num(225), num(DESIRE_RATIO_X_4), num(kUppercutDuaMinX),
                                num(kUppercutDuaMaxX))),`,
      to: `      as_action(bot_uppercut_dua(num(226), num(DESIRE_RATIO_X_4), num(kUppercutDuaMinX),
                                num(kUppercutDuaMaxX))),`,
    },
    {
      note: "davis 的 dva 欲望值用错常量",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_uppercut_dva(num(75), num(DESIRE_RATIO_X_4), num(kUppercutDvaMinX),
                                 num(kUppercutDvaMaxX))),`,
      to: `      as_action(bot_uppercut_dva(num(75), num(DESIRE_RATIO_X_3), num(kUppercutDvaMinX),
                                 num(kUppercutDvaMaxX))),`,
    },
    {
      note: "davis 的 dva+j 概率写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_uppercut_dva(num(25), num(probability(5.0, 0.5)), num(kUppercutDvaMinX),
                                 num(kUppercutDvaMaxX))(edit_davis_dva_j())),`,
      to: `      as_action(bot_uppercut_dva(num(25), num(probability(5.0, 0.6)), num(kUppercutDvaMinX),
                                 num(kUppercutDvaMaxX))(edit_davis_dva_j())),`,
    },
    {
      note: "davis 的 d^j 最大 x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `                                 num(kUppercutDvaMinX + kUppercutDvaMaxX))(edit_davis_dj())),`,
      to: `                                 num(kUppercutDvaMaxX))(edit_davis_dj())),`,
    },
    {
      note: "davis 的 d^j+a 最大 x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_uppercut_dva(num(0), num(1.0), num(kUppercutDvaMinX), num(40))(edit_davis_dj_a())),`,
      to: `      as_action(bot_uppercut_dva(num(0), num(1.0), num(kUppercutDvaMinX), num(41))(edit_davis_dj_a())),`,
    },
    {
      note: "davis 的 dva+run 概率写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `                                   num(probability(7.0, 0.5)))),`,
      to: `                                   num(probability(7.0, 0.6)))),`,
    },
    {
      note: "davis dva+j 的动作 id 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      o->set(u"action_id", sv(u"dva+j"));
      o->set(u"keys", arr({sv(gk::kj)}));`,
      to: `      o->set(u"action_id", sv(u"dva+j2"));
      o->set(u"keys", arr({sv(gk::kj)}));`,
    },
    {
      note: "davis dva+j 的按键写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      o->set(u"keys", arr({sv(gk::kj)}));
      cond.and_(sv(bot_val::kEnemyY), u">", Value(0.0));`,
      to: `      o->set(u"keys", arr({sv(gk::ka)}));
      cond.and_(sv(bot_val::kEnemyY), u">", Value(0.0));`,
    },
    {
      note: "davis dva+j 的条件比较符反了",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      cond.and_(sv(bot_val::kEnemyY), u">", Value(0.0));`,
      to: `      cond.and_(sv(bot_val::kEnemyY), u"<", Value(0.0));`,
    },
    {
      note: "davis d^j 的按键写成 a",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      o->set(u"action_id", sv(u"d^j"));
      o->set(u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::kj)}));`,
      to: `      o->set(u"action_id", sv(u"d^j"));
      o->set(u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)}));`,
    },
    {
      note: "davis d^j+a 的动作 id 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      o->set(u"action_id", sv(u"d^j+a"));`,
      to: `      o->set(u"action_id", sv(u"d^j+b"));`,
    },
    {
      note: "davis 的 87 帧号写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(arr({sv(u"87")}), arr({sv(u"dont_stop_run_attack")}));`,
      to: `  m.set_frames(arr({sv(u"88")}), arr({sv(u"dont_stop_run_attack")}));`,
    },
    {
      note: "davis 的 range(240,269) 上限写小",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(range_array(240.0, 269.0), arr({sv(u"d>a+a")}));`,
      to: `  m.set_frames(range_array(240.0, 268.0), arr({sv(u"d>a+a")}));`,
    },
    {
      note: "davis 的 282 帧号写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(arr({num(282)}), arr({sv(u"dva+run")}));`,
      to: `  m.set_frames(arr({num(283)}), arr({sv(u"dva+run")}));`,
    },
    {
      note: "davis 的后两次 set_frames 顺序互换",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(range_array(270.0, 289.0), arr({sv(u"dva+j"), sv(u"d^a")}));
  m.set_frames(arr({num(39)}), arr({sv(u"dva+j"), sv(u"d^a")}));`,
      to: `  m.set_frames(arr({num(39)}), arr({sv(u"dva+j"), sv(u"d^a")}));
  m.set_frames(range_array(270.0, 289.0), arr({sv(u"dva+j"), sv(u"d^a")}));`,
    },
    {
      note: "davis 的手写动作键序变了",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `                     {u"keys", arr(std::vector<Value>())},
                     {u"desire", Value(defines::num(u"Defines.MAX_AI_DESIRE"))}})),`,
      to: `                     {u"desire", Value(defines::num(u"Defines.MAX_AI_DESIRE"))},
                     {u"keys", arr(std::vector<Value>())}})),`,
    },
    {
      note: "jack 的 range 上限写小",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(range_array(240.0, 247.0), arr({sv(u"d>a+d>a")}));`,
      to: `  m.set_frames(range_array(240.0, 246.0), arr({sv(u"d>a+d>a")}));`,
    },
    {
      note: "jack 的 states 顺序互换",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_states(arr({num(static_cast<double>(StateEnum::Rowing)),
                    num(static_cast<double>(StateEnum::Catching))}),
               arr({sv(u"d^a")}));`,
      to: `  m.set_states(arr({num(static_cast<double>(StateEnum::Catching)),
                    num(static_cast<double>(StateEnum::Rowing))}),
               arr({sv(u"d^a")}));`,
    },
    {
      note: "jack 的 1/15 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_uppercut_dua(num(225), num(1.0 / 15.0), num(kUppercutDuaMinX),
                                 num(kUppercutDuaMaxX))),`,
      to: `      as_action(bot_uppercut_dua(num(225), num(1.0 / 16.0), num(kUppercutDuaMinX),
                                 num(kUppercutDuaMaxX))),`,
    },
    {
      note: "jack 的 d>a+d>a 按键写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `                                      arr({sv(gk::kd), sv(u"F"), sv(gk::ka)}))),`,
      to: `                                      arr({sv(gk::kd), sv(u"F"), sv(gk::kj)}))),`,
    },
    {
      note: "justin 的 front_test max_x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `                               Value(), num(-10), num(100))),`,
      to: `                               Value(), num(-10), num(101))),`,
    },
    {
      note: "justin 的 front_test min_x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `                               Value(), num(-10), num(100))),`,
      to: `                               Value(), num(-20), num(100))),`,
    },
    {
      note: "justin 的 range 上限写小",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(range_array(240.0, 246.0), arr({sv(u"d>a+a")}));`,
      to: `  m.set_frames(range_array(240.0, 245.0), arr({sv(u"d>a+a")}));`,
    },
    {
      note: "justin 的 frames 动作顺序互换",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `               arr({sv(u"dva"), sv(u"d>a")}));`,
      to: `               arr({sv(u"d>a"), sv(u"dva")}));`,
    },
    {
      note: "louis 的 d>a 最大 x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_ball_dfa(num(150), Value(), num(120), num(800))),`,
      to: `      as_action(bot_ball_dfa(num(150), Value(), num(120), num(801))),`,
    },
    {
      note: "louis 的 d^j 最大 x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_uppercut_duj(num(100), Value(), num(-10), num(120))),`,
      to: `      as_action(bot_uppercut_duj(num(100), Value(), num(-10), num(121))),`,
    },
    {
      note: "louis dja 的血量阈值写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    cond.and_(sv(entity_val::kHP_P), u"<", Value(33.0));`,
      to: `    cond.and_(sv(entity_val::kHP_P), u"<", Value(34.0));`,
    },
    {
      note: "louis 的 frames 动作顺序互换",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `               arr({sv(u"d^j"), sv(u"d>a"), sv(u"d>j"), sv(u"dja")}));`,
      to: `               arr({sv(u"d>j"), sv(u"d>a"), sv(u"d^j"), sv(u"dja")}));`,
    },
    {
      note: "mark 的 d>a max_d 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    set_rays_max_d(action, 1600.0);
    return action;`,
      to: `    set_rays_max_d(action, 1500.0);
    return action;`,
    },
    {
      note: "mark 的 d>j max_d 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    set_rays_max_d(action, 1600.0);
    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));`,
      to: `    set_rays_max_d(action, 1601.0);
    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));`,
    },
    {
      note: "mark 的 d>j 条件左值写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));`,
      to: `    cond.and_(sv(bot_val::kEnemyDiffX), u"!=", Value(1.0));`,
    },
    {
      note: "mark 的 d>j 比较符写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    cond.and_(sv(bot_val::kEnemyOutOfRange), u"!=", Value(1.0));`,
      to: `    cond.and_(sv(bot_val::kEnemyOutOfRange), u"==", Value(1.0));`,
    },
    {
      note: "mark 的 cancel_d>j 第二个 ray 的 z 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      rays->push_back(ray_with_z(first, 0.2));
      rays->push_back(ray_with_z(first, -0.2));`,
      to: `      rays->push_back(ray_with_z(first, 0.2));
      rays->push_back(ray_with_z(first, -0.3));`,
    },
    {
      note: "mark 的 cancel_d>j 第一个 ray 的 z 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      rays->push_back(ray_with_z(first, 0.2));
      rays->push_back(ray_with_z(first, -0.2));`,
      to: `      rays->push_back(ray_with_z(first, 0.3));
      rays->push_back(ray_with_z(first, -0.2));`,
    },
    {
      note: "mark 的 cancel_d>j 条件阈值写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    cond.or_(sv(bot_val::kEnemyDiffX), u"<", Value(-100.0));`,
      to: `    cond.or_(sv(bot_val::kEnemyDiffX), u"<", Value(-101.0));`,
    },
    {
      note: "mark 的 cancel_d>j 条件比较符写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    cond.or_(sv(bot_val::kEnemyDiffX), u"<", Value(-100.0));`,
      to: `    cond.or_(sv(bot_val::kEnemyDiffX), u">", Value(-100.0));`,
    },
    {
      note: "mark 的 cancel_d>j 按键写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    o->set(u"keys", arr({sv(gk::kJump)}));`,
      to: `    o->set(u"keys", arr({sv(gk::ka)}));`,
    },
    {
      note: "mark 的 cancel_d>j 不给原 ray 加 reverse",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      if (ray != nullptr) ray->set(u"reverse", Value(true));`,
      to: `      if (ray != nullptr) ray->set(u"reverse", Value(false));`,
    },
    {
      note: "mark 的 range(85,89) 上限写小",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  m.set_frames(range_array(85.0, 89.0), arr({sv(u"d>a+a")}));`,
      to: `  m.set_frames(range_array(85.0, 88.0), arr({sv(u"d>a+a")}));`,
    },
    {
      note: "ray_with_z 不再覆盖 z",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    out.set(k, k == u"z" ? Value(z) : *p);`,
      to: `    out.set(k, *p);`,
    },
    {
      note: "set_rays_max_d 的键名写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `    if (r != nullptr) r->set(u"max_d", Value(v));`,
      to: `    if (r != nullptr) r->set(u"max_dd", Value(v));`,
    },
    {
      note: "sorcerer 的 idle mp 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_idle_action(u"dvj", arr({sv(gk::kDefend), sv(gk::kDown), sv(gk::kJump)}),
                                num(350))(edit_sorcerer_status())),`,
      to: `      as_action(bot_idle_action(u"dvj", arr({sv(gk::kDefend), sv(gk::kDown), sv(gk::kJump)}),
                                num(351))(edit_sorcerer_status())),`,
    },
    {
      note: "sorcerer 的 status 顺序写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      o->set(u"status", arr({sv(bot_state_enum::kIdle), sv(bot_state_enum::kChasing),
                              sv(bot_state_enum::kAvoiding)}));`,
      to: `      o->set(u"status", arr({sv(bot_state_enum::kIdle), sv(bot_state_enum::kAvoiding),
                              sv(bot_state_enum::kChasing)}));`,
    },
    {
      note: "sorcerer 的可恢复血量阈值写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      cond.and_(sv(entity_val::kHpRecoverable), u">=", Value(50.0));`,
      to: `      cond.and_(sv(entity_val::kHpRecoverable), u">=", Value(51.0));`,
    },
    {
      note: "sorcerer 的第二个条件左值写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      cond.and_(sv(bot_val::kSafe), u"==", Value(1.0));`,
      to: `      cond.and_(sv(bot_val::kEnemyX), u"==", Value(1.0));`,
    },
    {
      note: "sorcerer 的 frames 动作顺序互换",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `               arr({sv(u"d>a"), sv(u"d>j"), sv(u"d^j"), sv(u"dvj")}));`,
      to: `               arr({sv(u"d>a"), sv(u"d>j"), sv(u"dvj"), sv(u"d^j")}));`,
    },
    {
      note: "sorcerer 的 d>j 最大 x 写错",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `      as_action(bot_ball_dfj(num(125), Value(), num(100), num(10000))),`,
      to: `      as_action(bot_ball_dfj(num(125), Value(), num(100), num(9999))),`,
    },
    {
      note: "注册表里 davis 写成 mark",
      file: "native/lfw/dat_translator/bots/make_bot_data.cpp",
      from: `  BotMaker::register_maker(oid::kDavis, make_bot_data_davis);`,
      to: `  BotMaker::register_maker(oid::kMark, make_bot_data_davis);`,
    },
  ],
};
