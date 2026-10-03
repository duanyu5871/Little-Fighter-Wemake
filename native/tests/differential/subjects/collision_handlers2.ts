import { handle_injury } from "../../../../src/LFW/collision/handle_injury";
import { handle_itr_catch } from "../../../../src/LFW/collision/handle_itr_catch";
import {
  handle_itr_effect_freeze,
  handle_itr_kind_freeze,
} from "../../../../src/LFW/collision/handle_itr_kind_freeze";
import { handle_john_shield_hit_other_ball } from "../../../../src/LFW/collision/handle_john_shield_hit_other_ball";
import { Ditto } from "../../../../src/LFW/ditto";
import { summary_mgr } from "../../../../src/LFW/entity/SummaryMgr";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

function num(v: unknown): string {
  return renderValue(v);
}

const state = {
  recov: 0.25 as unknown,
  is_fighter: true,
  vel: 0 as unknown,
  itr_motionless: undefined as unknown,
  itr: {} as Record<string, unknown>,
  dataset: {} as Record<string, unknown>,
  rest: 0,
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
  _weight: unknown = 1;
  _state: unknown = undefined;
  _facing: unknown = 1;
  _on_ground = true;
  _dataset_values: Record<string, unknown> = {};
  _type: unknown = 8;
  _velocity = { x: 0, y: 0, z: 0 };
  _toughness = 9;
  src_emitter: unknown = undefined;

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: 0, y: 0, z: 0 };
  }
  get data(): Record<string, unknown> {
    return {
      type: this._type,
      indexes: { ice: this._ice },
      base: { hit_sounds: this._hit_sounds },
    };
  }
  get weight(): unknown {
    return this._weight;
  }
  get state(): unknown {
    return this._state;
  }
  get facing(): unknown {
    return this._facing;
  }
  get is_on_ground(): boolean {
    return this._on_ground;
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
  set toughness(v: number) {
    this._toughness = v;
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
    return this._defend_max;
  }
  get resting(): unknown {
    return this._resting;
  }
  set resting(v: unknown) {
    this._resting = v;
    log.push(`${this._id}:set_resting:${renderValue(v)}`);
  }
  get catch_time(): unknown {
    return this._catch_time;
  }
  set catch_time(v: unknown) {
    this._catch_time = v;
    log.push(`${this._id}:set_catch_time:${renderValue(v)}`);
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
  get marks(): { has: (k: string) => boolean } {
    return { has: (k: string) => k === "Electrify" && this._marks };
  }
  get arest(): unknown {
    return undefined;
  }
  set arest(v: unknown) {
    log.push(`${this._id}:set_arest:${renderValue(v)}`);
  }
  dataset(key: string): unknown {
    if (key === "electrify_duration") return this._elec;
    return this._dataset_values[key];
  }
  itr_fall(_itr: unknown): unknown {
    return this._itr_fall;
  }
  add_v_rest(c: { vid: string; rest: number }): void {
    log.push(`add_v_rest:${c.vid}:${renderValue(c.rest)}`);
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    this._velocity = { x: x as number, y: y as number, z: z as number };
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
  find_entity: (id: string) => {
    if (id === a._id) return a;
    if (id === v._id) return v;
    return undefined;
  },
};

const lfwFake = {
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

(Ditto as unknown as { warn: (m: string) => void }).warn = (m: string) => {
  log.push(`warn:${m}`);
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
  injury: undefined as unknown,
  injury_r: undefined as unknown,
  real_injury: undefined as unknown,
  real_injury_r: undefined as unknown,
} as Record<string, unknown>;

function sideText(f: Fake): string {
  return (
    `${f._id}.hp=${num(f._hp)} ${f._id}.hp_r=${num(f._hp_r)} ${f._id}.fall=${num(f._fall)} ` +
    `${f._id}.defend=${num(f._defend)} ${f._id}.resting=${num(f._resting)} ${f._id}.tough=${num(f._toughness)} ` +
    `${f._id}.catch_time=${num(f._catch_time)} ${f._id}.catching=${String((f._catching as { id?: unknown } | null | undefined)?.id ?? "-")} ` +
    `${f._id}.catcher=${String((f._catcher as { id?: unknown } | null | undefined)?.id ?? "-")} ` +
    `${f._id}.shaking=${num(f._shaking)} ${f._id}.motionless=${num(f._motionless)} ` +
    `${f._id}.vel=${num(f._velocity.x)}/${num(f._velocity.y)}/${num(f._velocity.z)}`
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

function walkSide(f: Fake, field: string, tok: string[], idx: number[]): void {
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
  else if (field === "type") f._type = parseValue(tok, idx);
  else if (field === "weight") f._weight = parseValue(tok, idx);
  else if (field === "state") f._state = parseValue(tok, idx);
  else if (field === "face") f._facing = parseValue(tok, idx);
  else if (field === "dataset") f._dataset_values = parseValue(tok, idx) as Record<string, unknown>;
  else {
    process.stderr.write(`unknown side field '${field}'\n`);
    process.exit(2);
  }
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_handlers2.mjs <case-file>\n");
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
      } else if (sub === "fighter") {
        i = i + 1;
      } else if (sub === "vel") {
        state.vel = parseValue(t, idx);
      } else if (sub === "velz") {
        parseValue(t, idx);
      } else if (sub === "itr_motionless") {
        state.itr_motionless = parseValue(t, idx);
      } else if (sub === "a" || sub === "v") {
        const f = sub === "a" ? a : v;
        const field = t[i++]!;
        walkSide(f, field, t, [i]);
      } else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      const name = t[i++]!;
      if (name === "injury") {
        const scale = i < t.length ? Number(t[i++]!) : 1;
        const keep = i < t.length ? t[i++] === "1" : false;
        handle_injury(collision as never, scale, keep);
      } else if (name === "catch") {
        handle_itr_catch(collision as never);
      } else if (name === "freeze") {
        handle_itr_kind_freeze(collision as never);
      } else if (name === "efreeze") {
        handle_itr_effect_freeze(collision as never);
      } else if (name === "shield") {
        handle_john_shield_hit_other_ball(collision as never);
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
