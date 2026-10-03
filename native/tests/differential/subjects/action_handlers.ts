import { collision_action_handlers } from "../../../../src/LFW/entity/collision_action_handlers";
import { Ditto } from "../../../../src/LFW/ditto";
import { parseJsStringLiteral, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const state = {
  injury: undefined as unknown,
  real_injury: undefined as unknown,
  data_found: undefined as unknown,
  has_data: false,
  ally: false,
  mt_int: 1,
};

let granted: Record<string, unknown> | undefined;

function num(v: unknown): string {
  return renderValue(v);
}

function idsOf(v: unknown): string {
  if (!Array.isArray(v)) return renderValue(v);
  return v
    .map((e) => (typeof e === "string" ? e : String((e as { id?: unknown } | undefined)?.id)))
    .join(",");
}

class Fake {
  _id: string;
  _data: unknown = undefined;
  _data_type: unknown = undefined;
  _velocity: { x: number; y: number; z: number } = { x: 3, y: 0, z: 0 };
  _facing: unknown = 1;
  _team: unknown = undefined;
  _hp: unknown = 10;
  _hp_r: unknown = 10;
  _hp_max: unknown = 20;
  _mp: unknown = 5;
  _mp_max: unknown = 15;
  bot = false;
  src_emitter: unknown = undefined;
  emitter: unknown = undefined;
  bearer: unknown = null;
  _fuse_bys: unknown = undefined;
  _dismiss_data: unknown = undefined;
  _dismiss_time: unknown = null;
  inv = 0;
  mot = 0;
  invul = 0;

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get data(): unknown {
    return this._data;
  }
  get data_type(): unknown {
    return this._data_type;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: 0, y: 0, z: 0 };
  }
  get velocity(): { x: number; y: number; z: number } {
    return this._velocity;
  }
  set_velocity(x?: number | null, _y?: number | null, _z?: number | null): void {
    log.push(`set_velocity:${renderValue(x)}`);
    if (x != null) this._velocity.x = x;
  }
  get facing(): unknown {
    return this._facing;
  }
  set facing(v: unknown) {
    this._facing = v;
    log.push(`set_facing:${renderValue(v)}`);
  }
  get team(): unknown {
    return this._team;
  }
  set team(v: unknown) {
    this._team = v;
    log.push(`set_team:${renderValue(v)}`);
  }
  get hp(): unknown {
    return this._hp;
  }
  set hp(v: unknown) {
    this._hp = v;
    log.push(`set_hp:${renderValue(v)}`);
  }
  get hp_r(): unknown {
    return this._hp_r;
  }
  set hp_r(v: unknown) {
    this._hp_r = v;
    log.push(`set_hp_r:${renderValue(v)}`);
  }
  get hp_max(): unknown {
    return this._hp_max;
  }
  get mp(): unknown {
    return this._mp;
  }
  set mp(v: unknown) {
    this._mp = v;
    log.push(`set_mp:${renderValue(v)}`);
  }
  get mp_max(): unknown {
    return this._mp_max;
  }
  get is_bot_ctrl(): boolean {
    return this.bot;
  }
  get ctrl(): unknown {
    return this.bot ? { __is_bot_ctrl__: true } : undefined;
  }
  get fuse_bys(): unknown {
    return this._fuse_bys;
  }
  set fuse_bys(v: unknown) {
    this._fuse_bys = v;
    log.push(`set_fuse_bys:${idsOf(v)}`);
  }
  get dismiss_data(): unknown {
    return this._dismiss_data;
  }
  set dismiss_data(v: unknown) {
    this._dismiss_data = v;
    log.push(`set_dismiss_data:${renderValue(v)}`);
  }
  get dismiss_time(): unknown {
    return this._dismiss_time;
  }
  set dismiss_time(v: unknown) {
    this._dismiss_time = v;
    log.push(`set_dismiss_time:${renderValue(v)}`);
  }
  get invisible(): number {
    return this.inv;
  }
  set invisible(v: number) {
    this.inv = v;
    log.push(`set_invisible:${renderValue(v)}`);
  }
  get motionless(): number {
    return this.mot;
  }
  set motionless(v: number) {
    this.mot = v;
    log.push(`set_motionless:${renderValue(v)}`);
  }
  get invulnerable(): number {
    return this.invul;
  }
  set invulnerable(v: number) {
    this.invul = v;
    log.push(`set_invulnerable:${renderValue(v)}`);
  }
  play_sound(sounds: unknown, pos: unknown = this.position): void {
    log.push(`${this._id}:play_sound:${renderValue(sounds)}:${renderValue(pos)}`);
  }
  enter_frame(info: unknown): void {
    log.push(`${this._id}:enter_frame:${renderValue(info)}`);
  }
  transform(d: unknown): void {
    log.push(`${this._id}:transform:${renderValue(d)}`);
  }
  is_ally(_other: unknown): boolean {
    log.push(`is_ally:${state.ally ? "1" : "0"}`);
    return state.ally;
  }
}

const a = new Fake("A");
const v = new Fake("V");
const e2 = new Fake("E2");

const buffFake = () => {
  const buf: Record<string, unknown> = { lifetime: 0, duration: 0, level: 0 };
  buf.set_attacker = (_x: unknown) => undefined;
  buf.set_victim = (_x: unknown) => undefined;
  buf.mount = () => undefined;
  granted = buf;
  return buf;
};

const worldFake = {
  buffs: { get: (_id: string) => undefined as unknown },
  find_entity: (id: string) => {
    log.push(`find_entity:${id}`);
    if (id === v._id) return v;
    if (id === e2._id) return e2;
    return undefined;
  },
};

const lfwFake = {
  mt: {
    int: () => state.mt_int,
    set mark(m: string) {
      log.push(`mt_mark:${m}`);
    },
  },
  datas: {
    find: (oid: string) => {
      log.push(`find_data:${oid}`);
      return state.has_data ? state.data_found : undefined;
    },
  },
  broadcast: (msg: string) => {
    log.push(`broadcast:${msg}`);
  },
  factory: {
    create_buff: (kind: string, _lfw: unknown, id: string) => {
      log.push(`create_buff:${kind}:${id}`);
      return buffFake();
    },
  },
};

for (const f of [a, v, e2]) {
  (f as unknown as Record<string, unknown>).world = worldFake;
  (f as unknown as Record<string, unknown>).lfw = lfwFake;
}

(Ditto as unknown as { alert: (m: string) => void }).alert = (m: string) => {
  log.push(`alert:${m}`);
};

const collision = {
  attacker: a,
  victim: v,
  world: worldFake,
  lfw: lfwFake,
  get injury(): unknown {
    return state.injury;
  },
  get real_injury(): unknown {
    return state.real_injury;
  },
} as Record<string, unknown>;

function stateText(): string {
  return (
    `a.hp=${num(a.hp)} a.hp_r=${num(a.hp_r)} a.mp=${num(a.mp)} a.vel=${num(a.velocity.x)} a.face=${num(a.facing)} a.team=${num(a.team)} ` +
    `v.hp=${num(v.hp)} v.hp_r=${num(v.hp_r)} v.mp=${num(v.mp)} v.vel=${num(v.velocity.x)} v.face=${num(v.facing)} v.team=${num(v.team)} ` +
    `v.inv=${num(v.inv)}/${num(v.mot)}/${num(v.invul)} ` +
    `e2.hp=${num(e2.hp)} e2.hp_r=${num(e2.hp_r)} e2.mp=${num(e2.mp)} ` +
    `a.fuse=${idsOf(a.fuse_bys)} a.dismiss=${num(a.dismiss_data)} a.dtime=${num(a.dismiss_time)} ` +
    `a.bot=${a.bot ? "1" : "0"} v.bot=${v.bot ? "1" : "0"} v.fuse=${idsOf(v.fuse_bys)} v.dismiss=${num(v.dismiss_data)} v.dtime=${num(v.dismiss_time)} ` +
    `buff=${
      granted === undefined
        ? "none"
        : `${num(granted.lifetime)}/${num(granted.duration)}/${num(granted.level)}`
    }`
  );
}

function walkEntity(f: Fake, field: string, tok: string[], idx: number[]): void {
  if (field === "hp") f._hp = parseValue(tok, idx);
  else if (field === "hp_r") f._hp_r = parseValue(tok, idx);
  else if (field === "hp_max") f._hp_max = parseValue(tok, idx);
  else if (field === "mp") f._mp = parseValue(tok, idx);
  else if (field === "mp_max") f._mp_max = parseValue(tok, idx);
  else if (field === "vel") f._velocity.x = Number(parseValue(tok, idx));
  else if (field === "face") f._facing = parseValue(tok, idx);
  else if (field === "team") f._team = parseValue(tok, idx);
  else if (field === "data") f._data = parseValue(tok, idx);
  else if (field === "bot") {
    f.bot = tok[idx[0]!] === "1";
    idx[0] = idx[0]! + 1;
  } else if (field === "src_emitter") f.src_emitter = parseValue(tok, idx);
  else if (field === "emitter") f.emitter = parseValue(tok, idx);
  else if (field === "fuse_bys") f._fuse_bys = parseValue(tok, idx);
  else if (field === "bearer") {
    f.bearer = tok[idx[0]!] === "1" ? e2 : null;
    idx[0] = idx[0]! + 1;
  } else {
    process.stderr.write(`unknown entity field '${field}'\n`);
    process.exit(2);
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_action_handlers.mjs <case-file>\n");
    process.exit(2);
  }

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    let i = 1;
    log.length = 0;
    granted = undefined;
    if (op === "env") {
      const sub = t[i++]!;
      const idx = [i];
      if (sub === "injury") {
        state.injury = parseValue(t, idx);
      } else if (sub === "real_injury") {
        state.real_injury = parseValue(t, idx);
      } else if (sub === "data_found") {
        state.data_found = parseValue(t, idx);
        state.has_data = true;
      } else if (sub === "no_data") {
        state.has_data = false;
      } else if (sub === "ally") {
        state.ally = t[i++] === "1";
      } else if (sub === "mt_int") {
        state.mt_int = Number(t[i++]!);
      } else if (sub === "a" || sub === "v" || sub === "e") {
        const f = sub === "a" ? a : sub === "v" ? v : e2;
        const field = t[i++]!;
        walkEntity(f, field, t, [i]);
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "act") {
      const type = parseJsStringLiteral(t[i++]!);
      const action = parseValue(t, [i]);
      const table = collision_action_handlers as unknown as Record<
        string,
        ((a: unknown, c: unknown) => unknown) | undefined
      >;
      const fn = table[type];
      const ret = fn ? fn(action, collision) : undefined;
      out.push(`act ${type} ret=${renderValue(ret)} || ${log.join(",")} | ${stateText()}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
