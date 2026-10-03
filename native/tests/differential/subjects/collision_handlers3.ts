import { handle_armor } from "../../../../src/LFW/collision/handle_armor";
import {
  handle_ball_is_hit_a,
  handle_ball_is_hit_b,
} from "../../../../src/LFW/collision/handle_ball_is_hit";
import { handle_itr_kind_whirlwind } from "../../../../src/LFW/collision/handle_itr_kind_whirlwind";
import { StateEnum, EntityEnum } from "../../../../src/LFW/defines";
import { summary_mgr } from "../../../../src/LFW/entity/SummaryMgr";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

function num(v: unknown): string {
  return renderValue(v);
}

const state = {
  dataset: {} as Record<string, unknown>,
  recov: 0.25 as unknown,
  armorwork: true,
  a_cube: { left: 0, right: 0, bottom: 0, top: 0, near: 0, far: 0 } as Record<string, number>,
  b_cube: { left: 0, right: 0, bottom: 0, top: 0, near: 0, far: 0 } as Record<string, number>,
  itr: {} as Record<string, unknown>,
  rest: 0,
  itr_motionless: undefined as unknown,
};

let granted: Record<string, unknown> | undefined;

class Fake {
  _id: string;
  _hp: unknown = 20;
  _hp_r: unknown = 20;
  _fall: unknown = 5;
  _fall_max: unknown = 100;
  _defend: unknown = 3;
  _defend_max: unknown = 50;
  _resting: unknown = 0;
  _catch_time: unknown = 0;
  _catch_time_max: unknown = 30;
  _shaking: unknown = undefined;
  _motionless: unknown = undefined;
  _catching: unknown = null;
  _catcher: unknown = null;
  _elec: unknown = 7;
  _marks = false;
  _itr_fall: unknown = 2;
  _hit_sounds: unknown = undefined;
  _ice: unknown = undefined;
  _in_the_sky: unknown = undefined;
  _type: unknown = EntityEnum.Fighter;
  _armor: unknown = undefined;
  _team: unknown = undefined;
  _bearer = false;
  _base_type: unknown = undefined;
  _state: unknown = undefined;
  _toughness = 9;
  _tough_max: unknown = 9;
  _dataset_values: Record<string, unknown> = {};
  _velx: unknown = 0;
  _vely: unknown = 0;
  _velz: unknown = 0;
  _posx: unknown = 0;
  _posy: unknown = 0;
  _posz: unknown = 0;
  src_emitter: unknown = undefined;

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get position(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._posx, y: this._posy, z: this._posz };
  }
  get velocity(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._velx, y: this._vely, z: this._velz };
  }
  get data(): Record<string, unknown> {
    return {
      type: this._type,
      indexes: { ice: this._ice, in_the_skys: [this._in_the_sky] },
      base: { hit_sounds: this._hit_sounds },
    };
  }
  get team(): unknown {
    return this._team;
  }
  set team(v: unknown) {
    this._team = v;
    log.push(`${this._id}:set_team:${renderValue(v)}`);
  }
  get bearer(): unknown {
    return this._bearer;
  }
  get base_type(): unknown {
    return this._base_type;
  }
  get state(): unknown {
    return this._state;
  }
  get armor(): unknown {
    return this._armor;
  }
  get toughness(): number {
    return this._toughness;
  }
  set toughness(v: number) {
    this._toughness = v;
    log.push(`${this._id}:set_toughness:${renderValue(v)}`);
  }
  get toughness_max(): unknown {
    return this._tough_max;
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
    return this._defend_max;
  }
  get resting(): unknown {
    return this._resting;
  }
  set resting(v: unknown) {
    this._resting = v;
    log.push(`${this._id}:set_resting:${renderValue(v)}`);
  }
  set_catch_time(v: unknown): void {
    this._catch_time = v;
    log.push(`${this._id}:set_catch_time:${renderValue(v)}`);
  }
  get catch_time_max(): unknown {
    return this._catch_time_max;
  }
  get catching(): unknown {
    return this._catching;
  }
  set catching(v: unknown) {
    this._catching = v;
    log.push(`${this._id}:set_catching:${String((v as { id?: unknown } | null)?.id)}`);
  }
  get catcher(): unknown {
    return this._catcher;
  }
  set catcher(v: unknown) {
    this._catcher = v;
    log.push(`${this._id}:set_catcher:${String((v as { id?: unknown } | null)?.id)}`);
  }
  get shaking(): unknown {
    return this._shaking;
  }
  set shaking(v: unknown) {
    this._shaking = v;
    log.push(`${this._id}:set_shaking:${renderValue(v)}`);
  }
  get motionless(): unknown {
    return this._motionless;
  }
  set motionless(v: unknown) {
    this._motionless = v;
    log.push(`${this._id}:set_motionless:${renderValue(v)}`);
  }
  get itr_motionless(): unknown {
    return state.itr_motionless;
  }
  get arest(): unknown {
    return undefined;
  }
  set arest(v: unknown) {
    log.push(`${this._id}:set_arest:${renderValue(v)}`);
  }
  get marks(): { has: (k: string) => boolean } {
    return { has: (k: string) => k === "Electrify" && this._marks };
  }
  dataset(key: string): unknown {
    if (key === "electrify_duration") return this._elec;
    const own = this._dataset_values[key];
    if (own !== undefined && own !== null) return own;
    const w = state.dataset[key];
    return w === undefined || w === null ? undefined : w;
  }
  itr_fall(_itr: unknown): unknown {
    return this._itr_fall;
  }
  add_v_rest(c: { vid: string; rest: number }): void {
    log.push(`add_v_rest:${c.vid}:${renderValue(c.rest)}`);
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    this._velx = x;
    this._vely = y;
    this._velz = z;
    log.push(`${this._id}:set_velocity:${renderValue(x)}:${renderValue(y)}:${renderValue(z)}`);
  }
  enter_frame(info: unknown): void {
    log.push(`${this._id}:enter_frame:${renderValue(info)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${renderValue(id)}`);
  }
  play_sound(sounds: unknown): void {
    log.push(`${this._id}:play_sound:${renderValue(sounds)}`);
  }
  spark_point(a: Record<string, unknown>, b: Record<string, unknown>): number[] {
    return [a.left as number, b.right as number, a.top as number];
  }
}

const a = new Fake("A");
const v = new Fake("V");

const buffFake = (): Record<string, unknown> => {
  const buf: Record<string, unknown> = { lifetime: 0, duration: 0, level: 0 };
  buf.set_attacker = (_x: unknown) => undefined;
  buf.set_victim = (_x: unknown) => undefined;
  buf.mount = () => undefined;
  granted = buf;
  return buf;
};

const worldFake = {
  get dataset(): Record<string, unknown> {
    return { ...state.dataset, hp_recoverability: state.recov };
  },
  buffs: { get: (_id: string) => undefined as unknown },
  spark: (x: unknown, y: unknown, z: unknown, type: unknown) => {
    log.push(`spark:${num(x)}:${num(y)}:${num(z)}:${num(type)}`);
  },
  find_entity: (id: string) => {
    if (id === a._id) return a;
    if (id === v._id) return v;
    return undefined;
  },
};

const lfwFake = {
  sounds: {
    play: (s: unknown, x: unknown, y: unknown, z: unknown) => {
      log.push(`snd:${num(s)}:${num(x)}:${num(y)}:${num(z)}`);
    },
  },
  factory: {
    create_buff: (kind: string, _lfw: unknown, id: string) => {
      log.push(`create_buff:${kind}:${id}`);
      return buffFake();
    },
  },
};

for (const f of [a, v]) {
  (f as unknown as Record<string, unknown>).world = worldFake;
  (f as unknown as Record<string, unknown>).lfw = lfwFake;
}

(summary_mgr as unknown as { apply_damage: (a: unknown, i: unknown, v: unknown, p: unknown) => void }).apply_damage =
  (att: unknown, injury: unknown, vic: unknown, prev_hp: unknown) => {
    log.push(
      `summary:${String((att as { id?: unknown })?.id)}:${renderValue(injury)}:${String(
        (vic as { id?: unknown })?.id,
      )}:${renderValue(prev_hp)}`,
    );
  };

const collision = {
  attacker: a,
  victim: v,
  world: worldFake,
  lfw: lfwFake,
  aid: "A",
  vid: "V",
  get itr(): Record<string, unknown> {
    return state.itr;
  },
  get dataset(): Record<string, unknown> {
    return state.dataset;
  },
  get rest(): number {
    return state.rest;
  },
  get a_cube(): Record<string, number> {
    return state.a_cube;
  },
  get b_cube(): Record<string, number> {
    return state.b_cube;
  },
  get bframe(): { state: number } {
    return { state: state.armorwork ? StateEnum.Standing : StateEnum.Injured };
  },
  get aframe(): { state: number } {
    return { state: StateEnum.Standing };
  },
  injury: undefined as unknown,
  injury_r: undefined as unknown,
  real_injury: undefined as unknown,
  real_injury_r: undefined as unknown,
} as Record<string, unknown>;

function sideText(f: Fake): string {
  return (
    `${f._id}.hp=${num(f._hp)} ${f._id}.hp_r=${num(f._hp_r)} ${f._id}.tough=${num(f._toughness)} ` +
    `${f._id}.tough_max=${num(f._tough_max)} ${f._id}.vel=${num(f._velx)}/${num(f._vely)}/${num(f._velz)} ` +
    `${f._id}.team=${num(f._team)} ${f._id}.state=${num(f._state)} ${f._id}.motionless=${num(f._motionless)} ` +
    `${f._id}.shaking=${num(f._shaking)}`
  );
}

function stateText(): string {
  let s = `${sideText(a)} ${sideText(v)}`;
  s += ` inj=${num(collision.injury)} inj_r=${num(collision.injury_r)} rinj=${num(collision.real_injury)} rinj_r=${num(collision.real_injury_r)}`;
  s += ` buff=${
    granted === undefined
      ? "none"
      : `${num(granted.lifetime)}/${num(granted.duration)}/${num(granted.level)}`
  }`;
  return s;
}

function walkSide(f: Fake, field: string, tok: string[], i: number): number {
  const idx = [i];
  if (field === "hp") f._hp = parseValue(tok, idx);
  else if (field === "hp_r") f._hp_r = parseValue(tok, idx);
  else if (field === "fall") f._fall = parseValue(tok, idx);
  else if (field === "fall_max") f._fall_max = parseValue(tok, idx);
  else if (field === "defend") f._defend = parseValue(tok, idx);
  else if (field === "defend_max") f._defend_max = parseValue(tok, idx);
  else if (field === "resting") f._resting = parseValue(tok, idx);
  else if (field === "catch_max") f._catch_time_max = parseValue(tok, idx);
  else if (field === "catching") {
    f._catching = tok[idx[0]!] === "1" ? v : null;
    idx[0] = idx[0]! + 1;
  } else if (field === "catcher") {
    f._catcher = tok[idx[0]!] === "1" ? a : null;
    idx[0] = idx[0]! + 1;
  } else if (field === "marks") {
    f._marks = tok[idx[0]!] === "1";
    idx[0] = idx[0]! + 1;
  } else if (field === "elec_dur") f._elec = parseValue(tok, idx);
  else if (field === "itr_fall") f._itr_fall = parseValue(tok, idx);
  else if (field === "hit_sounds") f._hit_sounds = parseValue(tok, idx);
  else if (field === "motionless") f._motionless = parseValue(tok, idx);
  else if (field === "src_emitter") f.src_emitter = parseValue(tok, idx);
  else if (field === "ice") f._ice = parseValue(tok, idx);
  else if (field === "in_the_sky") f._in_the_sky = parseValue(tok, idx);
  else if (field === "type") f._type = parseValue(tok, idx);
  else if (field === "state") f._state = parseValue(tok, idx);
  else if (field === "base_type") f._base_type = parseValue(tok, idx);
  else if (field === "bearer") {
    f._bearer = tok[idx[0]!] === "1";
    idx[0] = idx[0]! + 1;
  } else if (field === "team") f._team = parseValue(tok, idx);
  else if (field === "armor") f._armor = parseValue(tok, idx);
  else if (field === "tough") f._toughness = Number(parseValue(tok, idx));
  else if (field === "tough_max") f._tough_max = parseValue(tok, idx);
  else if (field === "velx") f._velx = parseValue(tok, idx);
  else if (field === "vely") f._vely = parseValue(tok, idx);
  else if (field === "velz") f._velz = parseValue(tok, idx);
  else if (field === "posx") f._posx = parseValue(tok, idx);
  else if (field === "posy") f._posy = parseValue(tok, idx);
  else if (field === "posz") f._posz = parseValue(tok, idx);
  else if (field === "dataset") f._dataset_values = parseValue(tok, idx) as Record<string, unknown>;
  else {
    process.stderr.write(`unknown side field '${field}'\n`);
    process.exit(2);
  }
  return idx[0]!;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_handlers3.mjs <case-file>\n");
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
      if (sub === "itr") {
        state.itr = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "dataset") {
        state.dataset = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "rest") {
        state.rest = Number(t[i++]!);
      } else if (sub === "recov") {
        state.recov = parseValue(t, idx);
      } else if (sub === "itr_motionless") {
        state.itr_motionless = parseValue(t, idx);
      } else if (sub === "armorwork") {
        state.armorwork = t[i++]! === "1";
      } else if (sub === "acube") {
        state.a_cube = parseValue(t, idx) as Record<string, number>;
      } else if (sub === "bcube") {
        state.b_cube = parseValue(t, idx) as Record<string, number>;
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
      if (name === "whirlwind") {
        handle_itr_kind_whirlwind(collision as never);
      } else if (name === "ballhit_a") {
        handle_ball_is_hit_a(collision as never);
      } else if (name === "ballhit_b") {
        handle_ball_is_hit_b(collision as never);
      } else if (name === "armor") {
        const r = handle_armor(collision as never);
        log.push(`ret:${r ? 1 : 0}`);
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
