import { handle_fall } from "../../../../src/LFW/collision/handle_fall";
import { EntityEnum } from "../../../../src/LFW/defines";
import { num, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const state = {
  dataset: {} as Record<string, unknown>,
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
  _fall: unknown = 0;
  _fall_max: unknown = 100;
  _defend: unknown = 1;
  _resting: unknown = 0;
  _state: unknown = undefined;
  _face: unknown = 1;
  _type: unknown = EntityEnum.Fighter;
  _vel = [0, 0, 0];
  _fire: unknown = undefined;
  _crit: unknown = undefined;
  _holding: unknown = undefined;
  _ice: unknown = undefined;
  _hit_sounds: unknown = undefined;
  _src_emitter: unknown = undefined;
  _motionless: unknown = undefined;
  _dataset_values: unknown = undefined;
  _elec_dur: unknown = 7;
  _itr_fall: unknown = 2;
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
    return this._defend;
  }
  get resting(): unknown {
    return this._resting;
  }
  set resting(v: unknown) {
    this._resting = v;
    log.push(`${this._id}:set_resting:${renderValue(v)}`);
  }
  get state(): unknown {
    return this._state;
  }
  get facing(): unknown {
    return this._face;
  }
  get data(): Record<string, unknown> {
    return {
      type: this._type,
      indexes: { fire: this._fire, critical_hit: this._crit },
    };
  }
  dataset(key: string): unknown {
    if (key === "electrify_duration") return this._elec_dur;
    if (key === "ivx_f" || key === "ivy_f" || key === "ivz_f" || key === "ivy_d") return 1;
    const d = this._dataset_values as Record<string, unknown> | undefined;
    return d === undefined || d === null ? undefined : d[key];
  }
  get weight(): number {
    return 1;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: 0, y: 0, z: 0 };
  }
  get ground_y(): number {
    return 0;
  }
  get is_on_ground(): boolean {
    return true;
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
  }
  spark_point(a: typeof acube, b: typeof bcube): readonly [number, number, number] {
    const x = a.left;
    const y = b.top;
    const z = a.near;
    log.push(`sp:${renderValue(x)}:${renderValue(y)}:${renderValue(z)}`);
    return [x, y, z] as const;
  }
  enter_frame(info: unknown): void {
    log.push(`${this._id}:enter_frame:${renderValue(info)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${renderValue(id)}`);
  }
  get holding(): unknown {
    return this._holding;
  }
  drop_holding(): void {
    this._holding = undefined;
    log.push(`${this._id}:drop_holding`);
  }
  itr_fall(_itr: unknown): unknown {
    return this._itr_fall;
  }
  get src_emitter(): unknown {
    return this._src_emitter;
  }
  get motionless(): unknown {
    return this._motionless;
  }
  get ice(): unknown {
    return this._ice;
  }
  get shaking(): unknown {
    return undefined;
  }
  get catching(): boolean {
    return false;
  }
  get catcher(): unknown {
    return null;
  }
  get catch_time_max(): unknown {
    return 30;
  }
  buffs_set(_k: string, _b: unknown): void {}
  buffs_delete(_k: string): void {}
}

const a = new Fake("A");
const v = new Fake("V");

const world = {
  get dataset(): Record<string, unknown> {
    return state.dataset;
  },
  spark(x: unknown, y: unknown, z: unknown, f: unknown): void {
    log.push(`spark:${renderValue(x)}:${renderValue(y)}:${renderValue(z)}:${renderValue(f)}`);
  },
  find_entity(id: string): Fake | null {
    if (id === "A") return a;
    if (id === "V") return v;
    return null;
  },
};

for (const f of [a, v]) {
  (f as unknown as Record<string, unknown>).world = world;
}

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
  get a_cube(): typeof acube {
    return acube;
  },
  get b_cube(): typeof bcube {
    return bcube;
  },
} as Record<string, unknown>;

function sideText(f: Fake): string {
  return (
    `${f._id}.hp=${renderValue(f._hp)} ${f._id}.hp_r=${renderValue(f._hp_r)} ` +
    `${f._id}.tough=${renderValue(f._toughness)} ${f._id}.fall=${renderValue(f._fall)} ` +
    `${f._id}.fall_max=${renderValue(f._fall_max)} ${f._id}.defend=${renderValue(f._defend)} ` +
    `${f._id}.resting=${renderValue(f._resting)} ${f._id}.state=${renderValue(f._state)} ` +
    `${f._id}.face=${renderValue(f._face)} ` +
    `${f._id}.vel=${renderValue(f._vel[0])}/${renderValue(f._vel[1])}/${renderValue(f._vel[2])}`
  );
}

function stateText(): string {
  return `${sideText(a)} ${sideText(v)}`;
}

function walkSide(f: Fake, field: string, tok: string[], i: number): number {
  const idx = [i];
  const bump = () => {
    const t = tok[idx[0]!]!;
    idx[0] = idx[0]! + 1;
    return t;
  };
  if (field === "hp") f._hp = parseValue(tok, idx);
  else if (field === "hp_r") f._hp_r = parseValue(tok, idx);
  else if (field === "tough") f._toughness = num(bump());
  else if (field === "fall") f._fall = parseValue(tok, idx);
  else if (field === "fall_max") f._fall_max = parseValue(tok, idx);
  else if (field === "defend") f._defend = parseValue(tok, idx);
  else if (field === "resting") f._resting = parseValue(tok, idx);
  else if (field === "state") f._state = parseValue(tok, idx);
  else if (field === "face") f._face = parseValue(tok, idx);
  else if (field === "type") f._type = parseValue(tok, idx);
  else if (field === "velx") f._vel[0] = num(bump());
  else if (field === "vely") f._vel[1] = num(bump());
  else if (field === "velz") f._vel[2] = num(bump());
  else if (field === "fire") f._fire = parseValue(tok, idx);
  else if (field === "crit") f._crit = parseValue(tok, idx);
  else if (field === "holding") f._holding = parseValue(tok, idx);
  else if (field === "ice") f._ice = parseValue(tok, idx);
  else if (field === "hit_sounds") f._hit_sounds = parseValue(tok, idx);
  else if (field === "src_emitter") f._src_emitter = parseValue(tok, idx);
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
    process.stderr.write("usage: lfw_trace_collision_fall.mjs <case-file>\n");
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
      if (sub === "itr") {
        state.itr = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "dataset") {
        state.dataset = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "velx") {
        iv.dvx = Number(t[i++]!);
      } else if (sub === "vely") {
        iv.dvy = Number(t[i++]!);
      } else if (sub === "velz") {
        iv.dvz = Number(t[i++]!);
      } else if (sub === "acube") {
        readCube(acube, parseValue(t, idx) as Record<string, unknown>);
      } else if (sub === "bcube") {
        readCube(bcube, parseValue(t, idx) as Record<string, unknown>);
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
      handle_fall(collision as never);
      out.push(`run fall || ${log.join(",")} | ${stateText()}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
