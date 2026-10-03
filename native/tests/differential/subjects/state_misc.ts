import "../../../../src/LFW/entity/Entity";
import { BallState_Base } from "../../../../src/LFW/state/BallState_Base";
import { CharacterState_TransformToLouisEX } from "../../../../src/LFW/state/CharacterState_Transform2LouisEX";
import { State_TransformTo8XXX } from "../../../../src/LFW/state/State_TransformTo8XXX";
import { State_TransformToCatching } from "../../../../src/LFW/state/State_TransformToCatching";
import { State_WeaponBroken } from "../../../../src/LFW/state/State_WeaponBroken";
import type { State_Base } from "../../../../src/LFW/state/State_Base";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _state: unknown = undefined;
  _data: Record<string, unknown> = {};
  _findData: unknown = undefined;
  _findFighter: unknown = undefined;
  _transformType: unknown = undefined;
  _shaking: unknown = undefined;
  _motionless: unknown = undefined;
  _vx: unknown = undefined;
  _vy: unknown = undefined;
  _vz: unknown = undefined;
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
  get state(): unknown {
    return this._state;
  }
  get data(): Record<string, unknown> {
    return this._data;
  }
  get shaking(): unknown {
    return this._shaking;
  }
  set shaking(v: unknown) {
    this._shaking = v;
    log.push(`${this._id}:set_shaking:${r(v)}`);
  }
  get motionless(): unknown {
    return this._motionless;
  }
  set motionless(v: unknown) {
    this._motionless = v;
    log.push(`${this._id}:set_motionless:${r(v)}`);
  }
  get position(): { x: number; y: number; z: number } {
    return { x: 0, y: 0, z: 0 };
  }
  get velocity(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._vx, y: this._vy, z: this._vz };
  }
  get lfw(): { datas: { find: unknown; find_fighter: unknown } } {
    return {
      datas: {
        find: (oid: string) => {
          log.push(`${this._id}:datas_find:${oid}`);
          return this._findData;
        },
        find_fighter: (oid: string) => {
          log.push(`${this._id}:datas_find_fighter:${oid}`);
          return this._findFighter;
        },
      },
    };
  }
  get world(): { callbacks: { call: unknown } } {
    return {
      callbacks: {
        call: (name: string) => {
          log.push(`${this._id}:world_callbacks_call:${name}`);
        },
      },
    };
  }

  transform(data: unknown): void {
    log.push(`${this._id}:transform:${r(data)}`);
    this._data = { type: this._transformType };
  }
  find_auto_frame(): unknown {
    log.push(`${this._id}:find_auto_frame`);
    return { id: "AUTO" };
  }
  transfrom_to_another(): void {
    log.push(`${this._id}:transfrom_to_another`);
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
  drop_holding(): void {
    log.push(`${this._id}:drop_holding`);
  }
  set_position(_x: number, _y: number, _z: number): void {}
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
let cls = "weapon_broken";
let stateObj: State_Base | undefined = undefined;

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
    `state=${r(state)} vstate=${r(v._state)} type=${r(v._data.type)} ` +
    `shaking=${r(v._shaking)} motionless=${r(v._motionless)} ` +
    `vel=[${r(v._vx)}:${r(v._vy)}:${r(v._vz)}]`
  );
}

function runMake(): void {
  const ctor =
    cls === "to_catching"
      ? State_TransformToCatching
      : cls === "to_louisex"
        ? CharacterState_TransformToLouisEX
        : cls === "to_8xxx"
          ? State_TransformTo8XXX
          : cls === "ball"
            ? BallState_Base
            : State_WeaponBroken;
  stateObj = new ctor(state as never);
  out.push(`run make || ${log.join(",")} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_state_misc.mjs <case-file>\n");
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
      else if (sub === "victim") victim = ent(String(parseValue(t, idx)));
      else if (sub === "vstate") {
        if (victim) victim._state = parseValue(t, idx);
      } else if (sub === "vtype") {
        if (victim) victim._data = { type: parseValue(t, idx) };
      } else if (sub === "finddata") {
        if (victim) victim._findData = parseValue(t, idx);
      } else if (sub === "findfighter") {
        if (victim) victim._findFighter = parseValue(t, idx);
      } else if (sub === "transformtype") {
        if (victim) victim._transformType = parseValue(t, idx);
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
      } else if (what === "landing") {
        const fn = stateObj!.on_landing;
        if (fn) fn(victim as never, undefined as never);
        out.push(`run landing || ${log.join(",")} | ${stateText()}`);
      } else if (what === "update") {
        stateObj!.update(victim as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
      } else if (what === "enter") {
        const fn = stateObj!.enter;
        if (fn) fn(victim as never, undefined as never);
        out.push(`run enter || ${log.join(",")} | ${stateText()}`);
      } else if (what === "leave") {
        stateObj!.leave(victim as never, undefined as never);
        out.push(`run leave || ${log.join(",")} | ${stateText()}`);
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
