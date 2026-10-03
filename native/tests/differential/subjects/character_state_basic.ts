import "../../../../src/LFW/entity/Entity";
import { CharacterState_Injured } from "../../../../src/LFW/state/CharacterState_Injured";
import { CharacterState_Running } from "../../../../src/LFW/state/CharacterState_Running";
import { CharacterState_Standing } from "../../../../src/LFW/state/CharacterState_Standing";
import type { CharacterState_Base } from "../../../../src/LFW/state/CharacterState_Base";
import { StateEnum } from "../../../../src/LFW/defines";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _state: unknown = undefined;
  _hp: unknown = undefined;
  _facing: unknown = undefined;
  _holdingBase: unknown = undefined;
  _holdingTeam: unknown = undefined;
  _dataset: Record<string, unknown> = {};
  _indexes: unknown = undefined;
  _frames: unknown = undefined;
  _onlanding: unknown = undefined;
  _vx: unknown = undefined;
  _vz: unknown = undefined;
  _team: unknown = undefined;
  _groundY: unknown = undefined;
  _px = 0;
  _py = 0;
  _pz = 0;
  _onGround = false;

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get state(): unknown {
    return this._state;
  }
  get hp(): unknown {
    return this._hp;
  }
  get facing(): unknown {
    return this._facing;
  }
  get team(): unknown {
    return this._team;
  }
  get ground_y(): unknown {
    return this._groundY;
  }
  get holding(): { base_type: unknown; team: unknown } | undefined {
    if (this._holdingBase === undefined) return undefined;
    const self = this;
    return {
      get base_type() {
        return self._holdingBase;
      },
      get team() {
        return self._holdingTeam;
      },
      set team(v: unknown) {
        self._holdingTeam = v;
        log.push(`${self._id}:holding_set_team:${r(v)}`);
      },
    };
  }
  get is_on_ground(): boolean {
    return this._onGround;
  }
  get data(): Record<string, unknown> {
    return { indexes: this._indexes, frames: this._frames };
  }
  get frame(): Record<string, unknown> {
    return { on_landing: this._onlanding };
  }
  get position(): { x: number; y: number; z: number } {
    return { x: this._px, y: this._py, z: this._pz };
  }
  get velocity(): { x: unknown; y: number; z: unknown } {
    return { x: this._vx, y: 0, z: this._vz };
  }
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

  dataset(key: string): unknown {
    log.push(`${this._id}:dataset:${key}`);
    return this._dataset[key];
  }
  get_sudden_death_frame(): unknown {
    log.push(`${this._id}:get_sudden_death_frame`);
    return { id: "SD" };
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
    if (x !== null && x !== undefined) this._vx = x;
    if (z !== null && z !== undefined) this._vz = z;
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
  }
  drop_holding(): void {
    log.push(`${this._id}:drop_holding`);
  }
  handle_ground_velocity_decay(): void {
    log.push(`${this._id}:handle_ground_velocity_decay`);
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
let state: unknown = StateEnum.Standing;
let cls = "standing";
let useDefault = false;
let stateObj: CharacterState_Base | undefined = undefined;

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
    `state=${r(v._state)} hp=${r(v._hp)} ground=${r(v._groundY)} ` +
    `pos=[${r(v._px)}:${r(v._py)}:${r(v._pz)}] vel=[${r(v._vx)}:${r(v._vz)}] ` +
    `holding=${r(v._holdingBase)} holding_team=${r(v._holdingTeam)} team=${r(v._team)}`
  );
}

function runMake(): void {
  if (cls === "running") {
    stateObj = useDefault ? new CharacterState_Running() : new CharacterState_Running(state as never);
  } else if (cls === "injured") {
    stateObj = useDefault ? new CharacterState_Injured() : new CharacterState_Injured(state as never);
  } else {
    stateObj = useDefault
      ? new CharacterState_Standing()
      : new CharacterState_Standing(state as never);
  }
  out.push(`run make || ${log.join(",")} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_basic.mjs <case-file>\n");
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
      if (sub === "cls") cls = String(parseValue(t, idx));
      else if (sub === "state") state = parseValue(t, idx);
      else if (sub === "usedefault") useDefault = num(t[i++]!) !== 0;
      else if (sub === "victim") victim = ent(String(parseValue(t, idx)));
      else if (sub === "hp") {
        if (victim) victim._hp = parseValue(t, idx);
      } else if (sub === "vstate") {
        if (victim) victim._state = parseValue(t, idx);
      } else if (sub === "facing") {
        if (victim) victim._facing = parseValue(t, idx);
      } else if (sub === "vteam") {
        if (victim) victim._team = parseValue(t, idx);
      } else if (sub === "onground") {
        if (victim) victim._onGround = num(t[i++]!) !== 0;
      } else if (sub === "ground_y") {
        if (victim) victim._groundY = parseValue(t, idx);
      } else if (sub === "holding") {
        const hv = parseValue(t, idx);
        if (victim) {
          victim._holdingBase = hv;
          victim._holdingTeam = undefined;
        }
      } else if (sub === "onlanding") {
        if (victim) victim._onlanding = parseValue(t, idx);
      } else if (sub === "dataset") {
        if (victim) victim._dataset = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "indexes") {
        if (victim) victim._indexes = parseValue(t, idx);
      } else if (sub === "frames") {
        if (victim) victim._frames = parseValue(t, idx);
      } else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        if (victim) {
          victim._px = Number(v.x);
          victim._py = Number(v.y);
          victim._pz = Number(v.z);
        }
      } else if (sub === "velx") {
        if (victim) victim._vx = parseValue(t, idx);
      } else if (sub === "velz") {
        if (victim) victim._vz = parseValue(t, idx);
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
        runMake();
      } else if (what === "update") {
        stateObj!.update(victim as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
      } else if (what === "enter") {
        const fn = stateObj!.enter;
        if (fn) fn(victim as never, undefined as never);
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
