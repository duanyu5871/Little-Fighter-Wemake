// Mutation spec for the `entity` differential slice.
//
// Subject: native/lfw/entity/entity.{h,cpp} (the TS side drives the real
// `src/LFW/entity/Entity.ts` class through a stub world / lfw).
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * The TS constructor needs `data.base` to exist (`reset` reads `data.base.*`
//    without a guard): every scene supplies a `base` record, so the port reading it
//    defensively is not a difference the harness can observe.
//  * `reset` also clears `copies` / `vrests` / `blockers` / `superpunchs` / the
//    collision lists and iterates `buffs` calling `del_victim`. Those containers are
//    empty in every scene (the collision / buff wiring slices own them), so mutating
//    their clearing is equivalent by construction and is not listed.
//  * `terrain` / `_atom_time` / `prev_position` / `velocity` / `_motionless_ticks`
//    are written by `reset` but nothing in this slice reads them back, so they are
//    only observable once the physics slices land.
//  * `add_catch_time(0)` and `set_catch_time(_catch_time)` are the same call, so the
//    `!value` early return is equivalent for the values the DSL can express.
//  * Three candidates are withdrawn as equivalent by construction and are noted here
//    instead of being listed: (a) `reset`'s `_toughness_resting_max = …DEFAULT_…`
//    and (c) `reset`'s `_toughness_r_value = dataset(…)` are both overwritten by
//    `reset_armor()` later in the same `reset` (60 → `armor?.toughness_resting ?? 0`),
//    and (b) the equality guards of `set_arest` / `set_catch_time` are unobservable
//    because neither setter notifies (`arest` has no callback, `set_catch_time` only
//    writes the slot).
//  * `state_on_dead` / `state_get_gravity` model `_state?.on_dead?.()` /
//    `_state?.get_gravity?.()`; "no state" and "state without that hook" both mean
//    "don't call", which is unobservable by design.
//  * The mutation runner rebuilds and compares whole traces, so a wrong literal, a
//    dropped notification, an inverted clamp or a swapped `??` all surface as drift.

export default {
  subject: "entity",
  mutations: [
    {
      note: "reset drops the base resting_max",
      file: "native/lfw/entity/entity.cpp",
      from: '  _resting_max = opt_num(field_or(base_of(data_now), u"resting_max"));',
      to: "  _resting_max = std::nullopt;",
    },
    {
      note: "reset drops the base fall_value_max",
      file: "native/lfw/entity/entity.cpp",
      from: '  _fall_value_max = opt_num(field_or(base_of(data_now), u"fall_value_max"));',
      to: "  _fall_value_max = std::nullopt;",
    },
    {
      note: "reset drops the base defend_value_max",
      file: "native/lfw/entity/entity.cpp",
      from: '  _defend_value_max = opt_num(field_or(base_of(data_now), u"defend_value_max"));',
      to: "  _defend_value_max = std::nullopt;",
    },
    {
      note: "reset drops the base defend_ratio (second read)",
      file: "native/lfw/entity/entity.cpp",
      from:
        '  _defend_ratio = opt_num(field_or(base_of(data_now), u"defend_ratio"));\n  jumping.x = 0;',
      to: "  _defend_ratio = std::nullopt;\n  jumping.x = 0;",
    },
    {
      note: "reset drops the base catch_time_max",
      file: "native/lfw/entity/entity.cpp",
      from: '  _catch_time_max = opt_num(field_or(base_of(data_now), u"catch_time_max"));',
      to: "  _catch_time_max = std::nullopt;",
    },
    {
      note: "reset does not snapshot hp_max from the dataset",
      file: "native/lfw/entity/entity.cpp",
      from: '  _hp_max = opt_num(dataset(u"hp_max"));',
      to: "  _hp_max = std::nullopt;",
    },
    {
      note: "reset does not snapshot mp_max from the dataset",
      file: "native/lfw/entity/entity.cpp",
      from: '  _mp_max = opt_num(dataset(u"mp_max"));',
      to: "  _mp_max = std::nullopt;",
    },
    {
      note: "reset drops the dirty-defend recovery value",
      file: "native/lfw/entity/entity.cpp",
      from: '  _defend_r_value = num_of(dataset(u"defend_r_value"));',
      to: "  _defend_r_value = 0;",
    },
    {
      note: "reset drops the fall recovery value",
      file: "native/lfw/entity/entity.cpp",
      from: '  _fall_r_value = num_of(dataset(u"fall_r_value"));',
      to: "  _fall_r_value = 0;",
    },
    {
      note: "reset drops the hp recovery tick range",
      file: "native/lfw/entity/entity.cpp",
      from: '  _hp_r_tick.set_max(num_of(dataset(u"hp_r_ticks")));',
      to: "  (void)dataset(u\"hp_r_ticks\");",
    },
    {
      note: "reset drops the mp recovery tick range",
      file: "native/lfw/entity/entity.cpp",
      from: '  _mp_r_tick.set_max(num_of(dataset(u"mp_r_ticks")));',
      to: "  (void)dataset(u\"mp_r_ticks\");",
    },
    {
      note: "reset drops the fall recovery tick range",
      file: "native/lfw/entity/entity.cpp",
      from: '  _fall_r_tick.set_max(num_of(dataset(u"fall_r_ticks")));',
      to: "  (void)dataset(u\"fall_r_ticks\");",
    },
    {
      note: "reset drops the defend recovery tick range",
      file: "native/lfw/entity/entity.cpp",
      from: '  _defend_r_tick.set_max(num_of(dataset(u"defend_r_ticks")));',
      to: "  (void)dataset(u\"defend_r_ticks\");",
    },
    {
      note: "reset keeps the old id",
      file: "native/lfw/entity/entity.cpp",
      from: "  id = host_->new_id();",
      to: "  (void)host_->new_id();",
    },
    {
      note: "reset faces the other way",
      file: "native/lfw/entity/entity.cpp",
      from: "  facing = 1;",
      to: "  facing = -1;",
    },
    {
      note: "reset ignores the host team",
      file: "native/lfw/entity/entity.cpp",
      from: "  _team = host_->new_team();",
      to: '  _team = u"9";',
    },
    {
      note: "reset keeps the registered callbacks",
      file: "native/lfw/entity/entity.cpp",
      from: "  callbacks.clear();",
      to: "  (void)callbacks;",
    },
    {
      note: "reset keeps the injected state hooks",
      file: "native/lfw/entity/entity.cpp",
      from: "  state_on_dead = nullptr;",
      to: "  (void)state_on_dead;",
    },
    {
      note: "reset keeps a stale reserve",
      file: "native/lfw/entity/entity.cpp",
      from: "  _reserve = 0;",
      to: "  _reserve = 1;",
    },
    {
      note: "reset starts on the ground",
      file: "native/lfw/entity/entity.cpp",
      from: "  is_on_ground = false;",
      to: "  is_on_ground = true;",
    },
    {
      note: "reset does not classify the key role",
      file: "native/lfw/entity/entity.cpp",
      from: "  _render_effect_time = 0;\n  auto_key_role();",
      to: "  _render_effect_time = 0;",
    },
    {
      note: "reset does not fill fall_value from its cap",
      file: "native/lfw/entity/entity.cpp",
      from: "  set_fall_value(fall_value_max());",
      to: "  set_fall_value(0.0);",
    },
    {
      note: "reset does not fill defend_value from its cap",
      file: "native/lfw/entity/entity.cpp",
      from: "  set_defend_value(defend_value_max());",
      to: "  set_defend_value(0.0);",
    },
    {
      note: "reset starts hp_r at zero",
      file: "native/lfw/entity/entity.cpp",
      from: "  _hp_r = hp_max();\n  _hp = _hp_r;",
      to: "  _hp_r = 0;\n  _hp = _hp_r;",
    },
    {
      note: "reset starts mp at zero",
      file: "native/lfw/entity/entity.cpp",
      from: "  _mp = mp_max();",
      to: "  _mp = 0;",
    },
    {
      note: "reset starts the catch time at zero",
      file: "native/lfw/entity/entity.cpp",
      from: "  set_catch_time(catch_time_max());",
      to: "  set_catch_time(0.0);",
    },
    {
      note: "reset uses another outline alpha",
      file: "native/lfw/entity/entity.cpp",
      from: "  _outline_color.clear();\n  _outline_alpha = 0.8;",
      to: "  _outline_color.clear();\n  _outline_alpha = 0.7;",
    },
    {
      note: "reset uses another outline width",
      file: "native/lfw/entity/entity.cpp",
      from: "  _outline_width = 1;",
      to: "  _outline_width = 2;",
    },
    {
      note: "reset leaves the mix strength alone",
      file: "native/lfw/entity/entity.cpp",
      from: "  _mix_strength = 0;",
      to: "  _mix_strength = 1;",
    },
    {
      note: "reset leaves the greyscale alone",
      file: "native/lfw/entity/entity.cpp",
      from: "  _greyscale = 0;",
      to: "  _greyscale = 1;",
    },
    {
      note: "reset keeps a previously forced outline_enabled",
      file: "native/lfw/entity/entity.cpp",
      from: "  _outline_enabled = std::nullopt;",
      to: "  (void)_outline_enabled;",
    },
    {
      note: "reset does not carry the empty frame into prev_frame",
      file: "native/lfw/entity/entity.cpp",
      from: "  _prev_frame = frame;",
      to: "  (void)_prev_frame;",
    },
    {
      note: "reset ignores the base drink record",
      file: "native/lfw/entity/entity.cpp",
      from: "  drink = truthy(drink_info) ? std::make_unique<DrinkInfo>(drink_info) : nullptr;",
      to: "  drink = nullptr;",
    },
    {
      note: "reserve is stored unrounded",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_reserve(double v) {\n  v = round_float(v);",
      to: "void Entity::set_reserve(double v) {",
    },
    {
      note: "reserve notifies even when unchanged",
      file: "native/lfw/entity/entity.cpp",
      from:
        "  const double o = _reserve;\n  if (o == v) return;\n  _reserve = v;\n  callbacks.call(u\"on_reserve_changed\", {ref(), Value(v), Value(o)});",
      to:
        "  const double o = _reserve;\n  _reserve = v;\n  callbacks.call(u\"on_reserve_changed\", {ref(), Value(v), Value(o)});",
    },
    {
      note: "reserve reports the new value as the old one",
      file: "native/lfw/entity/entity.cpp",
      from: '  callbacks.call(u"on_reserve_changed", {ref(), Value(v), Value(o)});',
      to: '  callbacks.call(u"on_reserve_changed", {ref(), Value(o), Value(v)});',
    },
    {
      note: "resting_max ignores its dataset fallback",
      file: "native/lfw/entity/entity.cpp",
      from:
        "double Entity::resting_max() const {\n  return _resting_max.has_value() ? *_resting_max : num_of(host_->world_dataset(u\"resting_max\"));",
      to: "double Entity::resting_max() const {\n  return _resting_max.value_or(0.0);",
    },
    {
      note: "resting_max compares against the stored value instead of the live one",
      file: "native/lfw/entity/entity.cpp",
      from:
        "void Entity::set_resting_max(double v) {\n  v = round_float(v);\n  const double o = resting_max();",
      to:
        "void Entity::set_resting_max(double v) {\n  v = round_float(v);\n  const double o = _resting_max.value_or(0.0);",
    },
    {
      note: "resting is stored unrounded",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_resting(double v) {\n  v = round_float(v);",
      to: "void Entity::set_resting(double v) {",
    },
    {
      note: "resting reports the new value as the old one",
      file: "native/lfw/entity/entity.cpp",
      from: '  callbacks.call(u"on_resting_changed", {ref(), Value(v), Value(o)});',
      to: '  callbacks.call(u"on_resting_changed", {ref(), Value(o), Value(v)});',
    },
    {
      note: "fall_value reports the stored value as the old one",
      file: "native/lfw/entity/entity.cpp",
      from:
        '  _fall_value = round_float(v);\n  if (v < o) {\n    set_resting(resting_max());\n    set_toughness_resting(toughness_resting_max());\n  }',
      to: "  _fall_value = round_float(v);",
    },
    {
      note: "fall_value stores the raw value",
      file: "native/lfw/entity/entity.cpp",
      from: "  _fall_value = round_float(v);",
      to: "  _fall_value = v;",
    },
    {
      note: "a fall_value drop does not restore the toughness",
      file: "native/lfw/entity/entity.cpp",
      from:
        "  if (v < o) {\n    set_resting(resting_max());\n    set_toughness_resting(toughness_resting_max());\n  }\n  callbacks.call(u\"on_fall_value_changed\", {ref(), Value(v), Value(o)});",
      to:
        "  if (v < o) {\n    set_resting(resting_max());\n  }\n  callbacks.call(u\"on_fall_value_changed\", {ref(), Value(v), Value(o)});",
    },
    {
      note: "fall_value compares against the rounded value",
      file: "native/lfw/entity/entity.cpp",
      from:
        "void Entity::set_fall_value(double v) {\n  const double o = _fall_value;\n  if (o == v) return;",
      to:
        "void Entity::set_fall_value(double v) {\n  const double o = _fall_value;\n  if (o == round_float(v)) return;",
    },
    {
      note: "toughness is not clamped at zero",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_toughness(double v) {\n  v = round_float(v);\n  if (v < 0) v = 0;",
      to: "void Entity::set_toughness(double v) {\n  v = round_float(v);",
    },
    {
      note: "a toughness drop does not restore resting toughness",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (v < o) set_toughness_resting(toughness_resting_max());",
      to: "  (void)0;",
    },
    {
      note: "toughness_max is not clamped at zero",
      file: "native/lfw/entity/entity.cpp",
      from:
        "void Entity::set_toughness_max(double v) {\n  v = round_float(v);\n  if (v < 0) v = 0;",
      to: "void Entity::set_toughness_max(double v) {\n  v = round_float(v);",
    },
    {
      note: "the resting toughness is stored unrounded",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_toughness_resting(double v) {\n  v = round_float(v);",
      to: "void Entity::set_toughness_resting(double v) {",
    },
    {
      note: "the resting toughness cap is stored unrounded",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_toughness_resting_max(double v) {\n  v = round_float(v);",
      to: "void Entity::set_toughness_resting_max(double v) {",
    },
    {
      note: "the hp cap is read straight from the dataset",
      file: "native/lfw/entity/entity.cpp",
      from:
        "double Entity::hp_max() const {\n  return _hp_max.has_value() ? *_hp_max : num_of(host_->world_dataset(u\"hp_max\"));",
      to:
        "double Entity::hp_max() const {\n  return num_of(host_->world_dataset(u\"hp_max\"));",
    },
    {
      note: "the mp cap is read straight from the dataset",
      file: "native/lfw/entity/entity.cpp",
      from:
        "double Entity::mp_max() const {\n  return _mp_max.has_value() ? *_mp_max : num_of(host_->world_dataset(u\"mp_max\"));",
      to:
        "double Entity::mp_max() const {\n  return num_of(host_->world_dataset(u\"mp_max\"));",
    },
    {
      note: "the hp cap notification reports the old value as the new one",
      file: "native/lfw/entity/entity.cpp",
      from: '  callbacks.call(u"on_hp_max_changed", {ref(), Value(*_hp_max), Value(o)});',
      to: '  callbacks.call(u"on_hp_max_changed", {ref(), Value(o), Value(*_hp_max)});',
    },
    {
      note: "hp is not clamped at zero",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_hp(double v) {\n  const double o = _hp;\n  v = max(0.0, v);",
      to: "void Entity::set_hp(double v) {\n  const double o = _hp;",
    },
    {
      note: "hp is stored unrounded",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_hp(double v) {\n  const double o = _hp;\n  v = max(0.0, v);\n  v = round_float(v);",
      to: "void Entity::set_hp(double v) {\n  const double o = _hp;\n  v = max(0.0, v);",
    },
    {
      note: "the hp loss overwrites instead of accumulating",
      file: "native/lfw/entity/entity.cpp",
      from:
        "    const std::shared_ptr<Summary> s = summary_mgr().get(id);\n    s->set_hp_lost(Value(to_number(s->hp_lost()) + (o - v)));",
      to:
        "    const std::shared_ptr<Summary> s = summary_mgr().get(id);\n    s->set_hp_lost(Value(o - v));",
    },
    {
      note: "the team summary is billed even for independent teams",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (v < o && !defines::is_independent(_team)) {\n    const std::shared_ptr<Summary> s = summary_mgr().get(_team);\n    s->set_hp_lost(",
      to: "  if (v < o) {\n    const std::shared_ptr<Summary> s = summary_mgr().get(_team);\n    s->set_hp_lost(",
    },
    {
      note: "the hp notification swaps its payload",
      file: "native/lfw/entity/entity.cpp",
      from: '  callbacks.call(u"on_hp_changed", {ref(), Value(v), Value(o)});',
      to: '  callbacks.call(u"on_hp_changed", {ref(), Value(o), Value(v)});',
    },
    {
      note: "the alive notification ignores the controller kind",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (ctrl_ != nullptr && ctrl_->is_human() && ((o > 0) != (v > 0))) {",
      to: "  if (ctrl_ != nullptr && ((o > 0) != (v > 0))) {",
    },
    {
      note: "the alive notification always reports alive",
      file: "native/lfw/entity/entity.cpp",
      from: "    host_->mark_players_alive(v > 0);",
      to: "    host_->mark_players_alive(true);",
    },
    {
      note: "the death branch also treats a zero hp write as death",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (o > 0 && v <= 0) {\n    callbacks.call(u\"on_dead\", {ref()});",
      to: "  if (o > 0 && v < 0) {\n    callbacks.call(u\"on_dead\", {ref()});",
    },
    {
      note: "the death notification is dropped",
      file: "native/lfw/entity/entity.cpp",
      from: '    callbacks.call(u"on_dead", {ref()});\n    if (state_on_dead) state_on_dead();',
      to: "    if (state_on_dead) state_on_dead();",
    },
    {
      note: "the state death hook is not called",
      file: "native/lfw/entity/entity.cpp",
      from: "    if (state_on_dead) state_on_dead();",
      to: "    (void)state_on_dead;",
    },
    {
      note: "the gone-frame guard is dropped",
      file: "native/lfw/entity/entity.cpp",
      from: '        frame_id_of(*this) != std::u16string(frame_id::kGone) &&\n        array_length(brokens) > 0) {',
      to: "        array_length(brokens) > 0) {",
    },
    {
      note: "the gone-state guard is dropped",
      file: "native/lfw/entity/entity.cpp",
      from:
        "    if (!strict_equals(state(), Value(static_cast<double>(StateEnum::Gone))) &&",
      to: "    if (true &&",
    },
    {
      note: "the broken-piece guard is dropped",
      file: "native/lfw/entity/entity.cpp",
      from: "        array_length(brokens) > 0) {",
      to: "        true) {",
    },
    {
      note: "the death frame prefers the data record",
      file: "native/lfw/entity/entity.cpp",
      from:
        '    const Value nf = next_frame_of(field_or(frame, u"on_dead"), field_or(_data, u"on_dead"));',
      to:
        '    const Value nf = next_frame_of(field_or(_data, u"on_dead"), field_or(frame, u"on_dead"));',
    },
    {
      note: "a dead entity never recovers hp_r",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (v > _hp_r) set_hp_r(v);",
      to: "  (void)_hp_r;",
    },
    {
      note: "hp_r is not clamped at zero",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_hp_r(double v) {\n  const double o = _hp_r;\n  v = max(0.0, v);",
      to: "void Entity::set_hp_r(double v) {\n  const double o = _hp_r;",
    },
    {
      note: "hp_r is stored unrounded",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_hp_r(double v) {\n  const double o = _hp_r;\n  v = max(0.0, v);\n  v = round_float(v);",
      to: "void Entity::set_hp_r(double v) {\n  const double o = _hp_r;\n  v = max(0.0, v);",
    },
    {
      note: "the mp loss overwrites instead of accumulating",
      file: "native/lfw/entity/entity.cpp",
      from:
        "    const std::shared_ptr<Summary> s = summary_mgr().get(id);\n    s->set_mp_usage(Value(to_number(s->mp_usage()) + (o - v)));",
      to:
        "    const std::shared_ptr<Summary> s = summary_mgr().get(id);\n    s->set_mp_usage(Value(o - v));",
    },
    {
      note: "the exhaust branch also treats a zero mp write as exhaustion",
      file: "native/lfw/entity/entity.cpp",
      from: '  if (o > 0 && v <= 0) {\n    const Value nf = next_frame_of(field_or(frame, u"on_exhaustion"),',
      to: '  if (o > 0 && v < 0) {\n    const Value nf = next_frame_of(field_or(frame, u"on_exhaustion"),',
    },
    {
      note: "the exhaust frame prefers the data record",
      file: "native/lfw/entity/entity.cpp",
      from:
        '    const Value nf = next_frame_of(field_or(frame, u"on_exhaustion"),\n                                   field_or(_data, u"on_exhaustion"));',
      to:
        '    const Value nf = next_frame_of(field_or(_data, u"on_exhaustion"),\n                                   field_or(frame, u"on_exhaustion"));',
    },
    {
      note: "the name getter ignores a stored undefined",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (!std::holds_alternative<NullTag>(_name)) return _name;",
      to: "  if (truthy(_name)) return _name;",
    },
    {
      note: "the name getter uses the player name for every controller",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (ctrl_ != nullptr && ctrl_->is_human()) {",
      to: "  if (ctrl_ != nullptr) {",
    },
    {
      note: "the fallback player name is hard coded",
      file: "native/lfw/entity/entity.cpp",
      from: '    return Value(u"Player " + to_string(field_or(ctrl_->player, u"id")));',
      to: '    return Value(std::u16string(u"Player 7"));',
    },
    {
      note: "the base name is not defaulted to an empty string",
      file: "native/lfw/entity/entity.cpp",
      from:
        "  const Value base_name = field_or(base_of(_data), u\"name\");\n  return nullish(base_name) ? Value(std::u16string()) : base_name;",
      to: "  return field_or(base_of(_data), u\"name\");",
    },
    {
      note: "the name setter does not skip an unchanged value",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_name(const Value& v) {\n  if (strict_equals(v, name())) return;",
      to: "void Entity::set_name(const Value& v) {",
    },
    {
      note: "the name notification passes the raw falsy value",
      file: "native/lfw/entity/entity.cpp",
      from: '                 {ref(), truthy(v) ? v : Value(std::u16string()), o});',
      to: "                 {ref(), v, o});",
    },
    {
      note: "the name notification reports the new value twice",
      file: "native/lfw/entity/entity.cpp",
      from: "  const Value o = _name;\n  _name = v;\n  callbacks.call(u\"on_name_changed\",",
      to: "  const Value o = v;\n  _name = v;\n  callbacks.call(u\"on_name_changed\",",
    },
    {
      note: "the team setter does not skip an unchanged team",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_team(std::u16string v) {\n  if (equals(Value(v), Value(_team))) return;",
      to: "void Entity::set_team(std::u16string v) {",
    },
    {
      note: "the variant keeps the raw NaN",
      file: "native/lfw/entity/entity.cpp",
      from: "  variant = truthy(Value(n)) ? n : 0;",
      to: "  variant = n;",
    },
    {
      note: "a team change does not bump the render effect",
      file: "native/lfw/entity/entity.cpp",
      from: '  callbacks.call(u"on_team_changed", {ref(), Value(_team), Value(o)});\n  ++_render_effect_time;',
      to: '  callbacks.call(u"on_team_changed", {ref(), Value(_team), Value(o)});',
    },
    {
      note: "blinking is not normalised",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_blinking(double v) { _blinking = round_float(max(0.0, v)); }",
      to: "void Entity::set_blinking(double v) { _blinking = v; }",
    },
    {
      note: "invisibility is not normalised",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_invisible(double v) { _invisible = round_float(max(0.0, v)); }",
      to: "void Entity::set_invisible(double v) { _invisible = v; }",
    },
    {
      note: "invulnerability is not normalised",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_invulnerable(double v) { _invulnerable = round_float(max(0.0, v)); }",
      to: "void Entity::set_invulnerable(double v) { _invulnerable = v; }",
    },
    {
      note: "arest is stored unrounded",
      file: "native/lfw/entity/entity.cpp",
      from: "  _arest = round_float(v);",
      to: "  _arest = v;",
    },
    {
      note: "an explicit outline color falls back to the team color",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (!_outline_color.empty()) return _outline_color;",
      to: "  (void)_outline_color;",
    },
    {
      note: "the team outline color lookup is dropped",
      file: "native/lfw/entity/entity.cpp",
      from: "  const Value* map = defines::find(u\"Defines.TeamInfoMap\");",
      to: "  const Value* map = nullptr;",
    },
    {
      note: "outline_enabled treats an undefined write as a number",
      file: "native/lfw/entity/entity.cpp",
      from: "void Entity::set_outline_enabled(const Value& v) {\n  _outline_enabled = opt_num(v);",
      to: "void Entity::set_outline_enabled(const Value& v) {\n  _outline_enabled = to_number(v);",
    },
    {
      note: "a color change does not bump the render effect",
      file: "native/lfw/entity/entity.cpp",
      from: "  _outline_color = std::move(v);\n  ++_render_effect_time;",
      to: "  _outline_color = std::move(v);",
    },
    {
      note: "the controller setter does not skip the same controller",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (ctrl_ == v) return;\n  controller::BaseController* prev = ctrl_;",
      to: "  controller::BaseController* prev = ctrl_;",
    },
    {
      note: "the alive notification ignores the hp",
      file: "native/lfw/entity/entity.cpp",
      from: "  host_->mark_players_alive(ctrl_->is_human() && hp() > 0);",
      to: "  host_->mark_players_alive(ctrl_->is_human());",
    },
    {
      note: "the previous controller is never released",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (prev != nullptr) host_->release_ctrl(prev);",
      to: "  (void)prev;",
    },
    {
      note: "the controller notification swaps its payload",
      file: "native/lfw/entity/entity.cpp",
      from: '  callbacks.call(u"on_ctrl_changed", {ctrl_ref(v), ctrl_ref(prev), ref()});',
      to: '  callbacks.call(u"on_ctrl_changed", {ctrl_ref(prev), ctrl_ref(v), ref()});',
    },
    {
      note: "the key role flips dead_gone",
      file: "native/lfw/entity/entity.cpp",
      from: "  dead_gone = truthy(v) ? 0 : 1;",
      to: "  dead_gone = truthy(v) ? 1 : 0;",
    },
    {
      note: "the key role flips name_visible",
      file: "native/lfw/entity/entity.cpp",
      from: "  name_visible = truthy(v) ? 1 : 0;",
      to: "  name_visible = truthy(v) ? 0 : 1;",
    },
    {
      note: "the key role flips wakeup_invuln",
      file: "native/lfw/entity/entity.cpp",
      from: "  wakeup_invuln = truthy(v) ? 1 : 0;",
      to: "  wakeup_invuln = truthy(v) ? 0 : 1;",
    },
    {
      note: "the automatic key role only matches bosses",
      file: "native/lfw/entity/entity.cpp",
      from: "      if (equals(item, Value(std::u16string(entity_group::kRegular))) ||\n          equals(item, Value(std::u16string(entity_group::kBoss)))) {",
      to: "      if (equals(item, Value(std::u16string(entity_group::kBoss)))) {",
    },
    {
      note: "the automatic key role matches anything",
      file: "native/lfw/entity/entity.cpp",
      from: "  bool v = false;\n  const Array* group_arr = as_array(group());",
      to: "  bool v = true;\n  const Array* group_arr = as_array(group());",
    },
    {
      note: "gravity picks the other dataset key",
      file: "native/lfw/entity/entity.cpp",
      from: '  const Value g2 = ctrl_ != nullptr && ctrl_->is_end(gk::kDefend) ? dataset(u"gravity")\n                                                                  : dataset(u"gravity_d");',
      to: '  const Value g2 = ctrl_ != nullptr && ctrl_->is_end(gk::kDefend) ? dataset(u"gravity_d")\n                                                                  : dataset(u"gravity");',
    },
    {
      note: "the state gravity always wins",
      file: "native/lfw/entity/entity.cpp",
      from: "  return nullish(g1) ? num_of(g2) : to_number(g1);",
      to: "  return num_of(g2);",
    },
    {
      note: "the ball branch of itr_motionless is inverted",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (type() == static_cast<double>(EntityEnum::Ball)) {",
      to: "  if (type() != static_cast<double>(EntityEnum::Ball)) {",
    },
    {
      note: "the frame dataset layer is dropped",
      file: "native/lfw/entity/entity.cpp",
      from: '  Value v = field_or(field_or(frame, u"dataset"), name.c_str());',
      to: "  Value v;",
    },
    {
      note: "the base dataset layer is dropped",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (nullish(v)) v = field_or(base_of(_data), name.c_str());",
      to: "  (void)_data;",
    },
    {
      note: "the background dataset layer is dropped",
      file: "native/lfw/entity/entity.cpp",
      from: "  if (nullish(v)) v = host_->bg_dataset(name);",
      to: "  (void)host_;",
    },
    {
      note: "the frame and base dataset layers are swapped",
      file: "native/lfw/entity/entity.cpp",
      from:
        '  Value v = field_or(field_or(frame, u"dataset"), name.c_str());\n  if (nullish(v)) v = field_or(base_of(_data), name.c_str());',
      to:
        '  Value v = field_or(base_of(_data), name.c_str());\n  if (nullish(v)) v = field_or(field_or(frame, u"dataset"), name.c_str());',
    },
    {
      note: "itr_fall ignores the itr record",
      file: "native/lfw/entity/entity.cpp",
      from: '  const Value fall = field_or(itr, u"fall");\n  return nullish(fall) ? dataset(u"itr_fall") : fall;',
      to: '  return dataset(u"itr_fall");',
    },
    {
      note: "the catch time is not rounded",
      file: "native/lfw/entity/entity.cpp",
      from: "  const double v = round_float(value);\n  if (equals(Value(_catch_time), Value(v))) return *this;",
      to: "  const double v = value;\n  if (equals(Value(_catch_time), Value(v))) return *this;",
    },
    {
      note: "the catch time is not clamped",
      file: "native/lfw/entity/entity.cpp",
      from: "  _catch_time = clamp(v, 0.0, catch_time_max());",
      to: "  _catch_time = v;",
    },
    {
      note: "add_catch_time ignores the current value",
      file: "native/lfw/entity/entity.cpp",
      from: "  return set_catch_time(_catch_time + value);",
      to: "  return set_catch_time(value);",
    },
    {
      note: "reset_armor keeps a falsy armor record as such",
      file: "native/lfw/entity/entity.cpp",
      from: "  armor = truthy(armor_v) ? armor_v : Value(NullTag{});",
      to: "  armor = armor_v;",
    },
    {
      note: "reset_armor applies the toughness before the cap",
      file: "native/lfw/entity/entity.cpp",
      from:
        "  set_toughness_max(num_of(toughness));\n  set_toughness(num_of(toughness));",
      to:
        "  set_toughness(num_of(toughness));\n  set_toughness_max(num_of(toughness));",
    },
    {
      note: "reset_armor prefers the armor toughness tick over the dataset one",
      file: "native/lfw/entity/entity.cpp",
      from:
        '  _toughness_r_tick.set_max(num_of(or_nullish(trt, dataset(u"toughness_r_tick"))));',
      to: '  _toughness_r_tick.set_max(num_of(dataset(u"toughness_r_tick")));',
    },
    {
      note: "reset_armor ignores the armor recovery value",
      file: "native/lfw/entity/entity.cpp",
      from: '  _toughness_r_value = num_of(or_nullish(trv, dataset(u"toughness_r_value")));',
      to: '  _toughness_r_value = num_of(dataset(u"toughness_r_value"));',
    },
    {
      note: "the reference view loses its id",
      file: "native/lfw/entity/entity.cpp",
      from: '  Object o;\n  o.set(u"id", Value(id));\n  return Value(std::make_shared<Object>(o));',
      to: "  Object o;\n  return Value(std::make_shared<Object>(o));",
    },
  ],
};
