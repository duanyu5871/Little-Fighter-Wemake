import "../../../../src/LFW/entity/Entity";
import { StateBase_Proxy } from "../../../../src/LFW/state/StateBase_Proxy";
import { State_15 } from "../../../../src/LFW/state/State_15";
import { State_Frozen } from "../../../../src/LFW/state/State_Frozen";
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
  _px = 0;
  _py = 0;
  _pz = 0;
  _pos = { x: 0, y: 0, z: 0 };
  _vx: unknown = undefined;
  _vz: unknown = undefined;
  _buffs = new Map<string, unknown>();
  _lfw = {
    factory: {
      create_buff: (kind: string, _l: unknown, id: string): unknown => {
        log.push(`create_buff:${kind}:${id}`);
        return undefined;
      },
    },
  };

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
  get catcher(): unknown {
    return this._catcher ? this : undefined;
  }
  get holding(): unknown {
    return this._hbtype === undefined ? undefined : { base_type: this._hbtype };
  }
  get is_on_ground(): boolean {
    return this._on_ground;
  }
  get frame(): { on_landing: unknown; id: unknown } {
    return { on_landing: this._onlanding, id: "10" };
  }
  get position(): { x: number; y: number; z: number } {
    return this._pos;
  }
  get velocity(): { x: unknown; z: unknown } {
    return { x: this._vx, z: this._vz };
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
    this._pos.x = x;
    this._pos.y = y;
    this._pos.z = z;
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
  }
  enter_frame_by_id_fallback(id: unknown, fallback: boolean): void {
    log.push(`${this._id}:enter_frame_by_id_fallback:${r(id)}:${fallback ? "1" : "0"}`);
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
  }
  handle_ground_velocity_decay(): void {
    log.push(`${this._id}:handle_ground_velocity_decay`);
  }
  set shaking(v: unknown) {
    log.push(`${this._id}:set_shaking:${r(v)}`);
  }
  set motionless(v: unknown) {
    log.push(`${this._id}:set_motionless:${r(v)}`);
  }
  play_sound(sounds: unknown): void {
    log.push(`${this._id}:play_sound:${r(sounds)}`);
  }
  apply_opoints(opoints: unknown): void {
    const n = Array.isArray(opoints) ? opoints.length : 0;
    log.push(`${this._id}:apply_opoints:${n}`);
  }
  dataset(key: string): unknown {
    log.push(`${this._id}:dataset:${key}`);
    return undefined;
  }
  buffs_set(key: string, _v: unknown): void {
    log.push(`${this._id}:buffs_set:${key}`);
  }
  buffs_delete(key: string): void {
    log.push(`${this._id}:buffs_delete:${key}`);
  }
  set_frame(info: unknown): void {
    log.push(`${this._id}:set_frame:${r((info as Record<string, unknown>)?.id)}`);
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
  set_frame_centery(): void {}
}

const ent = new FakeEnt("E1");
let state: unknown = 0;
let velocity: unknown = undefined;
let rid: unknown = undefined;
let cls = "proxy";
let obj: { state: unknown; update: Function; leave: Function; on_restrict: Function } | undefined =
  undefined;

function stateText(): string {
  const p = ent._pos;
  return `hp=${r(ent._hp)} state=${r(obj!.state)} pos=[${r(p.x)}:${r(p.y)}:${r(p.z)}]`;
}

function retLine(what: string, v: unknown, lg: string[]): string {
  return `run ${what} || ${lg.join(",")} | r=${r(v)} | ${stateText()}`;
}

function callHook(what: string, name: string, args: unknown[]): void {
  const o = obj as unknown as Record<string, unknown>;
  const fn = o[name] as ((...a: unknown[]) => unknown) | undefined;
  const v = fn ? fn.apply(o, args) : undefined;
  out.push(retLine(what, v, log));
}

function callVoid(what: string, name: string, args: unknown[]): void {
  const o = obj as unknown as Record<string, unknown>;
  const fn = o[name] as ((...a: unknown[]) => unknown) | undefined;
  if (fn) fn.apply(o, args);
  out.push(`run ${what} || ${log.join(",")} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_state_base_proxy.mjs <case-file>\n");
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
      else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        ent._pos.x = Number(v.x);
        ent._pos.y = Number(v.y);
        ent._pos.z = Number(v.z);
      } else if (sub === "vx") ent._vx = parseValue(t, idx);
      else if (sub === "vz") ent._vz = parseValue(t, idx);
      else if (sub === "vel") velocity = parseValue(t, idx);
      else if (sub === "rid") rid = parseValue(t, idx);
      else if (sub === "rxyz") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        ent._pos.x = Number(v.x);
        ent._pos.y = Number(v.y);
        ent._pos.z = Number(v.z);
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
        if (cls === "15") obj = new State_15() as never;
        else if (cls === "frozen") obj = new State_Frozen(state as never) as never;
        else obj = new StateBase_Proxy(state as never) as never;
        out.push(`run make || ${log.join(",")} | ${stateText()}`);
      } else if (what === "default") {
        obj = new State_Frozen() as never;
        out.push(`run default || ${log.join(",")} | ${stateText()}`);
      } else if (what === "update") {
        obj!.update(ent as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
      } else if (what === "leave") {
        obj!.leave(ent as never, undefined as never);
        out.push(`run leave || ${log.join(",")} | ${stateText()}`);
      } else if (what === "restrict") {
        obj!.on_restrict(ent as never, ent._pos.x, ent._pos.y, ent._pos.z);
        out.push(`run restrict || ${log.join(",")} | ${stateText()}`);
      } else if (what === "preupdate") {
        callVoid("preupdate", "pre_update", [ent]);
      } else if (what === "enter") {
        callVoid("enter", "enter", [ent, undefined]);
      } else if (what === "dead") {
        callVoid("dead", "on_dead", [ent]);
      } else if (what === "landing") {
        callVoid("landing", "on_landing", [ent, velocity]);
      } else if (what === "leaveground") {
        callVoid("leaveground", "on_leave_ground", [ent]);
      } else if (what === "gravity") {
        callHook("gravity", "get_gravity", [ent]);
      } else if (what === "auto") {
        callHook("auto", "get_auto_frame", [ent]);
      } else if (what === "sdf") {
        callHook("sdf", "get_sudden_death_frame", [ent]);
      } else if (what === "cef") {
        callHook("cef", "get_caught_end_frame", [ent]);
      } else if (what === "ffbi") {
        callHook("ffbi", "find_frame_by_id", [ent, rid]);
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
