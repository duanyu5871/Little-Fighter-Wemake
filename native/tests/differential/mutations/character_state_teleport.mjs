// Mutation spec for the `character_state_teleport` differential slice.
//
// Subject: native/lfw/state/character_state_teleport.{h,cpp}
//
// Notes recorded up front (unobservable-by-design items, not silently skipped):
//  * `(void)prev_frame;` -- the TS `enter(m)` ignores the previous frame; a
//    mutation dropping the cast is a no-op.
//  * a mutation of the *second* out parameter of `e.position(mx, my, mz)`
//    (swapping `my`/`mz`, filling `my` instead of `mx`, ...) is unobservable:
//    the entity side position reader is silent on both harnesses and only `mx`
//    is consumed by the distance formula. The `mx` slot itself IS observable,
//    hence the `e.position(mx, my, mz);` -> `mx = 0;` mutation below.
//  * the deliberate `abs(o.position.z - o.position.z)` bug of the original TS
//    is reproduced verbatim; the mutations below prove that the case file can
//    see the second operand (Z1/Z2 scenario).
//  * the two mirror TS classes share their whole body except for the ally
//    polarity and the accepted-best predicate, so the port merges them into the
//    file-local `pick_target` / `teleport_to` helpers. The merge introduces no
//    branch that the original pair does not already have.

export default {
  subject: "character_state_teleport",
  mutations: [
    {
      note: "empty entity list is not detected",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  const Value list = e.world_entities();",
      to: "  const Value list = Value();",
    },
    {
      note: "the array view of the entity list is dropped",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  const Array* arr = as_array(list);",
      to: "  const Array* arr = nullptr;",
    },
    {
      note: "the loop is entered only when there is no array",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  if (arr != nullptr) {",
      to: "  if (arr == nullptr) {",
    },
    {
      note: "the last entity is skipped",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    for (size_t i = 0; i < arr->size(); ++i) {",
      to: "    for (size_t i = 0; i + 1 < arr->size(); ++i) {",
    },
    {
      note: "the fighter filter is inverted",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (!e.is_fighter_ref(o) || e.is_self_ref(o)) continue;",
      to: "      if (e.is_fighter_ref(o) || e.is_self_ref(o)) continue;",
    },
    {
      note: "the self filter is dropped",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (!e.is_fighter_ref(o) || e.is_self_ref(o)) continue;",
      to: "      if (!e.is_fighter_ref(o)) continue;",
    },
    {
      note: "the fighter and self filters are anded",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (!e.is_fighter_ref(o) || e.is_self_ref(o)) continue;",
      to: "      if (!e.is_fighter_ref(o) && e.is_self_ref(o)) continue;",
    },
    {
      note: "the self filter is negated",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (!e.is_fighter_ref(o) || e.is_self_ref(o)) continue;",
      to: "      if (!e.is_fighter_ref(o) || !e.is_self_ref(o)) continue;",
    },
    {
      note: "the first rejected entity stops the whole search",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (!e.is_fighter_ref(o) || e.is_self_ref(o)) continue;",
      to: "      if (!e.is_fighter_ref(o) || e.is_self_ref(o)) break;",
    },
    {
      note: "the nearest search stops skipping allies",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (nearest_enemy ? e.is_ally_ref(o) : !e.is_ally_ref(o)) continue;",
      to: "      if (nearest_enemy ? e.is_ally_ref(o) : e.is_ally_ref(o)) continue;",
    },
    {
      note: "the nearest search skips enemies instead of allies",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (nearest_enemy ? e.is_ally_ref(o) : !e.is_ally_ref(o)) continue;",
      to: "      if (nearest_enemy ? !e.is_ally_ref(o) : !e.is_ally_ref(o)) continue;",
    },
    {
      note: "the ally polarity of the two searches is swapped",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (nearest_enemy ? e.is_ally_ref(o) : !e.is_ally_ref(o)) continue;",
      to: "      if (nearest_enemy ? !e.is_ally_ref(o) : e.is_ally_ref(o)) continue;",
    },
    {
      note: "the ally filter never rejects",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (nearest_enemy ? e.is_ally_ref(o) : !e.is_ally_ref(o)) continue;",
      to: "      if (false) continue;",
    },
    {
      note: "the whole ally filter is removed",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (nearest_enemy ? e.is_ally_ref(o) : !e.is_ally_ref(o)) continue;\n      if (to_number(e.ref_hp(o)) <= 0) continue;",
      to: "      if (to_number(e.ref_hp(o)) <= 0) continue;",
    },
    {
      note: "dead entities become acceptable",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (to_number(e.ref_hp(o)) <= 0) continue;",
      to: "      if (to_number(e.ref_hp(o)) < 0) continue;",
    },
    {
      note: "only dead entities are acceptable",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (to_number(e.ref_hp(o)) <= 0) continue;",
      to: "      if (to_number(e.ref_hp(o)) > 0) continue;",
    },
    {
      note: "the hp filter never rejects",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (to_number(e.ref_hp(o)) <= 0) continue;",
      to: "      if (false) continue;",
    },
    {
      note: "the whole hp filter is removed",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (to_number(e.ref_hp(o)) <= 0) continue;\n      double mx = 0;",
      to: "      double mx = 0;",
    },
    {
      note: "the own position is never read for the distance",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      e.position(mx, my, mz);",
      to: "      mx = 0;",
    },
    {
      note: "the x term uses the own y",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const double dis = abs(e.ref_position_x(o) - mx) +",
      to: "      const double dis = abs(e.ref_position_x(o) - my) +",
    },
    {
      note: "the x term uses the own z",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const double dis = abs(e.ref_position_x(o) - mx) +",
      to: "      const double dis = abs(e.ref_position_x(o) - mz) +",
    },
    {
      note: "the x term loses its absolute value",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const double dis = abs(e.ref_position_x(o) - mx) +",
      to: "      const double dis = (e.ref_position_x(o) - mx) +",
    },
    {
      note: "the dead z term becomes a real term",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "                         abs(e.ref_position_z(o) - e.ref_position_z(o));",
      to: "                         abs(e.ref_position_z(o) - mz);",
    },
    {
      note: "the dead z term uses the own x",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "                         abs(e.ref_position_z(o) - e.ref_position_z(o));",
      to: "                         abs(e.ref_position_z(o) - mx);",
    },
    {
      note: "the first candidate of the nearest search is not accepted",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const bool better = nearest_enemy ? (best < 0 || dis < best) : (dis > best);",
      to: "      const bool better = nearest_enemy ? (dis < best) : (dis > best);",
    },
    {
      note: "the nearest search replaces on ties",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const bool better = nearest_enemy ? (best < 0 || dis < best) : (dis > best);",
      to: "      const bool better = nearest_enemy ? (best < 0 || dis <= best) : (dis > best);",
    },
    {
      note: "the farthest search replaces on ties",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const bool better = nearest_enemy ? (best < 0 || dis < best) : (dis > best);",
      to: "      const bool better = nearest_enemy ? (best < 0 || dis < best) : (dis >= best);",
    },
    {
      note: "the two searches swap their predicates",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const bool better = nearest_enemy ? (best < 0 || dis < best) : (dis > best);",
      to: "      const bool better = nearest_enemy ? (dis > best) : (best < 0 || dis < best);",
    },
    {
      note: "the nearest search maximises and the farthest minimises",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const bool better = nearest_enemy ? (best < 0 || dis < best) : (dis > best);",
      to: "      const bool better = nearest_enemy ? (best < 0 || dis > best) : (dis < best);",
    },
    {
      note: "the predicate ignores the search direction",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      const bool better = nearest_enemy ? (best < 0 || dis < best) : (dis > best);",
      to: "      const bool better = nearest_enemy ? (best < 0 || dis < best) : (best < 0 || dis < best);",
    },
    {
      note: "every candidate is better",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (better) {",
      to: "      if (true) {",
    },
    {
      note: "no candidate is ever better",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "      if (better) {",
      to: "      if (false) {",
    },
    {
      note: "the best distance is pinned to zero",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "        best = dis;\n        tar = o;",
      to: "        best = 0;\n        tar = o;",
    },
    {
      note: "the best distance is never updated",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "        best = dis;\n        tar = o;",
      to: "        tar = o;",
    },
    {
      note: "the target is never recorded",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "        best = dis;\n        tar = o;",
      to: "        best = dis;",
    },
    {
      note: "no target is ever returned",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  return tar;",
      to: "  return Value();",
    },
    {
      note: "the no target fallback does not keep the own position",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  if (truthy(tar)) {",
      to: "  if (false) {",
    },
    {
      note: "the target branch always runs",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  if (truthy(tar)) {",
      to: "  if (true) {",
    },
    {
      note: "the x offset loses its rounding",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    x = round(e.ref_position_x(tar) - to_number(e.facing()) * 120);",
      to: "    x = e.ref_position_x(tar) - to_number(e.facing()) * 120;",
    },
    {
      note: "the x offset magnitude is wrong",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    x = round(e.ref_position_x(tar) - to_number(e.facing()) * 120);",
      to: "    x = round(e.ref_position_x(tar) - to_number(e.facing()) * 100);",
    },
    {
      note: "the x offset sign is wrong",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    x = round(e.ref_position_x(tar) - to_number(e.facing()) * 120);",
      to: "    x = round(e.ref_position_x(tar) + to_number(e.facing()) * 120);",
    },
    {
      note: "the x offset ignores the facing",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    x = round(e.ref_position_x(tar) - to_number(e.facing()) * 120);",
      to: "    x = round(e.ref_position_x(tar) - 120);",
    },
    {
      note: "the x offset uses the target z",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    x = round(e.ref_position_x(tar) - to_number(e.facing()) * 120);",
      to: "    x = round(e.ref_position_z(tar) - to_number(e.facing()) * 120);",
    },
    {
      note: "the z coordinate loses its rounding",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    z = round(e.ref_position_z(tar));",
      to: "    z = e.ref_position_z(tar);",
    },
    {
      note: "the z coordinate uses the target x",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    z = round(e.ref_position_z(tar));",
      to: "    z = round(e.ref_position_x(tar));",
    },
    {
      note: "the z coordinate wrongly gets the x offset",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "    z = round(e.ref_position_z(tar));",
      to: "    z = round(e.ref_position_z(tar) - to_number(e.facing()) * 120);",
    },
    {
      note: "the ground segment is queried with swapped arguments",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  const Value segment = e.ground_segment(x, z);",
      to: "  const Value segment = e.ground_segment(z, x);",
    },
    {
      note: "the ground y is queried with swapped arguments",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  const double gy = e.ground_y(segment, x, z);",
      to: "  const double gy = e.ground_y(segment, z, x);",
    },
    {
      note: "the ground y is ignored",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  e.set_position(x, gy, z);",
      to: "  e.set_position(x, y, z);",
    },
    {
      note: "the teleport z is dropped",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  e.set_position(x, gy, z);",
      to: "  e.set_position(x, gy, 0);",
    },
    {
      note: "the teleport x is dropped",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  e.set_position(x, gy, z);",
      to: "  e.set_position(0, gy, z);",
    },
    {
      note: "the nearest class searches for the farthest",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  teleport_to(e, pick_target(e, true));",
      to: "  teleport_to(e, pick_target(e, false));",
    },
    {
      note: "the farthest class searches for the nearest",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  teleport_to(e, pick_target(e, false));",
      to: "  teleport_to(e, pick_target(e, true));",
    },
    {
      note: "the nearest class installs the farthest hook",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  enter = &cst_nearest_enter;",
      to: "  enter = &cst_farthest_enter;",
    },
    {
      note: "the farthest class installs the nearest hook",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  enter = &cst_farthest_enter;",
      to: "  enter = &cst_nearest_enter;",
    },
    {
      note: "the nearest hook is never installed",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  enter = &cst_nearest_enter;",
      to: "  enter = nullptr;",
    },
    {
      note: "the farthest hook is never installed",
      file: "native/lfw/state/character_state_teleport.cpp",
      from: "  enter = &cst_farthest_enter;",
      to: "  enter = nullptr;",
    },
    {
      note: "the nearest default state is the farthest one",
      file: "native/lfw/state/character_state_teleport.h",
      from: "      Value state = Value(static_cast<double>(StateEnum::TeleportToNearestEnemy)));",
      to: "      Value state = Value(static_cast<double>(StateEnum::TeleportToFarthestAlly)));",
    },
    {
      note: "the nearest default state is zero",
      file: "native/lfw/state/character_state_teleport.h",
      from: "      Value state = Value(static_cast<double>(StateEnum::TeleportToNearestEnemy)));",
      to: "      Value state = Value(0.0));",
    },
    {
      note: "the farthest default state is the nearest one",
      file: "native/lfw/state/character_state_teleport.h",
      from: "      Value state = Value(static_cast<double>(StateEnum::TeleportToFarthestAlly)));",
      to: "      Value state = Value(static_cast<double>(StateEnum::TeleportToNearestEnemy)));",
    },
    {
      note: "the farthest default state is zero",
      file: "native/lfw/state/character_state_teleport.h",
      from: "      Value state = Value(static_cast<double>(StateEnum::TeleportToFarthestAlly)));",
      to: "      Value state = Value(0.0));",
    },
  ],
};
