import "../../../../src/LFW/entity/Entity";
import { CharacterState_Caught } from "../../../../src/LFW/state/CharacterState_Caught";
import { CharacterState_Rowing } from "../../../../src/LFW/state/CharacterState_Rowing";
import type { CharacterState_Base } from "../../../../src/LFW/state/CharacterState_Base";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _dataset: Record<string, unknown> = {};
  _indexes: unknown = undefined;
  _onlanding: unknown = undefined;
  _holdingBase: unknown = undefined;
  _holdingTeam: unknown = undefined;
  _hasHolding = false;
  _vx: unknown = undefined;
  _vy: unknown = undefined;
  _team: unknown = undefined;
  _fallValue: unknown = undefined;
  _fallValueMax: unknown = undefined;
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
  get team(): unknown {
    return this._team;
  }
  get fall_value(): unknown {
    return this._fallValue;
  }
  set fall_value(v: unknown) {
    this._fallValue = v;
    log.push(`${this._id}:set_fall_value:${r(v)}`);
  }
  get fall_value_max(): unknown {
    return this._fallValueMax;
  }
  get holding(): { base_type: unknown; team: unknown } | undefined {
    if (!this._hasHolding) return undefined;
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
  get data(): Record<string, unknown> {
    return { indexes: this._indexes };
  }
  get frame(): Record<string, unknown> {
    return { on_landing: this._onlanding };
  }
  get velocity(): { x: unknown; y: unknown; z: number } {
    return { x: this._vx, y: this._vy, z: 0 };
  }
  get position(): { x: number; y: number; z: number } {
    return { x: 0, y: 0, z: 0 };
  }

  dataset(key: string): unknown {
    log.push(`${this._id}:dataset:${key}`);
    return this._dataset[key];
  }
  drop_holding(): void {
    log.push(`${this._id}:drop_holding`);
  }
  handle_ground_velocity_decay(): void {
    log.push(`${this._id}:handle_ground_velocity_decay`);
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
    if (x !== null && x !== undefined) this._vx = x;
    if (y !== null && y !== undefined) this._vy = y;
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
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
let prevState: unknown = undefined;
let cls = "caught";
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
    `fall=${r(v._fallValue)} fallmax=${r(v._fallValueMax)} ` +
    `vel=[${r(v._vx)}:${r(v._vy)}] hasholding=${v._hasHolding ? "1" : "0"} ` +
    `holding=${r(v._holdingBase)} holding_team=${r(v._holdingTeam)} team=${r(v._team)}`
  );
}

function runMake(): void {
  stateObj = cls === "rowing" ? new CharacterState_Rowing(state as never) : new CharacterState_Caught(state as never);
  out.push(`run make || ${log.join(",")} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_caught_rowing.mjs <case-file>\n");
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
      else if (sub === "prevstate") prevState = parseValue(t, idx);
      else if (sub === "victim") victim = ent(String(parseValue(t, idx)));
      else if (sub === "fall") {
        if (victim) victim._fallValue = parseValue(t, idx);
      } else if (sub === "fallmax") {
        if (victim) victim._fallValueMax = parseValue(t, idx);
      } else if (sub === "vteam") {
        if (victim) victim._team = parseValue(t, idx);
      } else if (sub === "velx") {
        if (victim) victim._vx = parseValue(t, idx);
      } else if (sub === "vely") {
        if (victim) victim._vy = parseValue(t, idx);
      } else if (sub === "has_holding") {
        if (victim) victim._hasHolding = Number(t[i++]!) !== 0;
      } else if (sub === "holding") {
        if (victim) {
          victim._holdingBase = parseValue(t, idx);
          victim._holdingTeam = undefined;
        }
      } else if (sub === "onlanding") {
        if (victim) victim._onlanding = parseValue(t, idx);
      } else if (sub === "dataset") {
        if (victim) victim._dataset = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "indexes") {
        if (victim) victim._indexes = parseValue(t, idx);
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
      } else if (what === "enter") {
        const fn = stateObj!.enter;
        if (fn) fn(victim as never, { state: prevState } as never);
        out.push(`run enter || ${log.join(",")} | ${stateText()}`);
      } else if (what === "update") {
        stateObj!.update(victim as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
      } else if (what === "landing") {
        const fn = stateObj!.on_landing;
        if (fn) fn(victim as never, undefined as never);
        out.push(`run landing || ${log.join(",")} | ${stateText()}`);
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
