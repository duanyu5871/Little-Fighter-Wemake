import { handle_weapon_is_hit } from "../../../../src/LFW/collision/handle_weapon_is_hit";
import { summary_mgr } from "../../../../src/LFW/entity/SummaryMgr";
import { EntityEnum } from "../../../../src/LFW/defines";
import { num, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const state = {
  dataset: {} as Record<string, unknown>,
  rest: 0,
  recov: 0.25,
  itr_motionless: undefined as unknown,
  itr: undefined as Record<string, unknown> | undefined,
};

const iv = { dvx: 0, dvy: 0, dvz: 0 };
const acube = { left: 0, right: 0, bottom: 0, top: 0, near: 0, far: 0 };
const bcube = { left: 0, right: 0, bottom: 0, top: 0, near: 0, far: 0 };

class Fake {
  _id: string;
  _hp: unknown = 50;
  _hp_r: unknown = 50;
  _toughness = 9;
  _tough_max = 100;
  _state: unknown = undefined;
  _face: unknown = 1;
  _base_type: unknown = undefined;
  _team: unknown = 0;
  _bearer = false;
  _dropping = false;
  _on_ground = true;
  _data_id: unknown = undefined;
  _type: unknown = EntityEnum.Fighter;
  _throwings: unknown = undefined;
  _in_the_skys: unknown = undefined;
  _vel = [0, 0, 0];
  _fall: unknown = 5;
  _fall_max: unknown = 100;
  _defend: unknown = 3;
  _resting: unknown = 0;
  _catch_time: unknown = 0;
  _src_emitter: unknown = undefined;
  _shaking: unknown = undefined;
  _motionless: unknown = undefined;
  _ice: unknown = undefined;
  _hit_sounds: unknown = undefined;
  _dataset_values: unknown = undefined;
  _elec_dur: unknown = 7;
  _arest: unknown = undefined;
  _motionless_fallback: unknown = undefined;
  marks = new Set<string>();
  buff_entity = { id: "buff" };

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get hp(): unknown {
    return this._hp;
  }
  set hp(v: unknown) {
    this._hp = v;
    log.push(`${this._id}:set_hp:${renderValue(v)}`);
  }
  get hp_r(): unknown {
    return this._hp_r;
  }
  set hp_r(v: unknown) {
    this._hp_r = v;
    log.push(`${this._id}:set_hp_r:${renderValue(v)}`);
  }
  get toughness(): number {
    return this._toughness;
  }
  set toughness(v: unknown) {
    this._toughness = Number(v);
    log.push(`${this._id}:set_toughness:${renderValue(v)}`);
  }
  get state(): unknown {
    return this._state;
  }
  get facing(): unknown {
    return this._face;
  }
  get base_type(): unknown {
    return this._base_type;
  }
  get team(): unknown {
    return this._team;
  }
  set team(v: unknown) {
    this._team = v;
    log.push(`${this._id}:set_team:${renderValue(v)}`);
  }
  get bearer(): unknown {
    return this._bearer ? { id: "bearer" } : null;
  }
  get dropping(): boolean {
    return this._dropping;
  }
  set dropping(v: unknown) {
    this._dropping = !!v;
    log.push(`${this._id}:set_dropping:${v ? "1" : "0"}`);
  }
  get is_on_ground(): boolean {
    return this._on_ground;
  }
  get data(): Record<string, unknown> {
    return {
      id: this._data_id,
      type: this._type,
      base: { hit_sounds: this._hit_sounds },
      indexes: { throwings: this._throwings, in_the_skys: this._in_the_skys },
    };
  }
  dataset(key: string): unknown {
    if (key === "electrify_duration") return this._elec_dur;
    if (
      key === "ivx_f" ||
      key === "ivy_f" ||
      key === "ivz_f" ||
      key === "ivy_d"
    ) {
      return 1;
    }
    const d = this._dataset_values as Record<string, unknown> | undefined;
    return d === undefined || d === null ? undefined : d[key];
  }
  get weight(): number {
    return 1;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: 0, y: 0, z: 0 };
  }
  get itr_motionless(): unknown {
    return state.itr_motionless;
  }
  get motionless(): unknown {
    return this._motionless;
  }
  set motionless(v: unknown) {
    this._motionless = v;
    log.push(`${this._id}:set_motionless:${renderValue(v)}`);
  }
  get shaking(): unknown {
    return this._shaking;
  }
  set shaking(v: unknown) {
    this._shaking = v;
    log.push(`${this._id}:set_shaking:${renderValue(v)}`);
  }
  get arest(): unknown {
    return this._arest;
  }
  set arest(v: unknown) {
    this._arest = v;
    log.push(`${this._id}:set_arest:${renderValue(v)}`);
  }
  get fall_value(): unknown {
    return this._fall;
  }
  set fall_value(v: unknown) {
    this._fall = v;
    log.push(`${this._id}:set_fall_value:${renderValue(v)}`);
  }
  get fall_value_max(): unknown {
    return this._fall_max;
  }
  get defend_value(): unknown {
    return this._defend;
  }
  set defend_value(v: unknown) {
    this._defend = v;
    log.push(`${this._id}:set_defend_value:${renderValue(v)}`);
  }
  get defend_value_max(): unknown {
    return this._fall_max;
  }
  get resting(): unknown {
    return this._resting;
  }
  set resting(v: unknown) {
    this._resting = v;
    log.push(`${this._id}:set_resting:${renderValue(v)}`);
  }
  get catch_time_max(): unknown {
    return 30;
  }
  get catching(): boolean {
    return false;
  }
  set catching(_v: unknown) {}
  get catcher(): unknown {
    return null;
  }
  set catcher(_v: unknown) {}
  get catch_time(): unknown {
    return 0;
  }
  set catch_time(v: unknown) {
    log.push(`${this._id}:set_catch_time:${renderValue(v)}`);
  }
  get src_emitter(): unknown {
    return this._src_emitter;
  }
  get ice(): unknown {
    return this._ice;
  }
  itr_fall(_itr: unknown): unknown {
    return 2;
  }
  get velocity(): { x: number; y: number; z: number } {
    return { x: this._vel[0]!, y: this._vel[1]!, z: this._vel[2]! };
  }
  set_velocity(x?: unknown, y?: unknown, z?: unknown): void {
    let s = `${this._id}:set_velocity`;
    if (x !== null && x !== undefined) {
      this._vel[0] = Number(x);
      s += `:${renderValue(x)}`;
    }
    if (y !== null && y !== undefined) {
      this._vel[1] = Number(y);
      s += `:${renderValue(y)}`;
    }
    if (z !== null && z !== undefined) {
      this._vel[2] = Number(z);
      s += `:${renderValue(z)}`;
    }
    log.push(s);
    if (this._vel[1]! > 0) this.leave_ground();
  }
  leave_ground(): void {
    this._on_ground = false;
    log.push(`${this._id}:leave_ground`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${renderValue(id)}`);
  }
  spark_point(a: typeof acube, b: typeof bcube): readonly [number, number, number] {
    const x = a.left;
    const y = b.top;
    const z = a.near;
    log.push(`sp:${renderValue(x)}:${renderValue(y)}:${renderValue(z)}`);
    return [x, y, z] as const;
  }
  add_v_rest(c: { vid: string; rest: number }): void {
    log.push(`add_v_rest:${c.vid}:${renderValue(c.rest)}`);
  }
  buffs_set(_k: string, _b: unknown): void {}
  buffs_delete(_k: string): void {}
}

const a = new Fake("A");
const v = new Fake("V");

const world = {
  get dataset(): Record<string, unknown> {
    return { ...state.dataset, hp_recoverability: state.recov };
  },
  spark(x: unknown, y: unknown, z: unknown, f: unknown): void {
    log.push(`spark:${renderValue(x)}:${renderValue(y)}:${renderValue(z)}:${String(f)}`);
  },
  find_entity(id: string): Fake | null {
    if (id === "A") return a;
    if (id === "V") return v;
    return null;
  },
};

const mt = {
  _mark: "",
  get mark(): string {
    return this._mark;
  },
  set mark(v: string) {
    this._mark = v;
    log.push(`mark:${v}`);
  },
  pick(indexes: unknown): string {
    log.push(`pick:${renderValue(indexes)}`);
    return "picked";
  },
  range(_l: number, _r: number): number {
    return 0;
  },
};

for (const f of [a, v]) {
  const rec = f as unknown as Record<string, unknown>;
  rec.world = world;
  rec.lfw = { mt };
}

summary_mgr.apply_damage = ((
  attacker: Fake,
  injury: unknown,
  victim: Fake,
  prev_hp: unknown,
) => {
  log.push(
    `summary:${attacker.id}:${renderValue(injury)}:${victim.id}:${renderValue(prev_hp)}`,
  );
}) as never;

const collision = {
  attacker: a,
  victim: v,
  world,
  aid: "A",
  vid: "V",
  get itr(): Record<string, unknown> {
    return { ...(state.itr ?? {}), dvx: iv.dvx, dvy: iv.dvy, dvz: iv.dvz } as Record<
      string,
      unknown
    >;
  },
  get dataset(): Record<string, unknown> {
    return state.dataset;
  },
  get rest(): number {
    return state.rest;
  },
  get a_cube(): typeof acube {
    return acube;
  },
  get b_cube(): typeof bcube {
    return bcube;
  },
  injury: undefined as unknown,
  injury_r: undefined as unknown,
  real_injury: undefined as unknown,
  real_injury_r: undefined as unknown,
} as Record<string, unknown>;

function sideText(f: Fake): string {
  return (
    `${f._id}.hp=${renderValue(f._hp)} ${f._id}.hp_r=${renderValue(f._hp_r)} ` +
    `${f._id}.tough=${renderValue(f._toughness)} ${f._id}.state=${renderValue(f._state)} ` +
    `${f._id}.face=${renderValue(f._face)} ${f._id}.base_type=${renderValue(f._base_type)} ` +
    `${f._id}.team=${renderValue(f._team)} ${f._id}.bearer=${f._bearer ? 1 : 0} ` +
    `${f._id}.dropping=${f._dropping ? 1 : 0} ${f._id}.on_ground=${f._on_ground ? 1 : 0} ` +
    `${f._id}.data_id=${renderValue(f._data_id)} ` +
    `${f._id}.vel=${renderValue(f._vel[0])}/${renderValue(f._vel[1])}/${renderValue(f._vel[2])}`
  );
}

function stateText(): string {
  return (
    `${sideText(a)} ${sideText(v)}` +
    ` inj=${renderValue(collision.injury)} inj_r=${renderValue(collision.injury_r)}` +
    ` rinj=${renderValue(collision.real_injury)} rinj_r=${renderValue(collision.real_injury_r)}`
  );
}

function walkSide(f: Fake, field: string, tok: string[], i: number): number {
  const idx = [i];
  if (field === "hp") f._hp = parseValue(tok, idx);
  else if (field === "hp_r") f._hp_r = parseValue(tok, idx);
  else if (field === "tough") f._toughness = num(tok[idx[0]!]!);
  else if (field === "state") f._state = parseValue(tok, idx);
  else if (field === "face") f._face = parseValue(tok, idx);
  else if (field === "base_type") f._base_type = parseValue(tok, idx);
  else if (field === "team") f._team = parseValue(tok, idx);
  else if (field === "bearer") {
    f._bearer = tok[idx[0]!] === "1";
    idx[0] = idx[0]! + 1;
  } else if (field === "drop") {
    f._dropping = tok[idx[0]!] === "1";
    idx[0] = idx[0]! + 1;
  } else if (field === "data_id") f._data_id = parseValue(tok, idx);
  else if (field === "throwings") f._throwings = parseValue(tok, idx);
  else if (field === "in_the_sky") f._in_the_skys = parseValue(tok, idx);
  else if (field === "velx") {
    f._vel[0] = num(tok[idx[0]!]!);
    idx[0] = idx[0]! + 1;
  } else if (field === "vely") {
    f._vel[1] = num(tok[idx[0]!]!);
    idx[0] = idx[0]! + 1;
  } else if (field === "velz") {
    f._vel[2] = num(tok[idx[0]!]!);
    idx[0] = idx[0]! + 1;
  } else if (field === "on_ground") {
    f._on_ground = tok[idx[0]!] === "1";
    idx[0] = idx[0]! + 1;
  } else if (field === "fall") f._fall = parseValue(tok, idx);
  else if (field === "fall_max") f._fall_max = parseValue(tok, idx);
  else if (field === "defend") f._defend = parseValue(tok, idx);
  else if (field === "resting") f._resting = parseValue(tok, idx);
  else if (field === "type") f._type = parseValue(tok, idx);
  else if (field === "src_emitter") f._src_emitter = parseValue(tok, idx);
  else if (field === "ice") f._ice = parseValue(tok, idx);
  else if (field === "hit_sounds") f._hit_sounds = parseValue(tok, idx);
  else if (field === "motionless") f._motionless = parseValue(tok, idx);
  else if (field === "dataset") f._dataset_values = parseValue(tok, idx);
  else {
    process.stderr.write(`unknown side field '${field}'\n`);
    process.exit(2);
  }
  return idx[0]!;
}

function readCube(c: typeof acube, v: Record<string, unknown>): void {
  c.left = Number(v.left ?? 0);
  c.right = Number(v.right ?? 0);
  c.bottom = Number(v.bottom ?? 0);
  c.top = Number(v.top ?? 0);
  c.near = Number(v.near ?? 0);
  c.far = Number(v.far ?? 0);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_weapon_is_hit.mjs <case-file>\n");
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
      const idx = [i];
      const val = (): unknown => parseValue(t, idx);
      if (sub === "itr") {
        state.itr = val() as Record<string, unknown>;
      } else if (sub === "dataset") {
        state.dataset = val() as Record<string, unknown>;
      } else if (sub === "rest") {
        state.rest = Number(t[i++]!);
      } else if (sub === "recov") {
        state.recov = Number(t[i++]!);
      } else if (sub === "itr_motionless") {
        state.itr_motionless = val();
      } else if (sub === "velx") {
        iv.dvx = Number(t[i++]!);
      } else if (sub === "vely") {
        iv.dvy = Number(t[i++]!);
      } else if (sub === "velz") {
        iv.dvz = Number(t[i++]!);
      } else if (sub === "acube") {
        readCube(acube, val() as Record<string, unknown>);
      } else if (sub === "bcube") {
        readCube(bcube, val() as Record<string, unknown>);
      } else if (sub === "a" || sub === "v") {
        const f = sub === "a" ? a : v;
        const field = t[i++]!;
        i = walkSide(f, field, t, i);
        if (i !== t.length) {
          process.stderr.write(`trailing tokens after side field '${field}'\n`);
          process.exit(2);
        }
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      const name = t[i++]!;
      if (name === "hit") {
        handle_weapon_is_hit(collision as never);
      } else {
        process.stderr.write(`unknown handler '${name}'\n`);
        process.exit(2);
      }
      out.push(`run ${name} || ${log.join(",")} | ${stateText()}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
