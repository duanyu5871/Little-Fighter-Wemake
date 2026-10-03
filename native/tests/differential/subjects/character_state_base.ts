import "../../../../src/LFW/entity/Entity";
import { CharacterState_Base } from "../../../../src/LFW/state/CharacterState_Base";
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
  _dataset: Record<string, unknown> = {};
  _indexes: unknown = undefined;
  _frames: unknown = undefined;
  _onlanding: unknown = undefined;
  _vx: unknown = undefined;
  _vz: unknown = undefined;
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
  get holding(): { base_type: unknown } | undefined {
    return this._holdingBase === undefined ? undefined : { base_type: this._holdingBase };
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
    return { x: 0, y: 0, z: 0 };
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
  const vel = v ? `[${r(v._vx)}:${r(v._vz)}]` : "-";
  return (
    `state=${r(state)} estate=${r(v?._state)} hp=${r(v?._hp)} face=${r(v?._facing)} ` +
    `onground=${v?._onGround ? "1" : "0"} ` +
    `holding=${r(v?.holding?.base_type)} vel=${vel}`
  );
}

function printRun(what: string, ret: unknown): void {
  out.push(`run ${what} || ${log.join(",")} | ret=${ret === undefined ? "-" : r(ret)} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_base.mjs <case-file>\n");
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
      else if (sub === "vstate") {
        if (victim) victim._state = parseValue(t, idx);
      } else if (sub === "victim") victim = ent(String(parseValue(t, idx)));
      else if (sub === "hp") {
        if (victim) victim._hp = parseValue(t, idx);
      } else if (sub === "facing") {
        if (victim) victim._facing = parseValue(t, idx);
      } else if (sub === "onground") {
        if (victim) victim._onGround = num(t[i++]!) !== 0;
      } else if (sub === "holding") {
        if (victim) victim._holdingBase = parseValue(t, idx);
      } else if (sub === "onlanding") {
        if (victim) victim._onlanding = parseValue(t, idx);
      } else if (sub === "dataset") {
        if (victim) victim._dataset = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "indexes") {
        if (victim) victim._indexes = parseValue(t, idx);
      } else if (sub === "frames") {
        if (victim) victim._frames = parseValue(t, idx);
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
        stateObj = new CharacterState_Base(state as never);
        printRun("make", undefined);
      } else if (what === "update") {
        stateObj!.update(victim as never);
        printRun("update", undefined);
      } else if (what === "landing") {
        stateObj!.on_landing!(victim as never, undefined as never);
        printRun("landing", undefined);
      } else if (what === "up") {
        stateObj!.on_leave_ground!(victim as never);
        printRun("up", undefined);
      } else if (what === "auto") {
        const fn = stateObj!.get_auto_frame;
        printRun("auto", fn ? fn(victim as never) : undefined);
      } else if (what === "sudden") {
        const fn = stateObj!.get_sudden_death_frame;
        printRun("sudden", fn ? fn(victim as never) : undefined);
      } else if (what === "caught") {
        const fn = stateObj!.get_caught_end_frame;
        printRun("caught", fn ? fn(victim as never) : undefined);
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
