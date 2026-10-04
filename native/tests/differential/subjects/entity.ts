import { BaseController } from "../../../../src/LFW/controller/BaseController";
import { Ditto } from "../../../../src/LFW/ditto";
import { Entity } from "../../../../src/LFW/entity/Entity";
import { NSlot, SSlot } from "../../../../src/LFW/entity/EntitySnapshot";
import { summary_mgr } from "../../../../src/LFW/entity/SummaryMgr";
import { State_Base } from "../../../../src/LFW/state/State_Base";
import { States } from "../../../../src/LFW/state/States";
import { WorldDataset } from "../../../../src/LFW/WorldDataset";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

type Any = never;

const r = (v: unknown): string => renderValue(v);

const out: string[] = [];
const log: string[] = [];

// The host side of `Entity`: TS reads `this.world` / `this.lfw`, so the harness hands
// over the same shape with spies.  `world.dataset` is the real `WorldDataset` on both
// sides; `bg.data.dataset` is a plain layer like the port's `IEntityHost::bg_dataset`.
const dataset = new WorldDataset();
const bgDataset: Record<string, unknown> = Object.create(null);

class HumanController extends BaseController {
  readonly __is_human_ctrl__ = true;
}

class BotController extends BaseController {
  readonly __is_bot_ctrl__ = true;
}

let idCounter = 0;
let team = "1";

const worldStub = {
  dataset,
  bg: { data: { dataset: bgDataset }, zoom_x: 1 },
  mark_players_alive: (_e: unknown, alive: boolean): void => {
    log.push("mark_players_alive:" + r(alive));
  },
  sounds: { play: (): void => undefined },
  entity_map: new Map<string, Any>(),
};

// `world.entity_map.get(id)` must always see the current ids, so it is answered from
// the live harness entities instead of a table that `reset` would invalidate.
worldStub.entity_map.get = ((id: string) => {
  if (ent !== undefined && ent.id === id) return ent;
  if (buddy !== undefined && buddy.id === id) return buddy;
  return undefined;
}) as never;

// `lfw.datas.find(id)`, filled by `env data`.
const dataTable = new Map<string, Any>();

// Snapshot buffers shared by `run snap|snapbuf|snappoke|snapapply`.
let gSnapNums: unknown[] = new Array<unknown>(Number(NSlot.COUNT));
let gSnapStrs: unknown[] = new Array<unknown>(Number(SSlot.COUNT));

// `{id}` (or `null`) so `catching` / `catcher` / `bearer` / `holding` print the same
// thing as the port without dumping a whole entity.
const idRef = (e: Entity | null | undefined): unknown => (e ? { id: e.id } : null);

// `marks` is a `Map`, so the dump keeps the insertion order the port's `std::map`
// cannot promise — sort by key to make both sides print the same text.
const marksText = (e: Entity): string =>
  [...(e as unknown as { marks: Map<string, string> }).marks.entries()]
    .map(([k, v]) => `${k}:${v}`)
    .sort()
    .join(",");

const renderNums = (nums: unknown[]): string =>
  Array.from({ length: nums.length }, (_v, k) => r(nums[k])).join(",");

const renderStrs = (strs: unknown[]): string =>
  Array.from({ length: strs.length }, (_v, k) => r(strs[k])).join(",");

type CollisionLike = { itr?: { kind?: unknown }; rest?: unknown };

// The `vrests` / `blockers` / `superpunchs` maps render as `["w1":14:3,…]`; sorted by
// key because the port's `std::map` cannot keep the `Map` insertion order.
const dumpCollisions = (e: Entity, key: "vrests" | "blockers" | "superpunchs"): string =>
  "[" +
  [...(e as unknown as Record<typeof key, Map<string, CollisionLike>>)[key].entries()]
    .sort(([a], [b]) => (a < b ? -1 : a > b ? 1 : 0))
    .map(([k, v]) => `${r(k)}:${r(v.itr?.kind)}:${r(v.rest)}`)
    .join(",") +
  "]";

// The four relation slots on both harness entities, so a back-pointer write on the
// *other* entity (`clean_holding` / `drop_catching`) is visible in the trace.
const relProbe = (): string =>
  [
    ent?.holding,
    ent?.bearer,
    ent?.catching,
    ent?.catcher,
    buddy?.holding,
    buddy?.bearer,
    buddy?.catching,
    buddy?.catcher,
  ]
    .map((v) => r(!!v))
    .join(" ");

const lfwStub = {
  players: new Map<string, Any>(),
  datas: { find: (id: string): Any => dataTable.get(id) },
  get new_team(): string {
    return team;
  },
  get new_id(): string {
    idCounter += 1;
    return "e" + idCounter;
  },
  factory: {
    acquire_ctrl: (cls: new (pid: string, e: Any) => BaseController, pid: string, e: Any) => {
      log.push("acquire_ctrl");
      return new cls(pid, e);
    },
    release_ctrl: (c: BaseController | undefined): void => {
      log.push("release_ctrl:" + ctrlMark(c));
    },
  },
};

(worldStub as unknown as { lfw: unknown }).lfw = lfwStub;

// `Ditto.warn` is a console warning; the port drops it (no trace effect), so the stub
// only has to exist.
Ditto.warn = (() => undefined) as never;

// `Entity` builds its vectors through the host.
Ditto.vec3 = (x = 0, y = 0, z = 0) => {  const v = { x, y, z } as { x: number; y: number; z: number; set: unknown };
  // The real `IVector3` keeps `set` off the own-key list; `Object.keys` (and so
  // `renderValue`) must only see x/y/z.
  Object.defineProperty(v, "set", {
    enumerable: false,
    value: (nx: number, ny: number, nz: number): void => {
      v.x = nx;
      v.y = ny;
      v.z = nz;
    },
  });
  return v as never;
};

let ent: Entity | undefined = undefined;
let buddy: Entity | undefined = undefined;

// --- state registry + the fake state (the 9g wiring) -------------------------
// The hooks are no longer poked onto a stub object: they live on a state that
// `set_state` selects, so a scene must `run reg` + `run setstate` before
// `run hook ...` can be observed — exactly like the real game, where `set_state` is
// the only way a state becomes active.
const states = new States();

// What `run hook ...` configures.  A hook that is "off" returns `undefined` without
// logging, which is what a missing TS callback does (`this._state?.get_gravity?.(…)`).
const hooks = {
  dead: false,
  gravity: false,
  gravity_value: undefined as unknown,
  find: false,
  find_echo: false,
  find_value: undefined as unknown,
  auto_frame: false,
  auto_frame_value: undefined as unknown,
  sudden: false,
  sudden_value: undefined as unknown,
  caught: false,
  caught_value: undefined as unknown,
};

const fid = (frame: unknown): string =>
  r((frame as { id?: unknown } | undefined | null)?.id);

const prevId = (e: Entity): string =>
  fid((e as unknown as { get_prev_frame(): unknown }).get_prev_frame());

class HarnessState extends State_Base {
  constructor(state: number | string) {
    super(state);
    this.enter = (e: Entity, prev: unknown): void => {
      log.push(
        `${r(this.state)}>enter:${e.id}:${fid(e.frame)}:${fid(prev)}:hp=${r(e.hp)}` +
          `:hmax=${r(e.hp_max)}:mp=${r(e.mp)}:ml=${r(e.motionless)}:sh=${r(e.shaking)}` +
          `:st=${r(e.state)}:og=${r(e.is_on_ground)}:team=${r(e.team)}:dt=${r(e.data.type)}` +
          `:jx=${r(e.jumping.x)}:vx=${r(e.velocity.x)}:vy=${r(e.velocity.y)}:vz=${r(e.velocity.z)}` +
          `:pos=${r(e.position.x)}/${r(e.position.y)}/${r(e.position.z)}:pf=${prevId(e)}`,
      );
      // The state touches the entity on entry so the setters stay observable.
      e.motionless = e.motionless;
      e.hp_r = e.hp_r;
    };
    this.on_dead = (e: Entity): void => {
      if (!hooks.dead) return;
      log.push(`state_on_dead:${e.id}:${r(e.hp_r)}`);
    };
    this.get_gravity = (): unknown => (hooks.gravity ? hooks.gravity_value : undefined);
    this.find_frame_by_id = (_e: Entity, id: unknown): unknown =>
      hooks.find ? (hooks.find_echo ? id : hooks.find_value) : undefined;
    this.get_auto_frame = (): unknown =>
      hooks.auto_frame ? hooks.auto_frame_value : undefined;
    this.get_sudden_death_frame = (): unknown =>
      hooks.sudden ? hooks.sudden_value : undefined;
    this.get_caught_end_frame = (): unknown =>
      hooks.caught ? hooks.caught_value : undefined;
  }

  leave(e: Entity, next: unknown): void {
    log.push(`${r(this.state)}>leave:${e.id}:${fid(next)}`);
  }
}

const dumpStates = (): string =>
  [...states.map.entries()]
    .map(([k, v]) => `${r(k)}:${(v as object).constructor.name}`)
    .join(",");

const ctrlMark = (c: unknown): string => {
  if (!c) return "u";
  const c0 = c as { __is_human_ctrl__?: boolean; __is_bot_ctrl__?: boolean };
  if (c0.__is_human_ctrl__) return "human";
  if (c0.__is_bot_ctrl__) return "bot";
  return "base";
};

const who = (e: unknown): string => (e === ent ? "self" : "?");

const makeCtrl = (kind: string): BaseController | undefined => {
  if (kind === "none") return undefined;
  if (kind === "human") {
    const c = new HumanController("7", ent as never);
    c.player = { id: 7, name: "P7" } as never;
    return c;
  }
  if (kind === "human_bare") {
    const c = new HumanController("9", ent as never);
    c.player = { id: 9 } as never;
    return c;
  }
  if (kind === "bot") return new BotController("7", ent as never);
  if (kind === "base_released") {
    const c = new BaseController("", ent as never);
    (c.keys.d as unknown as { hit: (t?: number) => void }).hit(1);
    return c;
  }
  return new BaseController("", ent as never);
};

function bindCallbacks(e: Entity): void {
  // The port reaches these through `IEntityHost`; here they are instance spies so the
  // frame/opoint/sound side effects are logged instead of running (their own slices
  // port them).
  const spies = e as unknown as Record<string, unknown>;
  spies.enter_frame = (nf: unknown): void => {
    log.push("enter_frame:" + r(nf));
  };
  spies.apply_opoints = (o: unknown): void => {
    log.push("apply_opoints:" + r(o));
  };
  spies.play_sound = (s: unknown): void => {
    log.push("play_sound:" + r(s));
  };
  const on = (key: string, fn: (...args: Any[]) => void): void => {
    (e.callbacks as unknown as { on: (k: string, f: unknown) => void }).on(key, fn);
  };
  on("on_hp_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_hp_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_hp_max_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_hp_max_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_mp_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_mp_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_mp_max_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_mp_max_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_hp_r_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_hp_r_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_team_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_team_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_name_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_name_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_dead", (e2: unknown) => {
    log.push(`on_dead:${who(e2)}`);
  });
  on("on_ctrl_changed", (v: unknown, prev: unknown, e2: unknown) => {
    log.push(`on_ctrl_changed:${ctrlMark(v)}:${ctrlMark(prev)}:${who(e2)}`);
  });
  on("on_reserve_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_reserve_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_resting_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_resting_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_resting_max_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_resting_max_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_fall_value_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_fall_value_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_fall_value_max_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_fall_value_max_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_defend_value_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_defend_value_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_defend_value_max_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_defend_value_max_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_toughness_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_toughness_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_toughness_max_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_toughness_max_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
  on("on_catch_time_max_changed", (e2: unknown, v: unknown, o: unknown) => {
    log.push(`on_catch_time_max_changed:${who(e2)}:${r(v)}:${r(o)}`);
  });
}

// The peek at the stat slots TS keeps private (`_catch_time`, the recovery ticks, …),
// mirroring `Entity::stat_slots()` in the port.
const statSlots = (e: Entity): Record<string, unknown> => {
  const p = e as unknown as Record<string, { max: number } | number>;
  return {
    catch_time: p._catch_time,
    toughness_r_value: p._toughness_r_value,
    fall_r_value: p._fall_r_value,
    defend_r_value: p._defend_r_value,
    toughness_r_tick_max: (p._toughness_r_tick as { max: number }).max,
    hp_r_tick_max: (p._hp_r_tick as { max: number }).max,
    mp_r_tick_max: (p._mp_r_tick as { max: number }).max,
    fall_r_tick_max: (p._fall_r_tick as { max: number }).max,
    defend_r_tick_max: (p._defend_r_tick as { max: number }).max,
  };
};

const roleProbe = (e: Entity): unknown[] => [e.name_visible, e.wakeup_invuln, e.dead_gone];

const NUMERIC_FIELDS = new Set([
  "lifetime", "spawn_time", "render_effect_time", "outline_alpha", "outline_width",
  "mix_strength", "greyscale", "ground_y", "mounted", "ghosted", "reserve",
  "toughness_resting_max", "resting_max", "resting", "fall_value", "toughness",
  "toughness_max", "toughness_resting", "catch_time_max", "fall_value_max",
  "defend_value", "defend_value_max", "defend_ratio", "mp", "hp_r", "hp", "mp_max",
  "hp_max", "blinking", "invisible", "invulnerable", "arest", "gravity",
  "itr_motionless", "weight", "base_type", "type", "variant", "wait", "stat_bar",
  "facing", "motionless", "shaking", "fallinjury", "throwinjury", "name_visible",
  "wakeup_invuln", "dead_gone", "ctrl_visible", "puppet", "is_on_ground",
  "jumping.x", "jumping.y", "jumping.z", "jumping.t", "aabb_min_x", "aabb_max_x",
  "l_len", "r_len", "atom_time", "from_wait_block",
]);

const VALUE_FIELDS = new Set([
  "outline_color", "mix_color", "outline_enabled", "name", "team", "origin_data_id",
  "group", "bot_ignore", "state", "armor", "dead_join", "transforms", "itr", "bdy",
  "frame", "prev_frame", "data", "emitter", "src_emitter", "drink", "ref", "ctrl", "id",
  "velocity", "prev_velocity", "position", "prev_position", "dvx", "dvy", "dvz",
  "landing_frame", "dismiss_time", "dismiss_data",
  "catching", "catcher", "bearer", "holding",
]);

const getNum = (e: Entity, name: string): number => {
  const p = e as unknown as Record<string, number>;
  switch (name) {
    case "lifetime":
      return e.lifetime;
    case "spawn_time":
      return e.spawn_time;
    case "render_effect_time":
      return e.render_effect_time;
    case "outline_alpha":
      return e.outline_alpha;
    case "outline_width":
      return e.outline_width;
    case "mix_strength":
      return e.mix_strength;
    case "greyscale":
      return e.greyscale;
    case "ground_y":
      return e.ground_y;
    case "mounted":
      return e.mounted;
    case "ghosted":
      return e.ghosted;
    case "reserve":
      return e.reserve;
    case "toughness_resting_max":
      return e.toughness_resting_max;
    case "resting_max":
      return e.resting_max;
    case "resting":
      return e.resting;
    case "fall_value":
      return e.fall_value;
    case "toughness":
      return e.toughness;
    case "toughness_max":
      return e.toughness_max;
    case "toughness_resting":
      return e.toughness_resting;
    case "catch_time_max":
      return e.catch_time_max;
    case "fall_value_max":
      return e.fall_value_max;
    case "defend_value":
      return e.defend_value;
    case "defend_value_max":
      return e.defend_value_max;
    case "defend_ratio":
      return e.defend_ratio;
    case "mp":
      return e.mp;
    case "hp_r":
      return e.hp_r;
    case "hp":
      return e.hp;
    case "mp_max":
      return e.mp_max;
    case "hp_max":
      return e.hp_max;
    case "blinking":
      return e.blinking;
    case "invisible":
      return e.invisible;
    case "invulnerable":
      return e.invulnerable;
    case "arest":
      return e.arest;
    case "gravity":
      return e.gravity;
    case "itr_motionless":
      return e.itr_motionless;
    case "weight":
      return e.weight;
    case "base_type":
      return e.base_type;
    case "type":
      return e.type;
    case "variant":
      return e.variant;
    case "wait":
      return e.wait;
    case "stat_bar":
      return e.stat_bar;
    case "facing":
      return e.facing;
    case "motionless":
      return e.motionless;
    case "shaking":
      return e.shaking;
    case "fallinjury":
      return e.fallinjury;
    case "throwinjury":
      return e.throwinjury;
    case "name_visible":
      return e.name_visible;
    case "wakeup_invuln":
      return e.wakeup_invuln;
    case "dead_gone":
      return e.dead_gone;
    case "ctrl_visible":
      return e.ctrl_visible;
    case "puppet":
      return e.puppet ? 1 : 0;
    case "is_on_ground":
      return e.is_on_ground ? 1 : 0;
    case "jumping.x":
      return e.jumping.x;
    case "jumping.y":
      return e.jumping.y;
    case "jumping.z":
      return e.jumping.z;
    case "jumping.t":
      return e.jumping.t;
    case "aabb_min_x":
      return e.aabb_min_x;
    case "aabb_max_x":
      return e.aabb_max_x;
    case "l_len":
      return e.l_len;
    case "r_len":
      return e.r_len;
    case "atom_time":
      return (e as unknown as { _atom_time: number })._atom_time;
    case "from_wait_block":
      return (e as unknown as { _from_wait_block: boolean })._from_wait_block ? 1 : 0;
    default:
      void p;
      return undefined;
  }
};

const getValue = (e: Entity, name: string): unknown => {
  switch (name) {
    case "outline_color":
      return e.outline_color;
    case "mix_color":
      return e.mix_color;
    case "outline_enabled":
      return e.outline_enabled;
    case "name":
      return e.name;
    case "team":
      return e.team;
    case "origin_data_id":
      return e.origin_data_id;
    case "group":
      return e.group;
    case "bot_ignore":
      return e.bot_ignore;
    case "state":
      return e.state;
    case "armor":
      return e.armor;
    case "dead_join":
      return e.dead_join;
    case "transforms":
      return e.transforms;
    case "itr":
      return e.itr;
    case "bdy":
      return e.bdy;
    case "frame":
      return e.frame;
    case "prev_frame":
      return (e as unknown as { _prev_frame: unknown })._prev_frame;
    case "data":
      return e.data;
    case "emitter":
      return e.emitter;
    case "src_emitter":
      return e.src_emitter;
    case "drink":
      return e.drink ? true : undefined;
    case "id":
      return e.id;
    case "ref":
      return { id: e.id };
    case "velocity":
      return e.velocity;
    case "prev_velocity":
      return e.prev_velocity;
    case "position":
      return e.position;
    case "prev_position":
      return e.prev_position;
    case "dvx":
      return e.dvx;
    case "dvy":
      return e.dvy;
    case "dvz":
      return e.dvz;
    case "landing_frame":
      return (e as unknown as { _landing_frame: unknown })._landing_frame;
    case "dismiss_time":
      return e.dismiss_time ?? null;
    case "dismiss_data":
      return e.dismiss_data;
    case "catching":
      return idRef(e.catching);
    case "catcher":
      return idRef(e.catcher);
    case "bearer":
      return idRef(e.bearer);
    case "holding":
      return idRef(e.holding);
    case "ctrl":
      return e.ctrl ? ctrlMark(e.ctrl) : undefined;
    default:
      return undefined;
  }
};

const setNum = (e: Entity, name: string, v: number): boolean => {
  const p = e as unknown as Record<string, number>;
  switch (name) {
    case "atom_time":
      (e as unknown as { _atom_time: number })._atom_time = v;
      return true;
    case "outline_alpha":
      e.outline_alpha = v;
      return true;
    case "outline_width":
      e.outline_width = v;
      return true;
    case "mix_strength":
      e.mix_strength = v;
      return true;
    case "greyscale":
      e.greyscale = v;
      return true;
    case "ghosted":
      p._ghosted = v;
      return true;
    case "reserve":
      e.reserve = v;
      return true;
    case "toughness_resting_max":
      e.toughness_resting_max = v;
      return true;
    case "resting_max":
      e.resting_max = v;
      return true;
    case "resting":
      e.resting = v;
      return true;
    case "fall_value":
      e.fall_value = v;
      return true;
    case "toughness":
      e.toughness = v;
      return true;
    case "toughness_max":
      e.toughness_max = v;
      return true;
    case "toughness_resting":
      e.toughness_resting = v;
      return true;
    case "catch_time_max":
      e.catch_time_max = v;
      return true;
    case "fall_value_max":
      e.fall_value_max = v;
      return true;
    case "defend_value":
      e.defend_value = v;
      return true;
    case "defend_value_max":
      e.defend_value_max = v;
      return true;
    case "defend_ratio":
      e.defend_ratio = v;
      return true;
    case "mp":
      e.mp = v;
      return true;
    case "hp_r":
      e.hp_r = v;
      return true;
    case "hp":
      e.hp = v;
      return true;
    case "mp_max":
      e.mp_max = v;
      return true;
    case "hp_max":
      e.hp_max = v;
      return true;
    case "blinking":
      e.blinking = v;
      return true;
    case "invisible":
      e.invisible = v;
      return true;
    case "invulnerable":
      e.invulnerable = v;
      return true;
    case "arest":
      e.arest = v;
      return true;
    case "variant":
      e.variant = v;
      return true;
    case "wait":
      e.wait = v;
      return true;
    case "stat_bar":
      e.stat_bar = v;
      return true;
    case "facing":
      e.facing = v as never;
      return true;
    case "motionless":
      e.motionless = v;
      return true;
    case "shaking":
      e.shaking = v;
      return true;
    case "fallinjury":
      e.fallinjury = v;
      return true;
    case "throwinjury":
      e.throwinjury = v;
      return true;
    case "name_visible":
      e.name_visible = v;
      return true;
    case "wakeup_invuln":
      e.wakeup_invuln = v;
      return true;
    case "dead_gone":
      e.dead_gone = v;
      return true;
    case "ctrl_visible":
      e.ctrl_visible = v;
      return true;
    case "puppet":
      e.puppet = !!v;
      return true;
    case "is_on_ground":
      e.is_on_ground = !!v;
      return true;
    case "jumping.x":
      e.jumping.x = v;
      return true;
    case "jumping.y":
      e.jumping.y = v;
      return true;
    case "jumping.z":
      e.jumping.z = v;
      return true;
    case "jumping.t":
      e.jumping.t = v;
      return true;
    default:
      void p;
      return false;
  }
};

const setValue = (e: Entity, name: string, v: unknown): boolean => {
  switch (name) {
    case "outline_color":
      e.outline_color = String(v);
      return true;
    case "mix_color":
      e.mix_color = String(v);
      return true;
    case "outline_enabled":
      e.outline_enabled = v as never;
      return true;
    case "name":
      e.name = v as never;
      return true;
    case "team":
      e.team = String(v);
      return true;
    case "dismiss_time":
      e.dismiss_time = v as never;
      return true;
    case "dismiss_data":
      e.dismiss_data = v as never;
      return true;
    case "landing_frame":
      (e as unknown as { _landing_frame: unknown })._landing_frame = v;
      return true;
    case "transforms":
      e.transforms = v as never;
      return true;
    case "dead_join":
      e.dead_join = v as never;
      return true;
    default:
      return false;
  }
};

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_entity.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    let i = 1;
    log.length = 0;
    if (op === "env") {
      const sub = t[i++]!;
      if (sub === "dataset") {
        const idx = [i];
        const key = String(parseValue(t, idx));
        dataset[key as keyof WorldDataset] = parseValue(t, idx) as never;
      } else if (sub === "bg") {
        const idx = [i];
        const key = String(parseValue(t, idx));
        bgDataset[key] = parseValue(t, idx);
      } else if (sub === "team") {
        team = String(parseValue(t, [i]));
      } else if (sub === "data") {
        const idx = [i];
        const id = String(parseValue(t, idx));
        dataTable.set(id, parseValue(t, idx));
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      const what = t[i++]!;
      if (what === "make") {
        ent = new Entity(worldStub as never, parseValue(t, [i]) as never, states as never);
        bindCallbacks(ent);
        out.push(`run make || id=${ent.id} | ${log.join(",")}`);
      } else if (what === "reset") {
        ent!.reset(parseValue(t, [i]) as never);
        out.push(`run reset || id=${ent!.id} | ${log.join(",")}`);
      } else if (what === "resetstates") {
        // `reset(data, states)`: the registry switch is observable, so the scene can
        // tell the default `ENTITY_STATES` apart from the harness registry.
        ent!.reset(parseValue(t, [i]) as never, states as never);
        out.push(`run resetstates || id=${ent!.id} | ${log.join(",")}`);
      } else if (what === "get") {
        const name = t[i++]!;
        let v: unknown = undefined;
        if (NUMERIC_FIELDS.has(name)) v = getNum(ent!, name);
        else if (VALUE_FIELDS.has(name)) v = getValue(ent!, name);
        else {
          process.stderr.write(`unknown get '${name}'\n`);
          process.exit(2);
        }
        out.push(`run get ${name} || ${log.join(",")} | v=${r(v)}`);
      } else if (what === "set") {
        const name = t[i++]!;
        const inV = parseValue(t, [i]);
        let ok = false;
        let outV: unknown = undefined;
        if (VALUE_FIELDS.has(name) && setValue(ent!, name, inV)) {
          ok = true;
          outV = getValue(ent!, name);
        } else if (NUMERIC_FIELDS.has(name) && setNum(ent!, name, Number(inV))) {
          ok = true;
          outV = getNum(ent!, name);
        }
        if (!ok) {
          process.stderr.write(`unknown set '${name}'\n`);
          process.exit(2);
        }
        out.push(`run set ${name} ${r(inV)} || ${log.join(",")} | v=${r(outV)}`);
      } else if (what === "slots") {
        out.push(`run slots || ${log.join(",")} | v=${r(statSlots(ent!))}`);
      } else if (what === "armor") {
        ent!.reset_armor();
        out.push(`run armor || ${log.join(",")} | v=${r(statSlots(ent!))}`);
      } else if (what === "catch") {
        ent!.set_catch_time(Number(parseValue(t, [i])));
        out.push(`run catch || ${log.join(",")} | v=${r(statSlots(ent!))}`);
      } else if (what === "addcatch") {
        ent!.add_catch_time(Number(parseValue(t, [i])));
        out.push(`run addcatch || ${log.join(",")} | v=${r(statSlots(ent!))}`);
      } else if (what === "catching") {
        ent!.set_catching(null);
        out.push(`run catching || ${log.join(",")}`);
      } else if (what === "role") {
        ent!.as_key_role(parseValue(t, [i]));
        out.push(`run role || ${log.join(",")} | v=${r(roleProbe(ent!))}`);
      } else if (what === "autorole") {
        ent!.auto_key_role();
        out.push(`run autorole || ${log.join(",")} | v=${r(roleProbe(ent!))}`);
      } else if (what === "dataset") {
        const key = String(parseValue(t, [i]));
        out.push(
          `run dataset ${r(key)} || ${log.join(",")} | v=${r(ent!.dataset(key as never))}`,
        );
      } else if (what === "itrfall") {
        const itr = parseValue(t, [i]);
        out.push(
          `run itrfall ${r(itr)} || ${log.join(",")} | v=${r(ent!.itr_fall(itr as never))}`,
        );
      } else if (what === "ctrl") {
        const kind = t[i++]!;
        if (kind === "none") ent!.ctrl = undefined;
        else if (kind === "same") ent!.ctrl = ent!.ctrl;
        else ent!.ctrl = makeCtrl(kind);
        out.push(`run ctrl ${kind} || ${log.join(",")} | v=${ctrlMark(ent!.ctrl)}`);
      } else if (what === "frame") {
        ent!.frame = parseValue(t, [i]) as never;
        out.push(`run frame ${r(ent!.frame)} || ${log.join(",")} | v=${r(ent!.frame)}`);
      } else if (what === "pos") {
        const idx = [i];
        const x = Number(parseValue(t, idx));
        const y = Number(parseValue(t, idx));
        const z = Number(parseValue(t, idx));
        ent!.position.set(x, y, z);
        out.push(`run pos ${r(x)} ${r(y)} ${r(z)} || ${log.join(",")} | p=${r(ent!.position)}`);
      } else if (what === "ground") {
        const y = Number(parseValue(t, [i]));
        (ent as unknown as { _ground_y: number })._ground_y = y;
        out.push(`run ground ${r(y)} || ${log.join(",")} | g=${r(ent!.ground_y)}`);
      } else if (what === "link") {
        const field = t[i++]!;
        const to = t[i++]!;
        const v = to === "self" ? (ent as unknown) : to === "buddy" ? (buddy as unknown) : null;
        if (field === "bearer") ent!.bearer = v as never;
        else if (field === "catcher") ent!.catcher = v as never;
        else if (field === "holding") ent!.holding = v as never;
        else if (field === "catching") ent!.catching = v as never;
        else {
          process.stderr.write(`unknown link '${field}'\n`);
          process.exit(2);
        }
        out.push(
          `run link ${field} ${to} || ${log.join(",")} | b=${r(!!ent!.bearer)} c=${r(!!ent!.catcher)}`,
        );
      } else if (what === "linkb") {
        const field = t[i++]!;
        const to = t[i++]!;
        const v = to === "self" ? (ent as unknown) : to === "buddy" ? (buddy as unknown) : null;
        if (field === "bearer") buddy!.bearer = v as never;
        else if (field === "catcher") buddy!.catcher = v as never;
        else if (field === "holding") buddy!.holding = v as never;
        else if (field === "catching") buddy!.catching = v as never;
        else {
          process.stderr.write(`unknown linkb '${field}'\n`);
          process.exit(2);
        }
        out.push(`run linkb ${field} ${to} || ${log.join(",")} | ${relProbe()}`);
      } else if (what === "setvel") {
        const idx = [i];
        const x = parseValue(t, idx);
        const y = parseValue(t, idx);
        const z = parseValue(t, idx);
        ent!.set_velocity(x as never, y as never, z as never);
        out.push(
          `run setvel ${r(x)} ${r(y)} ${r(z)} || ${log.join(",")} | v=${r(ent!.velocity)} pv=${r(ent!.prev_velocity)} g=${r(ent!.is_on_ground)}`,
        );
      } else if (what === "leaveground") {
        ent!.leave_ground();
        out.push(
          `run leaveground || ${log.join(",")} | p=${r(ent!.position)} g=${r(ent!.is_on_ground)}`,
        );
      } else if (what === "gravity") {
        ent!.handle_gravity();
        out.push(`run gravity || ${log.join(",")} | v=${r(ent!.velocity)}`);
      } else if (what === "gdecay") {
        const factor = parseValue(t, [i]);
        if (factor === undefined) ent!.handle_ground_velocity_decay();
        else ent!.handle_ground_velocity_decay(Number(factor));
        out.push(
          `run gdecay ${r(factor)} || ${log.join(",")} | v=${r(ent!.velocity)} pv=${r(ent!.prev_velocity)}`,
        );
      } else if (what === "vdecay") {
        const idx = [i];
        const accx = parseValue(t, idx);
        const accz = parseValue(t, idx);
        const factor = parseValue(t, idx);
        ent!.handle_velocity_decay(accx as never, accz as never, factor as never);
        out.push(
          `run vdecay ${r(accx)} ${r(accz)} ${r(factor)} || ${log.join(",")} | v=${r(ent!.velocity)} pv=${r(ent!.prev_velocity)}`,
        );
      } else if (what === "velocity") {
        const vinfo = parseValue(t, [i]);
        ent!.update_velocity(vinfo as never);
        out.push(
          `run velocity ${r(vinfo)} || ${log.join(",")} | v=${r(ent!.velocity)} pv=${r(ent!.prev_velocity)} g=${r(ent!.is_on_ground)}`,
        );
      } else if (what === "land") {
        const self = t[i] === "self";
        const v = self ? ent!.frame : parseValue(t, [i]);
        (ent as unknown as { _landing_frame: unknown })._landing_frame = v;
        out.push(
          `run land ${self ? "self" : r(v)} || ${log.join(",")} | v=${r((ent as unknown as { _landing_frame: unknown })._landing_frame)}`,
        );
      } else if (what === "keys") {
        const lr = Number(t[i++]);
        const ud = Number(t[i++]);
        const jd = Number(t[i++]);
        const c = makeCtrl("base")!;
        const keys = (c as unknown as { keys: Record<string, { hit: (t?: number) => void }> })
          .keys;
        if (lr > 0) keys.R!.hit(1);
        else if (lr < 0) keys.L!.hit(1);
        if (ud > 0) keys.D!.hit(1);
        else if (ud < 0) keys.U!.hit(1);
        if (jd > 0) keys.j!.hit(1);
        else if (jd < 0) keys.d!.hit(1);
        ent!.ctrl = c;
        out.push(
          `run keys ${r(lr)} ${r(ud)} ${r(jd)} || ${log.join(",")} | lr=${c.LR} ud=${c.UD} jd=${c.jd}`,
        );
      } else if (what === "buddy") {
        buddy = new Entity(worldStub as never, parseValue(t, [i]) as never, states as never);
        out.push(`run buddy || id=${buddy.id} | ${log.join(",")}`);
      } else if (what === "buddyset") {
        const name = t[i++]!;
        const inV = parseValue(t, [i]);
        let outV: unknown = undefined;
        if (VALUE_FIELDS.has(name) && setValue(buddy!, name, inV)) outV = getValue(buddy!, name);
        else if (NUMERIC_FIELDS.has(name) && setNum(buddy!, name, Number(inV))) {
          outV = getNum(buddy!, name);
        } else {
          process.stderr.write(`unknown buddyset '${name}'\n`);
          process.exit(2);
        }
        out.push(`run buddyset ${name} ${r(inV)} || ${log.join(",")} | v=${r(outV)}`);
      } else if (what === "prev") {
        const v = parseValue(t, [i]);
        (ent as unknown as { _prev_frame: unknown })._prev_frame = v;
        out.push(`run prev ${r(v)} || ${log.join(",")} | v=${r(ent!.get_prev_frame())}`);
      } else if (what === "findframe") {
        const id = parseValue(t, [i]);
        out.push(
          `run findframe ${r(id)} || ${log.join(",")} | v=${r(ent!.find_frame_by_id(id as never))}`,
        );
      } else if (what === "autoframe") {
        out.push(`run autoframe || ${log.join(",")} | v=${r(ent!.find_auto_frame())}`);
      } else if (what === "align") {
        const idx = [i];
        const fid = parseValue(t, idx);
        const src = parseValue(t, idx);
        const dst = parseValue(t, idx);
        out.push(
          `run align ${r(fid)} ${r(src)} ${r(dst)} || ${log.join(",")} | v=${r(ent!.find_align_frame(String(fid), src as never, dst as never))}`,
        );
      } else if (what === "suddenframe") {
        out.push(
          `run suddenframe || ${log.join(",")} | v=${r(ent!.get_sudden_death_frame())}`,
        );
      } else if (what === "caughtframe") {
        const v = ent!.get_caught_end_frame();
        out.push(
          `run caughtframe || ${log.join(",")} | v=${r(v)} p=${r(ent!.position)}`,
        );
      } else if (what === "facingflag") {
        const f = parseValue(t, [i]);
        out.push(
          `run facingflag ${r(f)} || ${log.join(",")} | v=${r(ent!.handle_facing_flag(f as never))} f=${r(ent!.facing)}`,
        );
      } else if (what === "waitflag") {
        const idx = [i];
        const w = parseValue(t, idx);
        const f = parseValue(t, idx);
        out.push(
          `run waitflag ${r(w)} ${r(f)} || ${log.join(",")} | v=${r(ent!.handle_wait_flag(w as never, f as never))} w=${r(ent!.wait)}`,
        );
      } else if (what === "framewait") {
        const f = parseValue(t, [i]);
        out.push(
          `run framewait ${r(f)} || ${log.join(",")} | v=${r(ent!.get_frame_wait(f as never))}`,
        );
      } else if (what === "waitblock") {
        const b = Number(t[i++]);
        (ent as unknown as { _from_wait_block: boolean })._from_wait_block = b !== 0;
        out.push(
          `run waitblock ${r(b)} || ${log.join(",")} | v=${r((ent as unknown as { _from_wait_block: boolean })._from_wait_block ? 1 : 0)}`,
        );
      } else if (what === "summaries") {
        const m = summary_mgr as unknown as {
          _items: Map<string, { hp_lost: number; mp_usage: number }>;
          _graves: unknown[];
        };
        let s = "";
        for (const [id, sum] of m._items) s += ` ${id}:${r(sum.hp_lost)}/${r(sum.mp_usage)}`;
        out.push(`run summaries || ${log.join(",")} | graves=${m._graves.length} items${s}`);
      } else if (what === "mark" || what === "delmark") {
        const idx = [i];
        const key = parseValue(t, idx);
        const hasValue = what === "mark";
        const value = hasValue ? parseValue(t, idx) : undefined;
        const hasGuard = idx[0] < t.length;
        const guard = hasGuard ? parseValue(t, idx) : undefined;
        const e = ent as unknown as {
          set_mark(k: string, v: string, prev?: unknown): boolean;
          del_mark(k: string, prev?: unknown): boolean;
        };
        const ok =
          what === "mark"
            ? e.set_mark(String(key), String(value), guard)
            : e.del_mark(String(key), guard);
        out.push(
          `run ${what} ${r(key)} ${hasValue ? r(value) : "-"} ${hasGuard ? r(guard) : "-"} || ${log.join(",")} | v=${r(ok)} marks=${marksText(ent!)}`,
        );
      } else if (what === "ally") {
        const which = t[i++]!;
        const other = which === "buddy" ? buddy : ent;
        out.push(
          `run ally ${which} || ${log.join(",")} | v=${r(ent!.is_ally(other!))} team=${r(ent!.team)} other=${other ? r(other.team) : "-"}`,
        );
      } else if (what === "emit") {
        const idx = Number(t[i++]!);
        const idValue = parseValue(t, [i]);
        const list = (ent as unknown as { emitters: string[] }).emitters;
        list[idx] = String(idValue);
        out.push(
          `run emit ${idx} ${r(idValue)} || ${log.join(",")} | n=${list.length}`,
        );
      } else if (what === "emitid") {
        const idxToken = t[i++]!;
        const which = t[i++]!;
        const idx = Number(idxToken);
        const target = which === "buddy" ? buddy : ent;
        const eid = target !== undefined ? target.id : "";
        const list = (ent as unknown as { emitters: string[] }).emitters;
        list[idx] = eid;
        out.push(
          `run emitid ${idxToken} ${which} || ${log.join(",")} | n=${list.length}`,
        );
      } else if (what === "getemitter") {
        const idx = Number(t[i++]!);
        const found = ent!.get_emitter(idx);
        out.push(
          `run getemitter ${idx} || ${log.join(",")} | v=${found === undefined ? "u" : r(idRef(found))}`,
        );
      } else if (what === "opointz") {
        const which = t[i++]!;
        const other = which === "buddy" ? buddy : which === "null" ? undefined : ent;
        const opoint = parseValue(t, [i]);
        out.push(
          `run opointz ${which} ${r(opoint)} || ${log.join(",")} | v=${r(ent!.get_opoint_speed_z(other!, opoint as never))} state=${r(ent!.state)}`,
        );
      } else if (what === "rec") {
        const which = t[i++]!;
        const e = ent!;
        if (which === "stat") e.stat_recovering();
        else if (which === "hp") e.hp_recovering();
        else if (which === "mp") e.mp_recovering();
        else if (which === "toughness") e.toughness_recovering();
        else if (which === "fall") e.fall_value_recovering();
        else if (which === "defend") e.defend_value_recovering();
        else {
          process.stderr.write(`unknown rec '${which}'\n`);
          process.exit(2);
        }
        out.push(
          `run rec ${which} || ${log.join(",")} | hp=${r(e.hp)} hpr=${r((e as unknown as { _hp_r: number })._hp_r)} mp=${r(e.mp)} mpmax=${r(e.mp_max)} r=${r(e.resting)} t=${r(e.toughness)} tr=${r(e.toughness_resting)} fv=${r(e.fall_value)} dv=${r(e.defend_value)}`,
        );
      } else if (what === "snap" || what === "snapbuf" || what === "snapapply") {
        if (what === "snapapply") ent!.read_snapshot(gSnapNums as never, gSnapStrs as never);
        const nums = new Array<unknown>(Number(NSlot.COUNT));
        const strs = new Array<unknown>(Number(SSlot.COUNT));
        ent!.to_snapshot(nums as never, strs as never);
        if (what === "snapbuf") {
          gSnapNums = nums;
          gSnapStrs = strs;
        }
        out.push(
          `run ${what} || ${log.join(",")} | n=${renderNums(nums)} s=${renderStrs(strs)}`,
        );
      } else if (what === "snappoke") {
        const name = t[i++]!;
        const idx = Number((NSlot as unknown as Record<string, number>)[name]);
        if (!Number.isFinite(idx)) {
          process.stderr.write(`unknown nslot '${name}'\n`);
          process.exit(2);
        }
        gSnapNums[idx] = parseValue(t, [i]);
        out.push(
          `run snappoke ${name} ${r(gSnapNums[idx])} || ${log.join(",")} | v=${r(gSnapNums[idx])}`,
        );
      } else if (what === "snappokestr") {
        const name = t[i++]!;
        const idx = Number((SSlot as unknown as Record<string, number>)[name]);
        if (!Number.isFinite(idx)) {
          process.stderr.write(`unknown sslot '${name}'\n`);
          process.exit(2);
        }
        gSnapStrs[idx] = parseValue(t, [i]);
        out.push(
          `run snappokestr ${name} ${r(gSnapStrs[idx])} || ${log.join(",")} | v=${r(gSnapStrs[idx])}`,
        );
      } else if (what === "snappokeid") {
        const name = t[i++]!;
        const which = t[i++]!;
        const target = which === "buddy" ? buddy : ent;
        const pokeId = target !== undefined ? target.id : "";
        const idx = Number((SSlot as unknown as Record<string, number>)[name]);
        if (!Number.isFinite(idx)) {
          process.stderr.write(`unknown sslot '${name}'\n`);
          process.exit(2);
        }
        gSnapStrs[idx] = pokeId;
        out.push(
          `run snappokeid ${name} ${which} || ${log.join(",")} | v=${r(pokeId)}`,
        );
      } else if (what === "copy") {
        const v = parseValue(t, [i]);
        const cid = String(v);
        const set = (ent as unknown as { copies: Set<string> }).copies;
        const added = !set.has(cid);
        set.add(cid);
        out.push(`run copy ${r(v)} || ${log.join(",")} | added=${r(added)}`);
      } else if (what === "hook") {
        const sub = t[i++]!;
        if (sub === "dead") {
          hooks.dead = true;
        } else if (sub === "gravity") {
          hooks.gravity = true;
          hooks.gravity_value = parseValue(t, [i]);
        } else if (sub === "frameid") {
          hooks.find = true;
          if (t[i] === "echo") {
            i++;
            hooks.find_echo = true;
          } else {
            hooks.find_echo = false;
            hooks.find_value = parseValue(t, [i]);
          }
        } else if (sub === "autoframe") {
          hooks.auto_frame = true;
          hooks.auto_frame_value = parseValue(t, [i]);
        } else if (sub === "sudden") {
          hooks.sudden = true;
          hooks.sudden_value = parseValue(t, [i]);
        } else if (sub === "caught") {
          hooks.caught = true;
          hooks.caught_value = parseValue(t, [i]);
        } else if (sub === "none") {
          hooks.dead = false;
          hooks.gravity = false;
          hooks.find = false;
          hooks.find_echo = false;
          hooks.auto_frame = false;
          hooks.sudden = false;
          hooks.caught = false;
        } else {
          process.stderr.write(`unknown hook '${sub}'\n`);
          process.exit(2);
        }
        out.push(`run hook ${sub} || ${log.join(",")}`);
      } else if (what === "reg" || what === "regbare" || what === "regkey") {
        const bare = what === "regbare";
        const key: unknown = what === "regkey" ? parseValue(t, [i]) : Number(t[i++]);
        states.set(
          key as never,
          (bare ? new State_Base(key as never) : new HarnessState(key as never)) as never,
        );
        out.push(`run ${what} ${r(key)} || ${log.join(",")} | n=${states.map.size}`);
      } else if (what === "setstate" || what === "setstateb") {
        const code = Number(t[i++]!);
        const e = (what === "setstateb" ? buddy : ent)!;
        e.set_state(code);
        out.push(
          `run ${what} ${r(code)} || ${log.join(",")} | n=${states.map.size} st=${r(
            (e as unknown as { _state: unknown })._state != null,
          )}`,
        );
      } else if (what === "vrest" || what === "vrestget" || what === "vrestdel") {
        const idx = [i];
        const aidValue = parseValue(t, idx);
        const aid = String(aidValue);
        let head = `run ${what} ${r(aidValue)}`;
        if (what === "vrest") {
          const kind = parseValue(t, idx);
          const rest = parseValue(t, idx);
          head += ` ${r(kind)} ${r(rest)}`;
          // The port builds its `Collision` struct directly; only `aid`, `itr.kind`
          // and `rest` are read by the three v_rest entry points.
          ent!.add_v_rest({ aid, itr: { kind }, rest: Number(rest) } as never);
        } else if (what === "vrestdel") {
          ent!.del_v_rest(aid);
        }
        const sizes = ent as unknown as {
          vrests: Map<string, CollisionLike>;
          blockers: Map<string, CollisionLike>;
          superpunchs: Map<string, CollisionLike>;
        };
        out.push(
          `${head} || ${log.join(",")} | n=${sizes.vrests.size} b=${sizes.blockers.size} s=${
            sizes.superpunchs.size
          } g=${r(ent!.get_v_rest(aid))}`,
        );
      } else if (what === "vrestdump") {
        out.push(
          `run vrestdump || ${log.join(",")} | n=${dumpCollisions(ent!, "vrests")} b=${dumpCollisions(
            ent!,
            "blockers",
          )} s=${dumpCollisions(ent!, "superpunchs")}`,
        );
      } else if (what === "flag") {
        const which = t[i++]!;
        const other = which === "buddy" ? (buddy as Entity) : (ent as Entity);
        out.push(
          `run flag ${which} || ${log.join(",")} | v=${r(ent!.get_flag(other))} t=${r(
            ent!.team,
          )} ot=${r(other.team)} ty=${r(ent!.type)} h=${r(ent!.hp)}`,
        );
      } else if (what === "cleanhold" || what === "cleancatch") {
        if (what === "cleanhold") ent!.clean_holding();
        else ent!.clean_catching();
        out.push(`run ${what} || ${log.join(",")} | ${relProbe()}`);
      } else if (what === "dropcatch") {
        const dropped = ent!.drop_catching();
        out.push(`run dropcatch || ${log.join(",")} | v=${r(dropped)} ${relProbe()}`);
      } else if (what === "blinkgone" || what === "blinkrespawn") {
        const d = parseValue(t, [i]);
        if (what === "blinkgone") ent!.blink_and_gone(Number(d));
        else ent!.blink_and_respawn(Number(d));
        out.push(
          `run ${what} ${r(d)} || ${log.join(",")} | bl=${r(ent!.blinking)} ab=${r(
            (ent as unknown as { _after_blink: unknown })._after_blink,
          )}`,
        );
      } else if (what === "itrground") {
        const itrs = parseValue(t, [i]);
        ent!.update_itr_bdy_hit_ground(itrs as never);
        out.push(
          `run itrground ${r(itrs)} || ${log.join(",")} | p=${r(ent!.position)} g=${r(
            ent!.ground_y,
          )}`,
        );
      } else if (what === "statesdump") {
        out.push(`run statesdump || ${log.join(",")} | v=${dumpStates()}`);
      } else {
        process.stderr.write(`unknown run '${what}'\n`);
        process.exit(2);
      }
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
