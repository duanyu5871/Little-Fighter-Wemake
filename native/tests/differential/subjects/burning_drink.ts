import "../../../../src/LFW/entity/Entity";
import { DrinkInfo } from "../../../../src/LFW/entity/DrinkInfo";
import { CharacterState_Drink } from "../../../../src/LFW/state/CharacterState_Drink";
import { State_Base } from "../../../../src/LFW/state/State_Base";
import { State_Burning } from "../../../../src/LFW/state/State_Burning";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _data: unknown = undefined;
  _indexes: unknown = undefined;
  _frames: unknown = undefined;
  _onlanding: unknown = undefined;
  _wdata: Record<string, unknown> = {};
  _state: unknown = undefined;
  _hbtype: unknown = undefined;
  _catcher = false;
  _on_ground = false;
  _hp = 50;
  _hpr = 0;
  _hpmax: unknown = 50;
  _mp: unknown = 0;
  _mpmax: unknown = 0;
  _px = 0;
  _py = 0;
  _pz = 0;
  _vx: unknown = undefined;
  _vy: unknown = undefined;
  _vz: unknown = undefined;
  _facing: unknown = undefined;
  _bounced: unknown = undefined;
  _has_holding = false;
  _hold_hp: unknown = undefined;
  _hold_hp_r: unknown = undefined;
  _drink: DrinkInfo | undefined = undefined;
  _mtrange: unknown = undefined;
  _buffs = new Map<string, unknown>();
  _lfw = {};

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get data(): unknown {
    const d = (this._data ?? {}) as Record<string, unknown>;
    return { ...d, indexes: this._indexes, frames: this._frames };
  }
  get state(): unknown {
    return this._state;
  }
  get hp(): number {
    return this._hp;
  }
  set hp(v: number) {
    this._hp = v;
  }
  get hp_r(): number {
    return this._hpr;
  }
  set hp_r(v: number) {
    this._hpr = v;
  }
  get hp_max(): unknown {
    return this._hpmax;
  }
  get mp(): unknown {
    return this._mp;
  }
  set mp(v: unknown) {
    this._mp = v;
  }
  get mp_max(): unknown {
    return this._mpmax;
  }
  get facing(): unknown {
    return this._facing;
  }
  set facing(v: unknown) {
    this._facing = v;
  }
  get bounced(): unknown {
    return this._bounced;
  }
  set bounced(v: unknown) {
    this._bounced = v;
  }
  get catcher(): unknown {
    return this._catcher ? this : undefined;
  }
  get holding(): unknown {
    if (!this._has_holding) return null;
    const self = this;
    return {
      get hp(): unknown {
        return self._hold_hp;
      },
      set hp(v: unknown) {
        self._hold_hp = v;
      },
      get hp_r(): unknown {
        return self._hold_hp_r;
      },
      set hp_r(v: unknown) {
        self._hold_hp_r = v;
      },
      drink: this._drink,
      lfw: {
        mt: {
          mark: undefined,
          range: (lo: number, hi: number): unknown => {
            log.push(`${self._id}:holding_mt_range:${r(lo)}:${r(hi)}`);
            return self._mtrange;
          },
        },
      },
      set_velocity(x: unknown, y: unknown, z: unknown): void {
        log.push(`${self._id}:holding_set_velocity:${r(x)}:${r(y)}:${r(z)}`);
      },
    };
  }
  get is_on_ground(): boolean {
    return this._on_ground;
  }
  get frame(): { on_landing: unknown; id: unknown } {
    return { on_landing: this._onlanding, id: "10" };
  }
  get position(): { x: number; y: number; z: number } {
    return { x: this._px, y: this._py, z: this._pz };
  }
  get velocity(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._vx, y: this._vy, z: this._vz };
  }
  get world(): { dataset: Record<string, unknown>; buffs: Map<string, unknown> } {
    const self = this;
    const proxy = new Proxy(self._wdata, {
      get(target, key) {
        log.push(`${self._id}:world_dataset:${String(key)}`);
        return target[String(key)];
      },
    });
    return { dataset: proxy, buffs: self._buffs };
  }
  get lfw(): unknown {
    return this._lfw;
  }

  drop_catching(): void {
    log.push(`${this._id}:catcher_drop_catching`);
  }
  drop_holding(): void {
    log.push(`${this._id}:drop_holding`);
  }
  set_position(x: number, y: number, z: number): void {
    this._px = x;
    this._py = y;
    this._pz = z;
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
  }
  handle_ground_velocity_decay(): void {
    log.push(`${this._id}:handle_ground_velocity_decay`);
  }
  dataset(key: string): unknown {
    log.push(`${this._id}:dataset:${key}`);
    return undefined;
  }
  set shaking(v: unknown) {
    log.push(`${this._id}:set_shaking:${r(v)}`);
  }
  set motionless(v: unknown) {
    log.push(`${this._id}:set_motionless:${r(v)}`);
  }
  buffs_set(key: string, _v: unknown): void {
    log.push(`${this._id}:buffs_set:${key}`);
  }
  buffs_delete(key: string): void {
    log.push(`${this._id}:buffs_delete:${key}`);
  }
}

const ent = new FakeEnt("E1");
let cls = "burning";
let state: unknown = 0;
let velocity: unknown = undefined;
let obj: State_Base | undefined = undefined;

function stateText(): string {
  const e = ent;
  const drink = e._drink ? e._drink.to_snapshot() : undefined;
  return (
    `hp=${r(e._hp)} hpr=${r(e._hpr)} hpmax=${r(e._hpmax)} mp=${r(e._mp)} mpmax=${r(e._mpmax)} ` +
    `state=${r(obj?.state)} bounced=${r(e._bounced)} facing=${r(e._facing)} ` +
    `holding=${e._has_holding ? "1" : "0"} hhp=${r(e._hold_hp)} hhpr=${r(e._hold_hp_r)} ` +
    `drink=${r(drink)} pos=[${r(e._px)}:${r(e._py)}:${r(e._pz)}]`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_burning_drink.mjs <case-file>\n");
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
      else if (sub === "data") ent._data = parseValue(t, idx);
      else if (sub === "indexes") ent._indexes = parseValue(t, idx);
      else if (sub === "frames") ent._frames = parseValue(t, idx);
      else if (sub === "onlanding") ent._onlanding = parseValue(t, idx);
      else if (sub === "wdata") ent._wdata = (parseValue(t, idx) ?? {}) as Record<string, unknown>;
      else if (sub === "vstate") ent._state = parseValue(t, idx);
      else if (sub === "hbtype") ent._hbtype = parseValue(t, idx);
      else if (sub === "catcher") ent._catcher = Boolean(parseValue(t, idx));
      else if (sub === "onground") ent._on_ground = Boolean(parseValue(t, idx));
      else if (sub === "hp") ent._hp = num(t[i++]!);
      else if (sub === "hpr") ent._hpr = num(t[i++]!);
      else if (sub === "hpmax") ent._hpmax = parseValue(t, idx);
      else if (sub === "mp") ent._mp = parseValue(t, idx);
      else if (sub === "mpmax") ent._mpmax = parseValue(t, idx);
      else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        ent._px = Number(v.x);
        ent._py = Number(v.y);
        ent._pz = Number(v.z);
      } else if (sub === "vx") ent._vx = parseValue(t, idx);
      else if (sub === "vy") ent._vy = parseValue(t, idx);
      else if (sub === "vz") ent._vz = parseValue(t, idx);
      else if (sub === "facing") ent._facing = parseValue(t, idx);
      else if (sub === "bounced") ent._bounced = parseValue(t, idx);
      else if (sub === "holding") ent._has_holding = Boolean(parseValue(t, idx));
      else if (sub === "holdhp") ent._hold_hp = parseValue(t, idx);
      else if (sub === "holdhpr") ent._hold_hp_r = parseValue(t, idx);
      else if (sub === "drink") {
        const v = parseValue(t, idx);
        ent._drink = v === undefined ? undefined : new DrinkInfo(v as never);
      } else if (sub === "mtrange") ent._mtrange = parseValue(t, idx);
      else if (sub === "vel") velocity = parseValue(t, idx);
      else {
        process.stderr.write(`unknown env '${sub}'\n`);
        process.exit(2);
      }
      out.push(`env ${sub}`);
      continue;
    }
    if (op === "run") {
      const what = t[i++]!;
      if (what === "make") {
        obj = cls === "drink" ? new CharacterState_Drink(state as never) : new State_Burning(state as never);
        out.push(`run make || ${log.join(",")} | s=${r(obj.state)} | ${stateText()}`);
      } else if (what === "default") {
        obj = cls === "drink" ? new CharacterState_Drink() : new State_Burning();
        out.push(`run default || ${log.join(",")} | s=${r(obj.state)} | ${stateText()}`);
      } else if (what === "enter") {
        const fn = obj!.enter;
        if (fn) fn.call(obj, ent as never, undefined as never);
        out.push(`run enter || ${log.join(",")} | ${stateText()}`);
      } else if (what === "update") {
        obj!.update(ent as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
      } else if (what === "leave") {
        obj!.leave(ent as never, undefined as never);
        out.push(`run leave || ${log.join(",")} | ${stateText()}`);
      } else if (what === "landing") {
        const fn = obj!.on_landing;
        if (fn) fn.call(obj, ent as never, velocity as never);
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
