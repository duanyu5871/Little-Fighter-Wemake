/**
 * Mutation spec for `native/lfw/collision/healing.cpp`.
 *
 * Disproven / unobservable mutations (proven by reading the TS original, the port
 * and the case file, not by a surviving run):
 *
 * 1. `if (v == nullptr) return;` removed, or rewritten as `if (a == nullptr)`, is
 *    unobservable: the harness resolves both `collision.aid` (`"A"`) and
 *    `collision.vid` (`"V"`), so neither lookup can fail. The guard is a port
 *    artefact of turning the TS destructuring (`const { attacker, victim }`) into
 *    two id lookups; it is not reachable from any in-contract input.
 * 2. Swapping the two `max(1.0, ...)` calls or moving the `ticks` line above the
 *    `value` line is behaviour preserving: both are pure reads of the victim's
 *    dataset and the arithmetic is applied afterwards.
 * 3. `g_env.buff_env()` is a resource accessor with no alternative expression; the
 *    whole `grant_buff` path (id construction, lifetime/duration/level, attacker
 *    and victim wiring, `mount`) is real ported code owned by the `buff` subject
 *    and is only *used* here. What this unit owns is the `injury` guard, the two
 *    `max(1, dataset)` clamps, the ceiling/ticks arithmetic and *what* entity is
 *    handed to `grant_buff` -- all four are covered below and observable through
 *    the harness (`create_buff:<kind>:<id>` in the log and
 *    `buff=<id>/<lifetime>/<duration>/<level>/<attacker>` in the state text).
 */
export default {
  subject: "collision_healing",
  mutations: [
    {
      note: "injury key misspelled",
      file: "native/lfw/collision/healing.cpp",
      from: '  const Value injury = field_or(c.itr, u"injury");',
      to: '  const Value injury = field_or(c.itr, u"injuries");',
    },
    {
      note: "injury read from bdy instead of itr",
      file: "native/lfw/collision/healing.cpp",
      from: '  const Value injury = field_or(c.itr, u"injury");',
      to: '  const Value injury = field_or(c.bdy, u"injury");',
    },
    {
      note: "falsy injury guard inverted",
      file: "native/lfw/collision/healing.cpp",
      from: "  if (!truthy(injury)) return;",
      to: "  if (truthy(injury)) return;",
    },
    {
      note: "falsy injury guard removed",
      file: "native/lfw/collision/healing.cpp",
      from: "  const Value injury = field_or(c.itr, u\"injury\");\n  if (!truthy(injury)) return;",
      to: '  const Value injury = field_or(c.itr, u"injury");',
    },
    {
      note: "attacker resolved from the victim id",
      file: "native/lfw/collision/healing.cpp",
      from: "  IHealingEntity* a = g_env.find_entity(c.aid);",
      to: "  IHealingEntity* a = g_env.find_entity(c.vid);",
    },
    {
      note: "attacker resolved from an empty id",
      file: "native/lfw/collision/healing.cpp",
      from: "  IHealingEntity* a = g_env.find_entity(c.aid);",
      to: "  IHealingEntity* a = g_env.find_entity(std::u16string());",
    },
    {
      note: "victim resolved from the attacker id",
      file: "native/lfw/collision/healing.cpp",
      from: "  IHealingEntity* v = g_env.find_entity(c.vid);",
      to: "  IHealingEntity* v = g_env.find_entity(c.aid);",
    },
    {
      note: "value dataset key swapped for ticks",
      file: "native/lfw/collision/healing.cpp",
      from: 'to_number(v->dataset(u"hp_healing_value")));',
      to: 'to_number(v->dataset(u"hp_healing_ticks")));',
    },
    {
      note: "ticks dataset key swapped for value",
      file: "native/lfw/collision/healing.cpp",
      from: 'to_number(v->dataset(u"hp_healing_ticks")));',
      to: 'to_number(v->dataset(u"hp_healing_value")));',
    },
    {
      note: "value clamp uses min",
      file: "native/lfw/collision/healing.cpp",
      from: '  const double value = max(1.0, to_number(v->dataset(u"hp_healing_value")));',
      to: '  const double value = min(1.0, to_number(v->dataset(u"hp_healing_value")));',
    },
    {
      note: "ticks clamp uses min",
      file: "native/lfw/collision/healing.cpp",
      from: '  const double ticks = max(1.0, to_number(v->dataset(u"hp_healing_ticks")));',
      to: '  const double ticks = min(1.0, to_number(v->dataset(u"hp_healing_ticks")));',
    },
    {
      note: "value clamp floor lowered to zero",
      file: "native/lfw/collision/healing.cpp",
      from: '  const double value = max(1.0, to_number(v->dataset(u"hp_healing_value")));',
      to: '  const double value = max(0.0, to_number(v->dataset(u"hp_healing_value")));',
    },
    {
      note: "ticks clamp floor lowered to zero",
      file: "native/lfw/collision/healing.cpp",
      from: '  const double ticks = max(1.0, to_number(v->dataset(u"hp_healing_ticks")));',
      to: '  const double ticks = max(0.0, to_number(v->dataset(u"hp_healing_ticks")));',
    },
    {
      note: "duration ceiling replaced by floor",
      file: "native/lfw/collision/healing.cpp",
      from: "  const double duration = ceil(to_number(injury) / value) * ticks;",
      to: "  const double duration = floor(to_number(injury) / value) * ticks;",
    },
    {
      note: "duration ceiling dropped",
      file: "native/lfw/collision/healing.cpp",
      from: "  const double duration = ceil(to_number(injury) / value) * ticks;",
      to: "  const double duration = (to_number(injury) / value) * ticks;",
    },
    {
      note: "injury divided by value becomes multiplied",
      file: "native/lfw/collision/healing.cpp",
      from: "  const double duration = ceil(to_number(injury) / value) * ticks;",
      to: "  const double duration = ceil(to_number(injury) * value) * ticks;",
    },
    {
      note: "injury and value operands swapped",
      file: "native/lfw/collision/healing.cpp",
      from: "  const double duration = ceil(to_number(injury) / value) * ticks;",
      to: "  const double duration = ceil(value / to_number(injury)) * ticks;",
    },
    {
      note: "ticks applied as a divisor",
      file: "native/lfw/collision/healing.cpp",
      from: "  const double duration = ceil(to_number(injury) / value) * ticks;",
      to: "  const double duration = ceil(to_number(injury) / value) / ticks;",
    },
    {
      note: "ticks omitted from the duration",
      file: "native/lfw/collision/healing.cpp",
      from: "  const double duration = ceil(to_number(injury) / value) * ticks;",
      to: "  const double duration = ceil(to_number(injury) / value);",
    },
    {
      note: "buff kind misspelled",
      file: "native/lfw/collision/healing.cpp",
      from: '  buff::grant_buff(g_env.buff_env(), u"Healing", a == nullptr ? nullptr : a->buff_entity(),',
      to: '  buff::grant_buff(g_env.buff_env(), u"healing", a == nullptr ? nullptr : a->buff_entity(),',
    },
    {
      note: "attacker handed to grant_buff is the victim",
      file: "native/lfw/collision/healing.cpp",
      from: "  buff::grant_buff(g_env.buff_env(), u\"Healing\", a == nullptr ? nullptr : a->buff_entity(),\n                   v->buff_entity(), duration);",
      to: '  buff::grant_buff(g_env.buff_env(), u"Healing", v->buff_entity(), v->buff_entity(), duration);',
    },
    {
      note: "victim handed to grant_buff is the attacker",
      file: "native/lfw/collision/healing.cpp",
      from: "  buff::grant_buff(g_env.buff_env(), u\"Healing\", a == nullptr ? nullptr : a->buff_entity(),\n                   v->buff_entity(), duration);",
      to: '  buff::grant_buff(g_env.buff_env(), u"Healing", a == nullptr ? nullptr : a->buff_entity(),\n                   a == nullptr ? nullptr : a->buff_entity(), duration);',
    },
  ],
};
