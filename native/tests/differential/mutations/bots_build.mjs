export default {
  subject: "bots_build",
  mutations: [
    {
      note: "frames.walkings 的上限写大",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"walkings", make_array(prefixed_series(u"walking_", 0.0, 5.0)));`,
      to: `  o.set(u"walkings", make_array(prefixed_series(u"walking_", 0.0, 6.0)));`,
    },
    {
      note: "frames.walkings 的前缀少了下划线",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"walkings", make_array(prefixed_series(u"walking_", 0.0, 5.0)));`,
      to: `  o.set(u"walkings", make_array(prefixed_series(u"walking", 0.0, 5.0)));`,
    },
    {
      note: "frames.runnings 的前缀少了下划线",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"runnings", make_array(prefixed_series(u"running_", 0.0, 3.0)));`,
      to: `  o.set(u"runnings", make_array(prefixed_series(u"running", 0.0, 3.0)));`,
    },
    {
      note: "frames.standings 多了前缀",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"standings", make_array(prefixed_series(u"", 0.0, 3.0)));`,
      to: `  o.set(u"standings", make_array(prefixed_series(u"s", 0.0, 3.0)));`,
    },
    {
      note: "frames.punchs 少一个",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"punchs", make_array(number_series(60.0, 69.0)));`,
      to: `  o.set(u"punchs", make_array(number_series(60.0, 68.0)));`,
    },
    {
      note: "frames.rowings 少一个",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"rowings", make_array(number_series(103.0, 107.0)));`,
      to: `  o.set(u"rowings", make_array(number_series(103.0, 106.0)));`,
    },
    {
      note: "frames.super_punch 少一个",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"super_punch", make_array(number_series(70.0, 79.0)));`,
      to: `  o.set(u"super_punch", make_array(number_series(70.0, 78.0)));`,
    },
    {
      note: "frames.defends 少一个",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"defends", make_array({text(u"110"), text(u"111")}));`,
      to: `  o.set(u"defends", make_array({text(u"110")}));`,
    },
    {
      note: "frames 的 punchs/rowings 插入序互换",
      file: "native/lfw/dat_translator/bots/frames.cpp",
      from: `  o.set(u"punchs", make_array(number_series(60.0, 69.0)));
  o.set(u"rowings", make_array(number_series(103.0, 107.0)));`,
      to: `  o.set(u"rowings", make_array(number_series(103.0, 107.0)));
  o.set(u"punchs", make_array(number_series(60.0, 69.0)));`,
    },
    {
      note: "bot_ball_dfa 的 desire 默认值用错常量",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `  return bot_front_test(kBallDfaId, arr({sv(gk::kd), sv(u"F"), sv(gk::ka)}), min_mp,
                        Value(num_or(desire, DESIRE_RATIO_X_2)), Value(num_or(min_x, 120.0)), max_x,`,
      to: `  return bot_front_test(kBallDfaId, arr({sv(gk::kd), sv(u"F"), sv(gk::ka)}), min_mp,
                        Value(num_or(desire, DESIRE_RATIO)), Value(num_or(min_x, 120.0)), max_x,`,
    },
    {
      note: "bot_ball_dfj 默认按键写成 d>a 的 a",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `  return bot_front_test(kBallDfjId, arr({sv(gk::kd), sv(u"F"), sv(gk::kj)}), min_mp,`,
      to: `  return bot_front_test(kBallDfjId, arr({sv(gk::kd), sv(u"F"), sv(gk::ka)}), min_mp,`,
    },
    {
      note: "bot_ball_cancelling 的兜底按键换成 a",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `                           {u"keys", keys_or(keys, {gk::kj})}});
    return apply_edit(fn, ret, cond);`,
      to: `                           {u"keys", keys_or(keys, {gk::ka})}});
    return apply_edit(fn, ret, cond);`,
    },
    {
      note: "bot_ball_continuation 的兜底按键换成 j",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `                           {u"keys", keys_or(keys, {gk::kAttack})}});`,
      to: `                           {u"keys", keys_or(keys, {gk::kj})}});`,
    },
    {
      note: "bot_ball_cancelling 的 e_ray 不带 reverse",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `                           {u"e_ray", arr({obj({{u"x", Value(1.0)},
                                                 {u"z", Value(0.0)},
                                                 {u"reverse", Value(true)}})})},`,
      to: `                           {u"e_ray", arr({obj({{u"x", Value(1.0)},
                                                 {u"z", Value(0.0)},
                                                 {u"reverse", Value(false)}})})},`,
    },
    {
      note: "bot_ball_continuation 的 e_ray 少了 z",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `                           {u"e_ray", arr({obj({{u"x", Value(1.0)}, {u"z", Value(0.0)}})})},`,
      to: `                           {u"e_ray", arr({obj({{u"x", Value(1.0)}})})},`,
    },
    {
      note: "bot_ball_cancelling 的 status 换成 Idle",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    CondMaker cond;
    const Value ret = obj({{u"action_id", Value(action_id)},
                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kChasing)})},
                           {u"e_ray", arr({obj({{u"x", Value(1.0)},`,
      to: `    CondMaker cond;
    const Value ret = obj({{u"action_id", Value(action_id)},
                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kIdle)})},
                           {u"e_ray", arr({obj({{u"x", Value(1.0)},`,
    },
    {
      note: "bot_idle_action 的 status 换成 Chasing",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `                           {u"status", arr({sv(bot_state_enum::kIdle)})},`,
      to: `                           {u"status", arr({sv(bot_state_enum::kChasing)})},`,
    },
    {
      note: "desire 不再过 Defines.desire",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `                           {u"desire", Value(defines::desire(num_or(desire, DESIRE_RATIO)))},
                           {u"status", arr({sv(bot_state_enum::kIdle)})},`,
      to: `                           {u"desire", Value(num_or(desire, DESIRE_RATIO))},
                           {u"status", arr({sv(bot_state_enum::kIdle)})},`,
    },
    {
      note: "bot_ball_continuation 的 mp 判定用 >=",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    const double mp_n = num_or(mp, 0.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", mp);`,
      to: `    const double mp_n = num_or(mp, 0.0);
    if (mp_n >= 0) cond.add(sv(entity_val::kMP), u">=", mp);`,
    },
    {
      note: "bot_idle_action 的 mp 判定用 >=",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `EditBotActionFunc bot_idle_action(const std::u16string& action_id, const Value& keys,
                                  const Value& min_mp, const Value& desire) {
  return [action_id, keys, min_mp, desire](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, -1.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", min_mp);`,
      to: `EditBotActionFunc bot_idle_action(const std::u16string& action_id, const Value& keys,
                                  const Value& min_mp, const Value& desire) {
  return [action_id, keys, min_mp, desire](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, -1.0);
    if (mp_n >= 0) cond.add(sv(entity_val::kMP), u">=", min_mp);`,
    },
    {
      note: "bot_front_test 的 mp 判定用 >=",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    CondMaker cond;
    const double mp_n = num_or(min_mp, 0.0);
    if (mp_n > 0) cond.add(sv(entity_val::kMP), u">=", min_mp);
    const Value ray_1 = ray_with(num_or(min_x, 0.0), max_x, Value(), false);`,
      to: `    CondMaker cond;
    const double mp_n = num_or(min_mp, 0.0);
    if (mp_n >= 0) cond.add(sv(entity_val::kMP), u">=", min_mp);
    const Value ray_1 = ray_with(num_or(min_x, 0.0), max_x, Value(), false);`,
    },
    {
      note: "bot_chasing_action 的 min_mp 默认值换成 1",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `EditBotActionFunc bot_chasing_action(const std::u16string& action_id, const Value& keys,
                                     const Value& min_mp, const Value& desire) {
  return [action_id, keys, min_mp, desire](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, -1.0);`,
      to: `EditBotActionFunc bot_chasing_action(const std::u16string& action_id, const Value& keys,
                                     const Value& min_mp, const Value& desire) {
  return [action_id, keys, min_mp, desire](const EditBotAction* fn) -> Value {
    CondMaker cond;
    const double mp_n = num_or(min_mp, 1.0);`,
    },
    {
      note: "bot_front_test 的 min_x 默认值换成 1",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    const Value ray_1 = ray_with(num_or(min_x, 0.0), max_x, Value(), false);`,
      to: `    const Value ray_1 = ray_with(num_or(min_x, 1.0), max_x, Value(), false);`,
    },
    {
      note: "ray 的 min_x/max_x 键序互换",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    return obj({{u"x", Value(1.0)}, {u"z", Value(0.0)}, {u"min_x", Value(min_x)}, {u"max_x", max_x}});`,
      to: `    return obj({{u"x", Value(1.0)}, {u"z", Value(0.0)}, {u"max_x", max_x}, {u"min_x", Value(min_x)}});`,
    },
    {
      note: "带 max_d 的 ray 键序互换",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `              {u"min_x", Value(min_x)},
              {u"max_x", max_x},
              {u"max_d", max_d}});`,
      to: `              {u"min_x", Value(min_x)},
              {u"max_d", max_d},
              {u"max_x", max_x}});`,
    },
    {
      note: "zable 的负向 ray 不生效",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `      rays.push_back(override_z(ray_1, Value(-z)));`,
      to: `      rays.push_back(override_z(ray_1, Value(z)));`,
    },
    {
      note: "zable 为 0 也加 ray",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    if (truthy(zable) && z > 0) {`,
      to: `    if (truthy(zable) && z >= 0) {`,
    },
    {
      note: "zable 的 truthy 判定被去掉",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    if (truthy(zable) && z > 0) {`,
      to: `    if (z >= 0) {`,
    },
    {
      note: "zable 扩展时第二个 ray 用取负后的数字",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `      rays.push_back(override_z(ray_1, zable));`,
      to: `      rays.push_back(override_z(ray_1, Value(z)));`,
    },
    {
      note: "bot_chasing_skill_action 的 v 键换成 U",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    case u'v': keys.push_back(sv(gk::kD)); break;`,
      to: `    case u'v': keys.push_back(sv(gk::kU)); break;`,
    },
    {
      note: "bot_chasing_skill_action 的 > 键换成 B",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    case u'>': keys.push_back(sv(u"F")); break;`,
      to: `    case u'>': keys.push_back(sv(u"B")); break;`,
    },
    {
      note: "bot_chasing_skill_action 的 a 键换成 j",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    case u'a': keys.push_back(sv(gk::ka)); break;`,
      to: `    case u'a': keys.push_back(sv(gk::kj)); break;`,
    },
    {
      note: "bot_chasing_skill_action 的首键换成 F",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `  std::vector<Value> keys;
  keys.push_back(sv(gk::kd));`,
      to: `  std::vector<Value> keys;
  keys.push_back(sv(u"F"));`,
    },
    {
      note: "bot_chasing_skill_action 的 action_id 默认值失效",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `  const std::u16string aid =
      std::holds_alternative<std::monostate>(action_id) ? keys_str : to_string(action_id);`,
      to: `  const std::u16string aid = keys_str;`,
    },
    {
      note: "bot_explosion_dua 的 min_x 默认值写错",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `         {u"e_ray", arr({ray_with(num_or(min_x, kExplosionDuaMinX),
                                    Value(num_or(max_x, kExplosionDuaMaxX)), Value(d), true)})},
         {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});`,
      to: `         {u"e_ray", arr({ray_with(num_or(min_x, -100.0),
                                    Value(num_or(max_x, kExplosionDuaMaxX)), Value(d), true)})},
         {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});`,
    },
    {
      note: "bot_explosion_dua 的 max_x 默认值写错",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `         {u"e_ray", arr({ray_with(num_or(min_x, kExplosionDuaMinX),
                                    Value(num_or(max_x, kExplosionDuaMaxX)), Value(d), true)})},
         {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});`,
      to: `         {u"e_ray", arr({ray_with(num_or(min_x, kExplosionDuaMinX),
                                    Value(num_or(max_x, 100.0)), Value(d), true)})},
         {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});`,
    },
    {
      note: "bot_explosion_dua 的 z_len 默认值写错",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    const double d = lfw::pow(num_or(z_len, kExplosionDuaZLen), 2.0);
    const Value ret = obj(
        {{u"action_id", sv(kExplosionDuaId)},`,
      to: `    const double d = lfw::pow(num_or(z_len, 100.0), 2.0);
    const Value ret = obj(
        {{u"action_id", sv(kExplosionDuaId)},`,
    },
    {
      note: "bot_explosion_dua 的次方写成 3",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `    const double d = lfw::pow(num_or(z_len, kExplosionDuaZLen), 2.0);
    const Value ret = obj(
        {{u"action_id", sv(kExplosionDuaId)},`,
      to: `    const double d = lfw::pow(num_or(z_len, kExplosionDuaZLen), 3.0);
    const Value ret = obj(
        {{u"action_id", sv(kExplosionDuaId)},`,
    },
    {
      note: "bot_explosion_duj 的按键写成 dua 的 a",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::kj)})}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_idle_action`,
      to: `         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});
    return apply_edit(fn, ret, cond);
  };
}

EditBotActionFunc bot_idle_action`,
    },
    {
      note: "bot_uppercut_dua 的 max_d 默认值写错",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `  const double d = num_or(max_d, 30.0);
  return obj({{u"action_id", sv(kUppercutDuaId)},`,
      to: `  const double d = num_or(max_d, 40.0);
  return obj({{u"action_id", sv(kUppercutDuaId)},`,
    },
    {
      note: "bot_uppercut_dua 的 max_d 不平铺",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `              {u"e_ray", arr({ray_with(num_or(min_x, kUppercutDuaMinX),
                                         Value(num_or(max_x, kUppercutDuaMaxX)),
                                         Value(d * d), true)})},
              {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
              {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});`,
      to: `              {u"e_ray", arr({ray_with(num_or(min_x, kUppercutDuaMinX),
                                         Value(num_or(max_x, kUppercutDuaMaxX)),
                                         Value(d), true)})},
              {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
              {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});`,
    },
    {
      note: "bot_uppercut_dua 的按键换成 dva 的 D",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `              {u"e_ray", arr({ray_with(num_or(min_x, kUppercutDuaMinX),
                                         Value(num_or(max_x, kUppercutDuaMaxX)),
                                         Value(d * d), true)})},
              {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
              {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});`,
      to: `              {u"e_ray", arr({ray_with(num_or(min_x, kUppercutDuaMinX),
                                         Value(num_or(max_x, kUppercutDuaMaxX)),
                                         Value(d * d), true)})},
              {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},
              {u"keys", arr({sv(gk::kd), sv(gk::kD), sv(gk::ka)})}});`,
    },
    {
      note: "bot_uppercut_duj 的按键写成 dua 的 a",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `              {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::kj)})}});
}

EditBotActionFunc bot_uppercut_dva`,
      to: `              {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});
}

EditBotActionFunc bot_uppercut_dva`,
    },
    {
      note: "bot_uppercut_dva 的按键换成 dua 的 U",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `         {u"keys", arr({sv(gk::kd), sv(gk::kD), sv(gk::ka)})}});`,
      to: `         {u"keys", arr({sv(gk::kd), sv(gk::kU), sv(gk::ka)})}});`,
    },
    {
      note: "bot_uppercut_dva 的 expression 变成条件式",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `         {u"expression", Value(cond.done())},`,
      to: `         {u"expression", mp_n > 0 ? Value(cond.done()) : Value()},`,
    },
    {
      note: "edit 回调不再被调用",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `Value apply_edit(const EditBotAction* fn, const Value& action, CondMaker& cond) {
  if (fn == nullptr) return action;`,
      to: `Value apply_edit(const EditBotAction* fn, const Value& action, CondMaker& cond) {
  if (fn != nullptr) return action;`,
    },
    {
      note: "bot_uppercut_dva 绕过 edit",
      file: "native/lfw/dat_translator/bots/bot_actions.cpp",
      from: `         {u"keys", arr({sv(gk::kd), sv(gk::kD), sv(gk::ka)})}});
    return apply_edit(fn, ret, cond);`,
      to: `         {u"keys", arr({sv(gk::kd), sv(gk::kD), sv(gk::ka)})}});
    (void)fn;
    return ret;`,
    },
    {
      note: "DESIRE_RATIO_X_3 的取值写错",
      file: "native/lfw/dat_translator/bots/constants.h",
      from: `inline constexpr double DESIRE_RATIO_X_3 = 0.1998;`,
      to: `inline constexpr double DESIRE_RATIO_X_3 = 0.2;`,
    },
    {
      note: "DESIRE_RATIO_X_2 的取值写错",
      file: "native/lfw/dat_translator/bots/constants.h",
      from: `inline constexpr double DESIRE_RATIO_X_2 = 0.1665;`,
      to: `inline constexpr double DESIRE_RATIO_X_2 = 0.17;`,
    },
    {
      note: "DESIRE_RATIO 的取值写错",
      file: "native/lfw/dat_translator/bots/constants.h",
      from: `inline constexpr double DESIRE_RATIO = 0.0666;`,
      to: `inline constexpr double DESIRE_RATIO = 0.066;`,
    },
  ],
};
