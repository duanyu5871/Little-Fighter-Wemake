// Mutation spec for the `character_state_lying` differential slice.
//
// Subject: native/lfw/state/character_state_lying.{h,cpp}
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * `e.set_invulnerable(30)` in the join block is ALWAYS overwritten: the same
//    block sets `wakeup_invuln = 1`, so the following `if (e.wakeup_invuln)` block
//    runs and rewrites `invulnerable`. The value written there is therefore not
//    observable and is intentionally not mutated (the wakeup block itself is).
//  * `set_hp_max` / `set_hp_r` / `set_hp` all receive the same value and are silent,
//    so their write ORDER is unobservable; the VALUE of each is mutated instead.
//  * `player_teams` is a `Set` in the original: insertion order and duplicates do
//    not matter, only membership does (which is mutated).
//  * `truthy(count_a) && count_a % 2` is equivalent to `count_a % 2` when the
//    count is 0 (`0 % 2 === 0`), so dropping the `truthy(count_a)` half is
//    semantically equivalent and cannot be killed by black-box testing.
//  * Both harnesses keep the entity fakes RAW (no `round_float` / `max(0, v)`
//    normalisation on `reserve` / `toughness` / `blinking` / `invulnerable`), so
//    those normalisations are not covered here; they belong to the Entity slice.
//  * `on_dead`'s empty `else if (e.dead_join)` branch has no body to mutate; the
//    branch ORDER is mutated instead.
//  * `find_frame_by_id`'s unused `id` argument is not observable.

export default {
  subject: "character_state_lying",
  mutations: [
    {
      note: "the lying state drops its default state",
      file: "native/lfw/state/character_state_lying.h",
      from: "= Value(static_cast<double>(StateEnum::Lying)));",
      to: "= Value(0.0));",
    },
    {
      note: "the lying state drops its state argument",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "CharacterState_Lying::CharacterState_Lying(Value state)\n    : CharacterState_Base(std::move(state)) {",
      to: "CharacterState_Lying::CharacterState_Lying(Value state)\n    : CharacterState_Base(Value()) {",
    },
    {
      note: "the constructor never installs enter",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  enter = [this](IStateEntity& e, const Value& prev_frame) {",
      to: "  auto unused_enter = [this](IStateEntity& e, const Value& prev_frame) {",
    },
    {
      note: "the constructor never installs on_dead",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  on_dead = &csl_on_dead;",
      to: "  (void)&csl_on_dead;",
    },
    {
      note: "the constructor never installs find_frame_by_id",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  find_frame_by_id = [](IStateEntity& e, const Value& id) -> Value {",
      to: "  auto unused_find = [](IStateEntity& e, const Value& id) -> Value {",
    },
    {
      note: "enter does not clear the attack counter",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_lying_a_count(Value(0.0));",
      to: "    e.set_lying_a_count(Value(1.0));",
    },
    {
      note: "enter does not clear the defend counter",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_lying_d_count(Value(0.0));",
      to: "    e.set_lying_d_count(Value(1.0));",
    },
    {
      note: "enter does not clear the common counter",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_lying_c_count(Value(0.0));",
      to: "    e.set_lying_c_count(Value(1.0));",
    },
    {
      note: "enter does not reset the key list",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_lying_c_count(Value(0.0));\n    e.ctrl_reset_key_list();",
      to: "    e.set_lying_c_count(Value(0.0));",
    },
    {
      note: "enter does not drop the holding",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    const bool holding = e.has_holding();\n    if (holding) e.drop_holding();",
      to: "    const bool holding = e.has_holding();",
    },
    {
      note: "the holding team is set even without a holding",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    if (holding &&\n        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "    if (strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
    },
    {
      note: "the heavy holding test is dropped",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "        true) {",
    },
    {
      note: "the heavy holding test reads the stick type",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Heavy)))) {",
      to: "        strict_equals(e.holding_base_type(), Value(static_cast<double>(WeaponEnum::Stick)))) {",
    },
    {
      note: "enter does not restore the toughness",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_toughness(e.toughness_max());",
      to: "    e.set_toughness(Value(0.0));",
    },
    {
      note: "enter does not clear the resting toughness",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_toughness_resting(Value(0.0));",
      to: "    e.set_toughness_resting(Value(1.0));",
    },
    {
      note: "enter never reports the death",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    if (to_number(e.hp()) <= 0 && on_dead) on_dead(e);",
      to: "    if (to_number(e.hp()) < 0 && on_dead) on_dead(e);",
    },
    {
      note: "enter reports the death while alive",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    if (to_number(e.hp()) <= 0 && on_dead) on_dead(e);",
      to: "    if (to_number(e.hp()) >= 0 && on_dead) on_dead(e);",
    },
    {
      note: "the puppet team is read from the wrong field",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    player_teams.push_back(lfw::field_or(arr->at(i), u\"team\"));",
      to: "    player_teams.push_back(lfw::field_or(arr->at(i), u\"teams\"));",
    },
    {
      note: "the puppet teams are never collected",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  for (size_t i = 0; arr != nullptr && i < arr->size(); ++i) {\n    player_teams.push_back(lfw::field_or(arr->at(i), u\"team\"));\n  }",
      to: "  (void)arr;",
    },
    {
      note: "every puppet team matches",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    if (strict_equals(xs[i], needle)) return true;",
      to: "    if (true) return true;",
    },
    {
      note: "the reserve is never spent",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(e.reserve())) e.set_reserve(Value(to_number(e.reserve()) - 1.0));",
      to: "  if (truthy(e.reserve())) e.set_reserve(Value(to_number(e.reserve()) - 2.0));",
    },
    {
      note: "the reserve counts up",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(e.reserve())) e.set_reserve(Value(to_number(e.reserve()) - 1.0));",
      to: "  if (truthy(e.reserve())) e.set_reserve(Value(to_number(e.reserve()) + 1.0));",
    },
    {
      note: "the respawn test is an alternative",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(e.reserve()) && set_has(player_teams, e.team())) {",
      to: "  if (truthy(e.reserve()) || set_has(player_teams, e.team())) {",
    },
    {
      note: "the team membership test is dropped",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(e.reserve()) && set_has(player_teams, e.team())) {",
      to: "  if (truthy(e.reserve())) {",
    },
    {
      note: "the respawn blink is a gone blink",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.blink_and_respawn(e.world_dataset(u\"gone_blink_time\"));",
      to: "    e.blink_and_gone(e.world_dataset(u\"gone_blink_time\"));",
    },
    {
      note: "the respawn blink uses the lying blink time",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.blink_and_respawn(e.world_dataset(u\"gone_blink_time\"));",
      to: "    e.blink_and_respawn(e.world_dataset(u\"lying_blink_time\"));",
    },
    {
      note: "the gone blink is a respawn blink",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.blink_and_gone(e.world_dataset(u\"gone_blink_time\"));",
      to: "    e.blink_and_respawn(e.world_dataset(u\"gone_blink_time\"));",
    },
    {
      note: "the join no longer blocks the gone blink",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  } else if (truthy(e.dead_join())) {",
      to: "  } else if (false) {",
    },
    {
      note: "the gone flag is tested as the join",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  } else if (truthy(e.dead_gone())) {",
      to: "  } else if (truthy(e.dead_join())) {",
    },
    {
      note: "the heavy holding team is written as a constant",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "      e.holding_set_team(e.team());",
      to: "      e.holding_set_team(Value(std::u16string(u\"9\")));",
    },
    {
      note: "the defend counter is read from the attack counter",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  const Value count_d = e.lying_d_count();",
      to: "  const Value count_d = e.lying_a_count();",
    },
    {
      note: "update skips the base update",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  CharacterState_Base::update(e);\n  const Value count_c = e.lying_c_count();",
      to: "  const Value count_c = e.lying_c_count();",
    },
    {
      note: "the common counter is read from the attack counter",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  const Value count_c = e.lying_c_count();\n  const Value count_a = e.lying_a_count();",
      to: "  const Value count_c = e.lying_a_count();\n  const Value count_a = e.lying_a_count();",
    },
    {
      note: "the attack counter is read from the defend counter",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  const Value count_a = e.lying_a_count();\n  const bool pressing_a = !e.ctrl_is_end(std::u16string(gk::ka));",
      to: "  const Value count_a = e.lying_d_count();\n  const bool pressing_a = !e.ctrl_is_end(std::u16string(gk::ka));",
    },
    {
      note: "the attack key is read as the defend key",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  const bool pressing_a = !e.ctrl_is_end(std::u16string(gk::ka));",
      to: "  const bool pressing_a = !e.ctrl_is_end(std::u16string(gk::kd));",
    },
    {
      note: "the attack counter is not incremented",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  e.set_lying_a_count(js_add(count_a, Value(1.0)));",
      to: "  e.set_lying_a_count(count_a);",
    },
    {
      note: "the attack counter steps by two",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  e.set_lying_a_count(js_add(count_a, Value(1.0)));",
      to: "  e.set_lying_a_count(js_add(count_a, Value(2.0)));",
    },
    {
      note: "the attack counter is added numerically",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  e.set_lying_a_count(js_add(count_a, Value(1.0)));",
      to: "  e.set_lying_a_count(Value(to_number(count_a) + 1.0));",
    },
    {
      note: "the attack modulo uses three",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(count_a) && truthy(Value(std::fmod(to_number(count_a), 2.0))) && pressing_a &&",
      to: "  if (truthy(count_a) && truthy(Value(std::fmod(to_number(count_a), 3.0))) && pressing_a &&",
    },
    {
      note: "the attack modulo uses one",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(count_a) && truthy(Value(std::fmod(to_number(count_a), 2.0))) && pressing_a &&",
      to: "  if (truthy(count_a) && truthy(Value(std::fmod(to_number(count_a), 1.0))) && pressing_a &&",
    },
    {
      note: "the attack odd test requires exactly one",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(count_a) && truthy(Value(std::fmod(to_number(count_a), 2.0))) && pressing_a &&",
      to: "  if (truthy(count_a) && std::fmod(to_number(count_a), 2.0) == 1.0 && pressing_a &&",
    },
    {
      note: "the attack key press is not required",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(count_a) && truthy(Value(std::fmod(to_number(count_a), 2.0))) && pressing_a &&\n      to_number(e.wait()) > 0) {",
      to: "  if (truthy(count_a) && truthy(Value(std::fmod(to_number(count_a), 2.0))) &&\n      to_number(e.wait()) > 0) {",
    },
    {
      note: "a zero wait still runs the attack branch",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "      to_number(e.wait()) > 0) {",
      to: "      to_number(e.wait()) >= 0) {",
    },
    {
      note: "the attack branch does not end the update",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        round_float(to_number(e.wait()) - to_number(e.world_dataset(u\"atom_time\")))));\n    return;",
      to: "        round_float(to_number(e.wait()) - to_number(e.world_dataset(u\"atom_time\")))));",
    },
    {
      note: "the attack branch adds the atom time",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        round_float(to_number(e.wait()) - to_number(e.world_dataset(u\"atom_time\")))));",
      to: "        round_float(to_number(e.wait()) + to_number(e.world_dataset(u\"atom_time\")))));",
    },
    {
      note: "the attack branch never counts up",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_lying_c_count(js_add(count_c, Value(1.0)));\n    e.set_wait(Value(\n        round_float(to_number(e.wait()) - to_number(e.world_dataset(u\"atom_time\")))));",
      to: "    e.set_wait(Value(\n        round_float(to_number(e.wait()) - to_number(e.world_dataset(u\"atom_time\")))));",
    },
    {
      note: "the defend counter is not incremented",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  e.set_lying_d_count(js_add(count_d, Value(1.0)));",
      to: "  e.set_lying_d_count(count_d);",
    },
    {
      note: "the defend key is read as the attack key",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  const bool pressing_d = !e.ctrl_is_end(std::u16string(gk::kd));",
      to: "  const bool pressing_d = !e.ctrl_is_end(std::u16string(gk::ka));",
    },
    {
      note: "the defend odd test requires exactly one",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(count_d) && truthy(Value(std::fmod(to_number(count_d), 2.0))) && pressing_d) {",
      to: "  if (truthy(count_d) && std::fmod(to_number(count_d), 2.0) == 1.0 && pressing_d) {",
    },
    {
      note: "the defend modulo uses three",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(count_d) && truthy(Value(std::fmod(to_number(count_d), 2.0))) && pressing_d) {",
      to: "  if (truthy(count_d) && truthy(Value(std::fmod(to_number(count_d), 3.0))) && pressing_d) {",
    },
    {
      note: "the defend key press is not required",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(count_d) && truthy(Value(std::fmod(to_number(count_d), 2.0))) && pressing_d) {",
      to: "  if (truthy(count_d) && truthy(Value(std::fmod(to_number(count_d), 2.0)))) {",
    },
    {
      note: "the defend branch subtracts the atom time",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        round_float(to_number(e.wait()) + to_number(e.world_dataset(u\"atom_time\")))));\n  }\n}",
      to: "        round_float(to_number(e.wait()) - to_number(e.world_dataset(u\"atom_time\")))));\n  }\n}",
    },
    {
      note: "the defend branch never counts up",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_lying_c_count(js_add(count_c, Value(1.0)));\n    e.set_wait(Value(\n        round_float(to_number(e.wait()) + to_number(e.world_dataset(u\"atom_time\")))));",
      to: "    e.set_wait(Value(\n        round_float(to_number(e.wait()) + to_number(e.world_dataset(u\"atom_time\")))));",
    },
    {
      note: "the defend branch writes the wait without rounding",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        round_float(to_number(e.wait()) + to_number(e.world_dataset(u\"atom_time\")))));",
      to: "        to_number(e.wait()) + to_number(e.world_dataset(u\"atom_time\"))));",
    },
    {
      note: "leave ignores the join",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(e.dead_join()) && to_number(e.hp()) <= 0) {",
      to: "  if (to_number(e.hp()) <= 0) {",
    },
    {
      note: "leave ignores the hp gate",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(e.dead_join()) && to_number(e.hp()) <= 0) {",
      to: "  if (truthy(e.dead_join())) {",
    },
    {
      note: "leave writes the wrong motionless value",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_motionless(Value(30.0));",
      to: "    e.set_motionless(Value(31.0));",
    },
    {
      note: "the join hp is read from the wrong field",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    const Value join_hp = lfw::field_or(dead_join, u\"hp\");",
      to: "    const Value join_hp = lfw::field_or(dead_join, u\"hps\");",
    },
    {
      note: "the join hp fallback is inverted",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    const Value next_hp = is_nullish(join_hp) ? e.hp_max() : join_hp;",
      to: "    const Value next_hp = is_nullish(join_hp) ? join_hp : e.hp_max();",
    },
    {
      note: "the join hp uses a falsy check",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    const Value next_hp = is_nullish(join_hp) ? e.hp_max() : join_hp;",
      to: "    const Value next_hp = truthy(join_hp) ? join_hp : e.hp_max();",
    },
    {
      note: "the join hp max is never written",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_hp_max(next_hp);",
      to: "    e.set_hp_max(Value(0.0));",
    },
    {
      note: "the join hp right value is never written",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_hp_r(next_hp);",
      to: "    e.set_hp_r(Value(0.0));",
    },
    {
      note: "the join hp is never written",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_hp(next_hp);",
      to: "    e.set_hp(Value(0.0));",
    },
    {
      note: "the join team is read from the wrong field",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    const Value join_team = lfw::field_or(dead_join, u\"team\");",
      to: "    const Value join_team = lfw::field_or(dead_join, u\"teams\");",
    },
    {
      note: "the join team fallback is the second team",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_team(is_nullish(join_team) ? Value(std::u16string(team_enum::kTeam_1)) : join_team);",
      to: "    e.set_team(is_nullish(join_team) ? Value(std::u16string(team_enum::kTeam_2)) : join_team);",
    },
    {
      note: "the join team fallback is inverted",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_team(is_nullish(join_team) ? Value(std::u16string(team_enum::kTeam_1)) : join_team);",
      to: "    e.set_team(is_nullish(join_team) ? join_team : Value(std::u16string(team_enum::kTeam_1)));",
    },
    {
      note: "the join reserve is read from the team field",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    const Value join_reserve = lfw::field_or(dead_join, u\"reserve\");",
      to: "    const Value join_reserve = lfw::field_or(dead_join, u\"team\");",
    },
    {
      note: "the join reserve fallback is one",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_reserve(is_nullish(join_reserve) ? Value(0.0) : join_reserve);",
      to: "    e.set_reserve(is_nullish(join_reserve) ? Value(1.0) : join_reserve);",
    },
    {
      note: "the join effect is written with the wrong kind",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.world_etc(px, py, pz, u\"6\");",
      to: "    e.world_etc(px, py, pz, u\"7\");",
    },
    {
      note: "the join effect is never written",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.world_etc(px, py, pz, u\"6\");\n    e.set_outline_color(std::u16string());",
      to: "    e.set_outline_color(std::u16string());",
    },
    {
      note: "the outline colour is not cleared",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_outline_color(std::u16string());",
      to: "    e.set_outline_color(std::u16string(u\"x\"));",
    },
    {
      note: "the pending join is not cleared",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_dead_join(Value(NullTag{}));",
      to: "    (void)dead_join;",
    },
    {
      note: "the wakeup invuln is not armed",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_wakeup_invuln(Value(1.0));",
      to: "    e.set_wakeup_invuln(Value(0.0));",
    },
    {
      note: "the wakeup blink test is inverted",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "  if (truthy(e.wakeup_invuln())) {",
      to: "  if (!truthy(e.wakeup_invuln())) {",
    },
    {
      note: "the wakeup blink uses the gone blink time",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_blinking(e.world_dataset(u\"lying_blink_time\"));",
      to: "    e.set_blinking(e.world_dataset(u\"gone_blink_time\"));",
    },
    {
      note: "the wakeup invulnerability is not refreshed",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_blinking(e.world_dataset(u\"lying_blink_time\"));\n    e.set_invulnerable(e.world_dataset(u\"lying_blink_time\"));",
      to: "    e.set_blinking(e.world_dataset(u\"lying_blink_time\"));",
    },
    {
      note: "the wakeup invulnerability uses the gone blink time",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    e.set_invulnerable(e.world_dataset(u\"lying_blink_time\"));",
      to: "    e.set_invulnerable(e.world_dataset(u\"gone_blink_time\"));",
    },
    {
      note: "a dead frame needs no hp test",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "    if (to_number(e.hp()) <= 0 &&\n        py <= to_number(e.ground_y()) &&",
      to: "    if (to_number(e.hp()) < 0 &&\n        py <= to_number(e.ground_y()) &&",
    },
    {
      note: "the lying frame allows being above the ground",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        py <= to_number(e.ground_y()) &&",
      to: "        py < to_number(e.ground_y()) &&",
    },
    {
      note: "the lying frame compares the x position",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        py <= to_number(e.ground_y()) &&",
      to: "        px <= to_number(e.ground_y()) &&",
    },
    {
      note: "the lying frame test uses another state",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        strict_equals(e.state(), Value(static_cast<double>(StateEnum::Lying))) &&",
      to: "        strict_equals(e.state(), Value(static_cast<double>(StateEnum::Falling))) &&",
    },
    {
      note: "the lying frame test is inverted",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        strict_equals(e.state(), Value(static_cast<double>(StateEnum::Lying))) &&",
      to: "        !strict_equals(e.state(), Value(static_cast<double>(StateEnum::Lying))) &&",
    },
    {
      note: "a pending join still gets the lying frame",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "        !truthy(e.dead_join())) {",
      to: "        truthy(e.dead_join())) {",
    },
    {
      note: "the lying frame is never returned",
      file: "native/lfw/state/character_state_lying.cpp",
      from: "      return e.frame_info();",
      to: "      return Value();",
    },
  ],
};
