import { handle_ball_hit_other } from "../../../../src/LFW/collision/handle_ball_hit_other";
import { handle_weapon_hit_other } from "../../../../src/LFW/collision/handle_weapon_hit_other";
import { EntityEnum } from "../../../../src/LFW/defines";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

function num(v: unknown): string {
  return renderValue(v);
}

const state = {
  dataset: {} as Record<string, unknown>,
  rest: 0,
  itr_motionless: undefined as unknown,
};

class Fake {
  _id: string;
  _hp: unknown = 20;
  _hp_r: unknown = 30;
  _state: unknown = undefined;
  _facing: unknown = 1;
  _base_type: unknown = undefined;
  _velx: unknown = 0;
  _vely: unknown = 0;
  _velz: unknown = 0;
  _frame_id: unknown = "f1";
  _throwings: unknown = undefined;
  _in_the_skys: unknown = undefined;
  _arest: unknown = undefined;
  _dropping = false;
  _hit_sounds: unknown = undefined;
  _type: unknown = EntityEnum.Fighter;
  _motionless: unknown = undefined;
  _shaking: unknown = undefined;

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
  get state(): unknown {
    return this._state;
  }
  get facing(): unknown {
    return this._facing;
  }
  get base_type(): unknown {
    return this._base_type;
  }
  get velocity(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._velx, y: this._vely, z: this._velz };
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    this._velx = x;
    this._vely = y;
    this._velz = z;
    log.push(`${this._id}:set_velocity:${renderValue(x)}:${renderValue(y)}:${renderValue(z)}`);
  }
  get frame(): { id: unknown } {
    return { id: this._frame_id };
  }
  get data(): Record<string, unknown> {
    return {
      type: this._type,
      base: { hit_sounds: this._hit_sounds },
      indexes: { throwings: this._throwings, in_the_skys: this._in_the_skys },
    };
  }
  get arest(): unknown {
    return this._arest;
  }
  set arest(v: unknown) {
    this._arest = v;
    log.push(`${this._id}:set_arest:${renderValue(v)}`);
  }
  set dropping(v: unknown) {
    this._dropping = !!v;
    log.push(`${this._id}:set_dropping:${v ? "1" : "0"}`);
  }
  get dropping(): boolean {
    return this._dropping;
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
  enter_frame(info: unknown): void {
    log.push(`${this._id}:enter_frame:${renderValue(info)}`);
  }
  play_sound(sounds: unknown): void {
    log.push(`${this._id}:play_sound:${renderValue(sounds)}`);
  }
  add_v_rest(c: { vid: string; rest: number }): void {
    log.push(`add_v_rest:${c.vid}:${renderValue(c.rest)}`);
  }
  find_align_frame(frame_id: unknown, throwings: unknown, in_the_skys: unknown): unknown {
    log.push(`faf:${renderValue(frame_id)}:${renderValue(throwings)}:${renderValue(in_the_skys)}`);
    return "faf_result";
  }
}

const a = new Fake("A");
const v = new Fake("V");

const worldFake = {
  get dataset(): Record<string, unknown> {
    return state.dataset;
  },
};

for (const f of [a, v]) {
  (f as unknown as Record<string, unknown>).world = worldFake;
}

const collision = {
  attacker: a,
  victim: v,
  world: worldFake,
  aid: "A",
  vid: "V",
  get itr(): Record<string, unknown> {
    return state.itr;
  },
  get bdy(): Record<string, unknown> {
    return state.bdy;
  },
  get aframe(): Record<string, unknown> {
    return state.aframe;
  },
  get dataset(): Record<string, unknown> {
    return state.dataset;
  },
  get rest(): number {
    return state.rest;
  },
} as Record<string, unknown>;

function sideText(f: Fake): string {
  return (
    `${f._id}.hp=${num(f._hp)} ${f._id}.hp_r=${num(f._hp_r)} ${f._id}.state=${num(f._state)} ` +
    `${f._id}.facing=${num(f._facing)} ${f._id}.base_type=${num(f._base_type)} ` +
    `${f._id}.vel=${num(f._velx)}/${num(f._vely)}/${num(f._velz)} ${f._id}.arest=${num(f._arest)} ` +
    `${f._id}.dropping=${f._dropping ? 1 : 0} ${f._id}.frame_id=${num(f._frame_id)}`
  );
}

function stateText(): string {
  return `${sideText(a)} ${sideText(v)}`;
}

function walkSide(f: Fake, field: string, tok: string[], i: number): number {
  const idx = [i];
  if (field === "hp") f._hp = parseValue(tok, idx);
  else if (field === "hp_r") f._hp_r = parseValue(tok, idx);
  else if (field === "state") f._state = parseValue(tok, idx);
  else if (field === "facing") f._facing = parseValue(tok, idx);
  else if (field === "base_type") f._base_type = parseValue(tok, idx);
  else if (field === "velx") f._velx = parseValue(tok, idx);
  else if (field === "vely") f._vely = parseValue(tok, idx);
  else if (field === "velz") f._velz = parseValue(tok, idx);
  else if (field === "frame_id") f._frame_id = parseValue(tok, idx);
  else if (field === "throwings") f._throwings = parseValue(tok, idx);
  else if (field === "in_the_sky") f._in_the_skys = parseValue(tok, idx);
  else if (field === "arest") f._arest = parseValue(tok, idx);
  else if (field === "dropping") {
    f._dropping = tok[idx[0]!] === "1";
    idx[0] = idx[0]! + 1;
  } else if (field === "hit_sounds") f._hit_sounds = parseValue(tok, idx);
  else if (field === "type") f._type = parseValue(tok, idx);
  else {
    process.stderr.write(`unknown side field '${field}'\n`);
    process.exit(2);
  }
  return idx[0]!;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_collision_handlers4.mjs <case-file>\n");
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
      } else if (sub === "bdy") {
        state.bdy = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "aframe") {
        state.aframe = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "dataset") {
        state.dataset = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "rest") {
        state.rest = Number(t[i++]!);
      } else if (sub === "itr_motionless") {
        state.itr_motionless = parseValue(t, idx);
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
      if (name === "ballhitother") {
        handle_ball_hit_other(collision as never);
      } else if (name === "weaponhitother") {
        handle_weapon_hit_other(collision as never);
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
