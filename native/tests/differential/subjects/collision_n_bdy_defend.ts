import { handle_itr_normal_bdy_defend } from "../../../../src/LFW/collision/handle_itr_normal_bdy_defend";
import { collision_action_handlers } from "../../../../src/LFW/entity/collision_action_handlers";
import { ActionType as AT } from "../../../../src/LFW/defines/actions/ActionType";
import { summary_mgr } from "../../../../src/LFW/entity/SummaryMgr";
import { EntityEnum } from "../../../../src/LFW/defines";
import { num, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const state = {
  dataset: {} as Record<string, unknown>,
  rest: 0,
  itr_motionless: undefined as unknown,
  itr: undefined as Record<string, unknown> | undefined,
  bframe: undefined as Record<string, unknown> | undefined,
  aframe: undefined as Record<string, unknown> | undefined,
  bdy: undefined as Record<string, unknown> | undefined,
};

const iv = { dvx: 0, dvy: 0, dvz: 0 };
const acube = { left: 0, right: 0, bottom: 0, top: 0, near: 0, far: 0 };
const bcube = { left: 0, right: 0, bottom: 0, top: 0, near: 0, far: 0 };

function wsDataset(): Record<string, unknown> {
  return state.dataset;
}

class Fake {
  _id: string;
  _hp: unknown = 50;
  _hp_r: unknown = 50;
  _toughness = 9;
  _toughness_max = 9;
  _armor: unknown = undefined;
  _state: unknown = undefined;
  _face: unknown = 1;
  _team: unknown = 0;
  _base_type: unknown = undefined;
  _bearer = false;
  _type: unknown = EntityEnum.Fighter;
  _vel = [0, 0, 0];
  _pos = [0, 0, 0];
  _ground_y = 0;
  _fall: unknown = 30;
  _fall_max: unknown = 100;
  _defend: unknown = 3;
  _defend_ratio: unknown = undefined;
  _resting: unknown = 0;
  _fire: unknown = undefined;
  _crit: unknown = undefined;
  _dizzy: unknown = undefined;
  _grand_injured: unknown = undefined;
  _injured: unknown = undefined;
  _backhurtact: unknown = undefined;
  _fronthurtact: unknown = undefined;
  _holding: unknown = undefined;
  _ice: unknown = undefined;
  _hit_sounds: unknown = undefined;
  _src_emitter: unknown = undefined;
  _motionless: unknown = undefined;
  _in_the_sky: unknown = undefined;
  _dataset_values: unknown = undefined;
  _elec_dur: unknown = 7;
  _itr_fall: unknown = 7;
  _arest: unknown = undefined;
  _shaking: unknown = undefined;
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
  get toughness_max(): number {
    return this._toughness_max;
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
  get defend_ratio(): unknown {
    return this._defend_ratio ?? wsDataset().defend_ratio;
  }
  get resting(): unknown {
    return this._resting;
  }
  set resting(v: unknown) {
    this._resting = v;
    log.push(`${this._id}:set_resting:${renderValue(v)}`);
  }
  get armor(): unknown {
    return this._armor;
  }
  get state(): unknown {
    return this._state;
  }
  get facing(): unknown {
    return this._face;
  }
  get team(): unknown {
    return this._team;
  }
  set team(v: unknown) {
    this._team = v;
    log.push(`${this._id}:set_team:${renderValue(v)}`);
  }
  get base_type(): unknown {
    return this._base_type;
  }
  get bearer(): unknown {
    return this._bearer ? { id: "bearer" } : null;
  }
  get data(): Record<string, unknown> {
    return {
      type: this._type,
      base: { hit_sounds: this._hit_sounds },
      indexes: {
        fire: this._fire,
        critical_hit: this._crit,
        dizzy: this._dizzy,
        grand_injured: this._grand_injured,
        injured: this._injured,
        ice: this._ice,
      },
    };
  }
  dataset(key: string): unknown {
    if (key === "electrify_duration") return this._elec_dur;
    const d = this._dataset_values as Record<string, unknown> | undefined;
    const own = d === undefined || d === null ? undefined : d[key];
    if (own !== undefined) return own;
    return wsDataset()[key];
  }
  get weight(): number {
    return 1;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: this._pos[0]!, y: this._pos[1]!, z: this._pos[2]! };
  }
  get ground_y(): number {
    return this._ground_y;
  }
  get is_on_ground(): boolean {
    return this._pos[1]! <= this._ground_y;
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
  set motionless(v: unknown) {
    this._motionless = v;
    log.push(`${this._id}:set_motionless:${renderValue(v)}`);
  }
  get itr_motionless(): unknown {
    return state.itr_motionless;
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
  get frame(): Record<string, unknown> {
    return {
      id: "f1",
      cpoint: { backhurtact: this._backhurtact, fronthurtact: this._fronthurtact },
    };
  }
  get falling(): unknown {
    return this._fall;
  }
  get landing_1(): unknown {
    return undefined;
  }
  get landing_2(): unknown {
    return undefined;
  }
  add_v_rest(c: { vid: string; rest: number }): void {
    log.push(`add_v_rest:${c.vid}:${renderValue(c.rest)}`);
  }
  play_sound(sounds: unknown): void {
    log.push(`${this._id}:play_sound:${renderValue(sounds)}`);
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
  get catch_time(): unknown {
    return 0;
  }
  set catch_time(v: unknown) {
    log.push(`${this._id}:set_catch_time:${renderValue(v)}`);
  }
  buffs_set(_k: string, _b: unknown): void {}
  buffs_delete(_k: string): void {}
}

const a = new Fake("A");
const v = new Fake("V");

const world = {
  get dataset(): Record<string, unknown> {
    return wsDataset();
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
  const rec = f as unknown as Record<string, unknown>;
  rec.world = world;
  rec.lfw = {
    sounds: {
      play(s: unknown, x: unknown, y: unknown, z: unknown): void {
        log.push(`snd3:${renderValue(s)}:${renderValue(x)}:${renderValue(y)}:${renderValue(z)}`);
      },
    },
    mt: {
      mark: "",
      pick(): string {
        return "picked";
      },
      range(): number {
        return 0;
      },
    },
  };
}

summary_mgr.apply_damage = ((attacker: Fake, injury: unknown, victim: Fake, prev_hp: unknown) => {
  log.push(`summary:${attacker.id}:${renderValue(injury)}:${victim.id}:${renderValue(prev_hp)}`);
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
  get bframe(): Record<string, unknown> {
    return (state.bframe ?? {}) as Record<string, unknown>;
  },
  get aframe(): Record<string, unknown> {
    return (state.aframe ?? {}) as Record<string, unknown>;
  },
  get bdy(): Record<string, unknown> {
    return (state.bdy ?? {}) as Record<string, unknown>;
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
    `${f._id}.tough=${renderValue(f._toughness)}/${renderValue(f._toughness_max)} ` +
    `${f._id}.armor=${renderValue(f._armor)} ${f._id}.state=${renderValue(f._state)} ` +
    `${f._id}.face=${renderValue(f._face)} ${f._id}.fall=${renderValue(f._fall)} ` +
    `${f._id}.fall_max=${renderValue(f._fall_max)} ${f._id}.defend=${renderValue(f._defend)} ` +
    `${f._id}.resting=${renderValue(f._resting)} ` +
    `${f._id}.vel=${renderValue(f._vel[0])}/${renderValue(f._vel[1])}/${renderValue(f._vel[2])}`
  );
}

function stateText(): string {
  return (
    `${sideText(a)} ${sideText(v)}` +
    ` inj=${renderValue(collision.injury)} inj_r=${renderValue(collision.injury_r)}` +
    ` rinj=${renderValue(collision.real_injury)}`
  );
}

function walkSide(f: Fake, field: string, tok: string[], i: number): number {
  const idx = [i];
  const bump = (): string => {
    const t = tok[idx[0]!]!;
    idx[0] = idx[0]! + 1;
    return t;
  };
  if (field === "hp") f._hp = parseValue(tok, idx);
  else if (field === "hp_r") f._hp_r = parseValue(tok, idx);
  else if (field === "tough") f._toughness = num(bump());
  else if (field === "tough_max") f._toughness_max = num(bump());
  else if (field === "armor") f._armor = parseValue(tok, idx);
  else if (field === "state") f._state = parseValue(tok, idx);
  else if (field === "face") f._face = parseValue(tok, idx);
  else if (field === "team") f._team = parseValue(tok, idx);
  else if (field === "base_type") f._base_type = parseValue(tok, idx);
  else if (field === "bearer") f._bearer = bump() === "1";
  else if (field === "type") f._type = parseValue(tok, idx);
  else if (field === "velx") f._vel[0] = num(bump());
  else if (field === "vely") f._vel[1] = num(bump());
  else if (field === "velz") f._vel[2] = num(bump());
  else if (field === "posx") f._pos[0] = num(bump());
  else if (field === "posy") f._pos[1] = num(bump());
  else if (field === "posz") f._pos[2] = num(bump());
  else if (field === "ground_y") f._ground_y = num(bump());
  else if (field === "fall") f._fall = parseValue(tok, idx);
  else if (field === "fall_max") f._fall_max = parseValue(tok, idx);
  else if (field === "defend") f._defend = parseValue(tok, idx);
  else if (field === "defend_ratio") f._defend_ratio = parseValue(tok, idx);
  else if (field === "itr_fall") f._itr_fall = parseValue(tok, idx);
  else if (field === "resting") f._resting = parseValue(tok, idx);
  else if (field === "fire") f._fire = parseValue(tok, idx);
  else if (field === "crit") f._crit = parseValue(tok, idx);
  else if (field === "dizzy") f._dizzy = parseValue(tok, idx);
  else if (field === "grand_injured") f._grand_injured = parseValue(tok, idx);
  else if (field === "injured") f._injured = parseValue(tok, idx);
  else if (field === "backhurtact") f._backhurtact = parseValue(tok, idx);
  else if (field === "fronthurtact") f._fronthurtact = parseValue(tok, idx);
  else if (field === "holding") f._holding = parseValue(tok, idx);
  else if (field === "ice") f._ice = parseValue(tok, idx);
  else if (field === "hit_sounds") f._hit_sounds = parseValue(tok, idx);
  else if (field === "src_emitter") f._src_emitter = parseValue(tok, idx);
  else if (field === "motionless") f._motionless = parseValue(tok, idx);
  else if (field === "in_the_sky") f._in_the_sky = parseValue(tok, idx);
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
  const dispatchStub =
    (key: string) =>
    (a: unknown, _c: unknown): void => {
      log.push(`dispatch:${key}:${renderValue(a)}`);
    };
  collision_action_handlers[AT.A_NEXT_FRAME] = dispatchStub("A_NEXT_FRAME") as never;
  collision_action_handlers[AT.V_NEXT_FRAME] = dispatchStub("V_NEXT_FRAME") as never;

  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_n_bdy_defend.mjs <case-file>\n");
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
      } else if (sub === "rest") {
        state.rest = Number(t[i++]!);
      } else if (sub === "itr_motionless") {
        state.itr_motionless = parseValue(t, idx);
      } else if (sub === "bframe") {
        state.bframe = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "aframe") {
        state.aframe = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "bdy") {
        state.bdy = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "armorwork") {
        i = idx[0]! + 1;
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
      handle_itr_normal_bdy_defend(collision as never);
      out.push(`run hit || ${log.join(",")} | ${stateText()}`);
      continue;
    }
    process.stderr.write(`unknown op '${op}'\n`);
    process.exit(2);
  }

  process.stdout.write(out.join("\n") + "\n");
}

main();
