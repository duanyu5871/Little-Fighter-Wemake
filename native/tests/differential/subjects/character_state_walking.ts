import "../../../../src/LFW/entity/Entity";
import { CharacterState_Walking } from "../../../../src/LFW/state/CharacterState_Walking";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _hp: unknown = undefined;
  _waitValue: unknown = undefined;
  _frameInfo: unknown = undefined;
  _holding: { base_type?: unknown } | undefined = undefined;
  _indexes: unknown = undefined;
  _groundY: unknown = undefined;
  _px = 0;
  _py = 0;
  _pz = 0;
  _waitFlag = 0;
  _ctrlUd = 0;
  _ctrlLr = 0;
  _holdingIsWeapon = false;
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
  get hp(): unknown {
    return this._hp;
  }
  get wait(): unknown {
    return this._waitValue;
  }
  set wait(v: unknown) {
    this._waitValue = v;
    log.push(`${this._id}:set_wait:${r(v)}`);
  }
  get ground_y(): unknown {
    return this._groundY;
  }
  get holding(): { base_type?: unknown; data: { type: number } } | undefined {
    return this._holding === undefined
      ? undefined
      : {
          base_type: this._holding.base_type,
          data: { type: this._holdingIsWeapon ? 16 : 0 },
        };
  }
  get ctrl(): { UD: boolean; LR: boolean } {
    return { UD: this._ctrlUd, LR: this._ctrlLr };
  }
  get data(): Record<string, unknown> {
    return { indexes: this._indexes, frames: undefined };
  }
  get frame(): Record<string, unknown> {
    return this._frameInfo as Record<string, unknown>;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: this._px, y: this._py, z: this._pz };
  }

  handle_wait_flag(wait: unknown, frame: unknown): number {
    log.push(`${this._id}:handle_wait_flag:${r(wait)}:${r(frame)}`);
    return this._waitFlag;
  }
  get_sudden_death_frame(): unknown {
    log.push(`${this._id}:get_sudden_death_frame`);
    return { id: "SD" };
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown, fallback = false): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}:${fallback ? "1" : "0"}`);
  }
  handle_ground_velocity_decay(): void {
    log.push(`${this._id}:handle_ground_velocity_decay`);
  }
  set_velocity(_x: unknown, _y: unknown, _z: unknown): void {}
  drop_holding(): void {
    log.push(`${this._id}:drop_holding`);
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
let stateObj: CharacterState_Walking | undefined = undefined;

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

function isWeaponHolding(v: FakeEnt): boolean {
  return v._holdingIsWeapon;
}

function stateText(): string {
  const v = victim!;
  return (
    `hp=${r(v._hp)} wait=${r(v._waitValue)} waitflag=${r(v._waitFlag)} frame=${r(v._frameInfo)} ` +
    `pos=[${r(v._px)}:${r(v._py)}:${r(v._pz)}] ground=${r(v._groundY)} ` +
    `holding=${r(v._holding?.base_type)} hweapon=${isWeaponHolding(v) ? "1" : "0"} ` +
    `ctrl=${v._ctrlUd ? "1" : "0"}${v._ctrlLr ? "1" : "0"}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_walking.mjs <case-file>\n");
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
      else if (sub === "victim") victim = ent(String(parseValue(t, idx)));
      else if (sub === "hp") {
        if (victim) victim._hp = parseValue(t, idx);
      } else if (sub === "vwait") {
        if (victim) victim._waitValue = parseValue(t, idx);
      } else if (sub === "waitflag") {
        if (victim) victim._waitFlag = num(t[i++]!);
      } else if (sub === "frame") {
        if (victim) victim._frameInfo = parseValue(t, idx);
      } else if (sub === "ground_y") {
        if (victim) victim._groundY = parseValue(t, idx);
      } else if (sub === "indexes") {
        if (victim) victim._indexes = parseValue(t, idx);
      } else if (sub === "ctrlud") {
        if (victim) victim._ctrlUd = num(t[i++]!);
      } else if (sub === "ctrllr") {
        if (victim) victim._ctrlLr = num(t[i++]!);
      } else if (sub === "hweapon") {
        if (victim) victim._holdingIsWeapon = num(t[i++]!) !== 0;
      } else if (sub === "holding") {
        const hv = parseValue(t, idx);
        if (victim) victim._holding = hv === undefined ? undefined : { base_type: hv };
      } else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        if (victim) {
          victim._px = Number(v.x);
          victim._py = Number(v.y);
          victim._pz = Number(v.z);
        }
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
        stateObj = new CharacterState_Walking(state as never);
        out.push(`run make || ${log.join(",")} | ${stateText()}`);
      } else if (what === "update") {
        stateObj!.update(victim as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
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
