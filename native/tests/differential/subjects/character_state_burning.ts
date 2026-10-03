import "../../../../src/LFW/entity/Entity";
import { CharacterState_Burning } from "../../../../src/LFW/state/CharacterState_Burning";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _indexes: unknown = undefined;
  _wdata: Record<string, unknown> = {};
  _facing: unknown = undefined;
  _bounced: unknown = undefined;
  _vx: unknown = undefined;
  _vy: unknown = undefined;
  _vz: unknown = undefined;
  _onlanding: unknown = undefined;
  _catcher = false;
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
  set facing(v: unknown) {
    this._facing = v;
    log.push(`${this._id}:set_facing:${r(v)}`);
  }
  get bounced(): unknown {
    return this._bounced;
  }
  set bounced(v: unknown) {
    this._bounced = v;
    log.push(`${this._id}:set_bounced:${r(v)}`);
  }
  get catcher(): { drop_catching: () => void } | undefined {
    if (!this._catcher) return undefined;
    const self = this;
    return {
      drop_catching: () => {
        log.push(`${self._id}:catcher_drop_catching`);
      },
    };
  }
  get data(): Record<string, unknown> {
    return { indexes: this._indexes };
  }
  get frame(): Record<string, unknown> {
    return { on_landing: this._onlanding };
  }
  get position(): { x: number; y: number; z: number } {
    return { x: 0, y: 0, z: 0 };
  }
  get velocity(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._vx, y: this._vy, z: this._vz };
  }
  get world(): { dataset: Record<string, unknown> } {
    const self = this;
    return {
      dataset: new Proxy(self._wdata, {
        get(target, key: string) {
          log.push(`${self._id}:world_dataset:${key}`);
          return target[key];
        },
      }),
    };
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
  handle_ground_velocity_decay(): void {
    log.push(`${this._id}:handle_ground_velocity_decay`);
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
let velocity: Record<string, unknown> = {};
let stateObj: CharacterState_Burning | undefined = undefined;

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
    `bounced=${r(v._bounced)} facing=${r(v._facing)} ` +
    `vel=[${r(v._vx)}:${r(v._vy)}:${r(v._vz)}] catcher=${v._catcher ? "1" : "0"}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_burning.mjs <case-file>\n");
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
      if (sub === "victim") victim = ent(String(parseValue(t, idx)));
      else if (sub === "indexes") {
        if (victim) victim._indexes = parseValue(t, idx);
      } else if (sub === "wdata") {
        if (victim) victim._wdata = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "facing") {
        if (victim) victim._facing = parseValue(t, idx);
      } else if (sub === "bounced") {
        if (victim) victim._bounced = parseValue(t, idx);
      } else if (sub === "velx") {
        if (victim) victim._vx = parseValue(t, idx);
      } else if (sub === "vely") {
        if (victim) victim._vy = parseValue(t, idx);
      } else if (sub === "velz") {
        if (victim) victim._vz = parseValue(t, idx);
      } else if (sub === "landingvel") {
        velocity = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "catcher") {
        if (victim) victim._catcher = Number(t[i++]!) !== 0;
      } else if (sub === "onlanding") {
        if (victim) victim._onlanding = parseValue(t, idx);
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
        stateObj = new CharacterState_Burning();
        out.push(`run make || ${log.join(",")} | ${stateText()}`);
      } else if (what === "enter") {
        const fn = stateObj!.enter;
        if (fn) fn(victim as never, undefined as never);
        out.push(`run enter || ${log.join(",")} | ${stateText()}`);
      } else if (what === "update") {
        stateObj!.update(victim as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
      } else if (what === "leave") {
        stateObj!.leave(victim as never, undefined as never);
        out.push(`run leave || ${log.join(",")} | ${stateText()}`);
      } else if (what === "landing") {
        const fn = stateObj!.on_landing;
        if (fn) fn(victim as never, velocity as never);
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
