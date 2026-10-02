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
  ],
};
