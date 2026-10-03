/**
 * Mutation spec for `native/lfw/state/character_state_dash.{h,cpp}`.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. The constructor's default argument (`StateEnum.Dash`) is unobservable: nothing
 *    in this unit reads the state object's own `_state`.
 * 2. `let next_vx = vx;` is dead: the `if / else if / else if / else if / else` chain
 *    that follows always assigns `next_vx`. Only `next_vz`'s initial value survives
 *    (when the up/down input is falsy), so that one is covered and this one is not.
 * 3. `e.position.y` and `e.ground_y` are read through the `position(...)` out-params
 *    and the `ground_y()` seam; only `py` is consumed, so swapping the *x* and *z*
 *    slots would be unobservable (the port cannot do that anyway -- `position` fills
 *    all three).
 * 4. A missing `velocity.y` becomes `NaN` through the `double`-typed `velocity_y()`
 *    seam, and `NaN != 0` is true -- the same as the TS `undefined !== 0`. So the
 *    `vely u` case is faithful here (unlike `calc_v`, whose `Default` branch would
 *    have returned the raw `undefined`).
 * 5. `if (UD)` / `else if (LR)` are truthiness tests on numbers, matched exactly by
 *    `truthy(Value(ud))` (0 falsy, +/-1 truthy). `NaN` cannot occur for a controller,
 *    so the `double`-typed `ctrl_ud()` / `ctrl_lr()` seams are faithful.
 */
export default {
  subject: "character_state_dash",
  mutations: [
    {
      note: "the airborne abort needs no vertical speed",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (py > to_number(e.ground_y()) && e.velocity_y() != 0) return;",
      to: "  if (py > to_number(e.ground_y()) || e.velocity_y() != 0) return;",
    },
    {
      note: "being exactly on the ground aborts",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (py > to_number(e.ground_y()) && e.velocity_y() != 0) return;",
      to: "  if (py >= to_number(e.ground_y()) && e.velocity_y() != 0) return;",
    },
    {
      note: "the airborne abort never happens",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (py > to_number(e.ground_y()) && e.velocity_y() != 0) return;\n",
      to: "",
    },
    {
      note: "an idle vertical speed aborts",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (py > to_number(e.ground_y()) && e.velocity_y() != 0) return;",
      to: "  if (py > to_number(e.ground_y()) && e.velocity_y() == 0) return;",
    },
    {
      note: "the abort compares the wrong axis",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (py > to_number(e.ground_y()) && e.velocity_y() != 0) return;",
      to: "  if (px > to_number(e.ground_y()) && e.velocity_y() != 0) return;",
    },
    {
      note: "the vertical speed is never read",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (py > to_number(e.ground_y()) && e.velocity_y() != 0) return;",
      to: "  if (py > to_number(e.ground_y())) return;",
    },
    {
      note: "the previous z velocity is not kept",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  Value next_vz = vz;",
      to: "  Value next_vz = Value();",
    },
    {
      note: "the x distance reads the x factor",
      file: "native/lfw/state/character_state_dash.cpp",
      from: '  const Value dash_distance = e.dataset(u"dash_distance");',
      to: '  const Value dash_distance = e.dataset(u"dash_x_f");',
    },
    {
      note: "the x factor reads the x distance",
      file: "native/lfw/state/character_state_dash.cpp",
      from: '  const Value dash_x_f = e.dataset(u"dash_x_f");',
      to: '  const Value dash_x_f = e.dataset(u"dash_distance");',
    },
    {
      note: "the z distance reads the z factor",
      file: "native/lfw/state/character_state_dash.cpp",
      from: '  const Value dash_distancez = e.dataset(u"dash_distancez");',
      to: '  const Value dash_distancez = e.dataset(u"dash_z_f");',
    },
    {
      note: "the z factor reads the z distance",
      file: "native/lfw/state/character_state_dash.cpp",
      from: '  const Value dash_z_f = e.dataset(u"dash_z_f");',
      to: '  const Value dash_z_f = e.dataset(u"dash_distancez");',
    },
    {
      note: "the height reads the height factor",
      file: "native/lfw/state/character_state_dash.cpp",
      from: '  const Value dash_height = e.dataset(u"dash_height");',
      to: '  const Value dash_height = e.dataset(u"dash_h_f");',
    },
    {
      note: "the height factor reads the height",
      file: "native/lfw/state/character_state_dash.cpp",
      from: '  const Value dash_h_f = e.dataset(u"dash_h_f");',
      to: '  const Value dash_h_f = e.dataset(u"dash_height");',
    },
    {
      note: "the x distance is added",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  const double dx = to_number(dash_distance) * to_number(dash_x_f);",
      to: "  const double dx = to_number(dash_distance) + to_number(dash_x_f);",
    },
    {
      note: "the z distance is added",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  const double dz = to_number(dash_distancez) * to_number(dash_z_f);",
      to: "  const double dz = to_number(dash_distancez) + to_number(dash_z_f);",
    },
    {
      note: "the height is added",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  const double vy = to_number(dash_height) * to_number(dash_h_f);",
      to: "  const double vy = to_number(dash_height) + to_number(dash_h_f);",
    },
    {
      note: "the up/down distance is added",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (truthy(Value(ud))) next_vz = Value(ud * dz);",
      to: "  if (truthy(Value(ud))) next_vz = Value(ud + dz);",
    },
    {
      note: "the z axis listens to left/right",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (truthy(Value(ud))) next_vz = Value(ud * dz);",
      to: "  if (truthy(Value(lr))) next_vz = Value(ud * dz);",
    },
    {
      note: "the z axis never moves",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (truthy(Value(ud))) next_vz = Value(ud * dz);\n",
      to: "",
    },
    {
      note: "the up/down input is read from left/right",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  const double ud = e.ctrl_ud();",
      to: "  const double ud = e.ctrl_lr();",
    },
    {
      note: "the up/down input is forced to one",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  const double ud = e.ctrl_ud();",
      to: "  const double ud = 1;",
    },
    {
      note: "the left/right input is forced to one",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  const double lr = e.ctrl_lr();",
      to: "  const double lr = 1;",
    },
    {
      note: "the previous running state test is loose",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (strict_equals(lfw::field_or(prev_frame, u\"state\"),\n                    Value(static_cast<double>(StateEnum::Running)))) {",
      to: "  if (equals(lfw::field_or(prev_frame, u\"state\"),\n               Value(static_cast<double>(StateEnum::Running)))) {",
    },
    {
      note: "the previous running state is another one",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "                    Value(static_cast<double>(StateEnum::Running)))) {",
      to: "                    Value(static_cast<double>(StateEnum::Walking)))) {",
    },
    {
      note: "the previous frame field is misspelled",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  if (strict_equals(lfw::field_or(prev_frame, u\"state\"),",
      to: "  if (strict_equals(lfw::field_or(prev_frame, u\"statee\"),",
    },
    {
      note: "a running entity ignores its facing",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "    next_vx = Value(to_number(e.facing()) * dx);\n  } else if (truthy(Value(lr))) {",
      to: "    next_vx = Value(dx);\n  } else if (truthy(Value(lr))) {",
    },
    {
      note: "a running entity adds its facing",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "    next_vx = Value(to_number(e.facing()) * dx);\n  } else if (truthy(Value(lr))) {",
      to: "    next_vx = Value(to_number(e.facing()) + dx);\n  } else if (truthy(Value(lr))) {",
    },
    {
      note: "the left/right distance ignores its sign",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "    next_vx = Value(lr * dx);",
      to: "    next_vx = Value(dx);",
    },
    {
      note: "the left/right distance is added",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "    next_vx = Value(lr * dx);",
      to: "    next_vx = Value(lr + dx);",
    },
    {
      note: "the left/right branch is skipped",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  } else if (truthy(Value(lr))) {\n    next_vx = Value(lr * dx);\n",
      to: "  } else if (false) {\n    next_vx = Value(lr * dx);\n",
    },
    {
      note: "a zero x velocity counts as forward",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  } else if (to_number(vx) > 0) {",
      to: "  } else if (to_number(vx) >= 0) {",
    },
    {
      note: "a positive x velocity dashes backward",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  } else if (to_number(vx) > 0) {\n    next_vx = Value(dx);",
      to: "  } else if (to_number(vx) < 0) {\n    next_vx = Value(dx);",
    },
    {
      note: "a positive x velocity dashes backward",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  } else if (to_number(vx) > 0) {\n    next_vx = Value(dx);\n  }",
      to: "  } else if (to_number(vx) > 0) {\n    next_vx = Value(-dx);\n  }",
    },
    {
      note: "a negative x velocity dashes forward",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  } else if (to_number(vx) < 0) {\n    next_vx = Value(-dx);",
      to: "  } else if (to_number(vx) < 0) {\n    next_vx = Value(dx);",
    },
    {
      note: "a zero x velocity counts as backward",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  } else if (to_number(vx) < 0) {",
      to: "  } else if (to_number(vx) <= 0) {",
    },
    {
      note: "the backward branch is skipped",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  } else if (to_number(vx) < 0) {\n    next_vx = Value(-dx);\n",
      to: "  } else if (false) {\n    next_vx = Value(-dx);\n",
    },
    {
      note: "the fallback ignores the facing",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "    next_vx = Value(to_number(e.facing()) * dx);\n  }\n  e.set_velocity",
      to: "    next_vx = Value(dx);\n  }\n  e.set_velocity",
    },
    {
      note: "the fallback adds the facing",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "    next_vx = Value(to_number(e.facing()) * dx);\n  }\n  e.set_velocity",
      to: "    next_vx = Value(to_number(e.facing()) + dx);\n  }\n  e.set_velocity",
    },
    {
      note: "the dash velocity is never written",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  e.set_velocity(next_vx, Value(vy), next_vz);\n",
      to: "",
    },
    {
      note: "the dash velocity is written with x and z swapped",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  e.set_velocity(next_vx, Value(vy), next_vz);",
      to: "  e.set_velocity(next_vz, Value(vy), next_vx);",
    },
    {
      note: "the dash height is dropped",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  e.set_velocity(next_vx, Value(vy), next_vz);",
      to: "  e.set_velocity(next_vx, Value(), next_vz);",
    },
    {
      note: "the dash enter hook is not installed",
      file: "native/lfw/state/character_state_dash.cpp",
      from: "  enter = &csd_enter;",
      to: "  enter = nullptr;",
    },
  ],
};
