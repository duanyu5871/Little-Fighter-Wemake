import "../../../../src/LFW/entity/Entity";
import { CharacterState_Dash } from "../../../../src/LFW/state/CharacterState_Dash";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _dataset: Record<string, unknown> = {};
  _facing: unknown = undefined;
  _vx: unknown = undefined;
  _vy: unknown = undefined;
  _vz: unknown = undefined;
  _groundY: unknown = undefined;
  _px = 0;
  _py = 0;
  _pz = 0;
  _ctrlUd = 0;
  _ctrlLr = 0;
  buffs = {
    set: (k: string, _v: unknown) => {
      log.push(`${this._id}:buffs_set:${k}`);
      return this.buffs;
    },
    delete: (k: string) => {
      log.push(`${this._id}:buffs_delete:${k}`);
      return true;
    },
  };

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get facing(): unknown {
    return this._facing;
  }
  get ground_y(): unknown {
    return this._groundY;
  }
  get ctrl(): { UD: number; LR: number } {
    return { UD: this._ctrlUd, LR: this._ctrlLr };
  }
  get position(): { x: number; y: number; z: number } {
    return { x: this._px, y: this._py, z: this._pz };
  }
  get velocity(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._vx, y: this._vy, z: this._vz };
  }

  dataset(key: string): unknown {
    log.push(`${this._id}:dataset:${key}`);
    return this._dataset[key];
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
    if (x !== null && x !== undefined) this._vx = x;
    if (y !== null && y !== undefined) this._vy = y;
    if (z !== null && z !== undefined) this._vz = z;
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
  }
  set_position(x: number, y: number, z: number): void {
    this._px = x;
    this._py = y;
    this._pz = z;
  }
  set_frame(info: unknown): void {
    log.push(`${this._id}:set_frame:${r((info as Record<string, unknown>)?.id)}`);
  }
  attach(on: boolean): void {
    log.push(`${this._id}:attach:${on ? "1" : "0"}`);
  }
  set outline_alpha(v: unknown) {
    log.push(`${this._id}:outline_alpha:${r(v)}`);
  }
  set outline_width(v: unknown) {
    log.push(`${this._id}:outline_width:${r(v)}`);
  }
  set outline_color(v: unknown) {
    log.push(`${this._id}:outline_color:${r(v)}`);
  }
}

const ents: FakeEnt[] = [];
let victim: FakeEnt | undefined = undefined;
let state: unknown = 0;
let prevState: unknown = undefined;
let stateObj: CharacterState_Dash | undefined = undefined;

function findEnt(id: string): FakeEnt | undefined {
  return ents.find((e) => e._id === id);
}

function ent(id: string): FakeEnt {
  const e = findEnt(id);
  if (e) return e;
  const created = new FakeEnt(id);
  ents.push(created);
  return created;
}

function stateText(): string {
  const v = victim!;
  return (
    `pos=[${r(v._px)}:${r(v._py)}:${r(v._pz)}] ground=${r(v._groundY)} ` +
    `vel=[${r(v._vx)}:${r(v._vy)}:${r(v._vz)}] face=${r(v._facing)} ` +
    `ctrl=${r(v._ctrlUd)},${r(v._ctrlLr)}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_dash.mjs <case-file>\n");
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
      if (sub === "state") state = parseValue(t, idx);
      else if (sub === "prevstate") prevState = parseValue(t, idx);
      else if (sub === "victim") victim = ent(String(parseValue(t, idx)));
      else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        if (victim) {
          victim._px = Number(v.x);
          victim._py = Number(v.y);
          victim._pz = Number(v.z);
        }
      } else if (sub === "ground_y") {
        if (victim) victim._groundY = parseValue(t, idx);
      } else if (sub === "velx") {
        if (victim) victim._vx = parseValue(t, idx);
      } else if (sub === "vely") {
        if (victim) victim._vy = parseValue(t, idx);
      } else if (sub === "velz") {
        if (victim) victim._vz = parseValue(t, idx);
      } else if (sub === "facing") {
        if (victim) victim._facing = parseValue(t, idx);
      } else if (sub === "ctrlud") {
        if (victim) victim._ctrlUd = num(t[i++]!);
      } else if (sub === "ctrllr") {
        if (victim) victim._ctrlLr = num(t[i++]!);
      } else if (sub === "dataset") {
        if (victim) victim._dataset = parseValue(t, idx) as Record<string, unknown>;
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
        stateObj = new CharacterState_Dash(state as never);
        out.push(`run make || ${log.join(",")} | ${stateText()}`);
      } else if (what === "enter") {
        const fn = stateObj!.enter;
        if (fn) fn(victim as never, { state: prevState } as never);
        out.push(`run enter || ${log.join(",")} | ${stateText()}`);
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
