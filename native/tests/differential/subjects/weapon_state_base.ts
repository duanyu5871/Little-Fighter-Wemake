import "../../../../src/LFW/entity/Entity";
import { WeaponState_Base } from "../../../../src/LFW/state/WeaponState_Base";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _indexes_flag: unknown = undefined;
  _ionground: unknown = undefined;
  _ithrow: unknown = undefined;
  _isky: unknown = undefined;
  _frames: unknown = undefined;
  _onlanding: unknown = undefined;
  _base: unknown = undefined;
  _wt: unknown = undefined;
  _on_ground = false;
  _dh: unknown = undefined;
  _state: unknown = undefined;
  _fid: unknown = undefined;
  _hp = 0;
  _hpr = 0;
  _align: unknown = undefined;

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get indexes(): unknown {
    const f = this._indexes_flag;
    if (f) {
      return { on_ground: this._ionground, throwings: this._ithrow, in_the_skys: this._isky };
    }
    if (f === null || f === undefined) {
      return { on_ground: undefined, throwings: undefined, in_the_skys: undefined };
    }
    return f;
  }
  get data(): { frames: unknown; indexes: unknown; base: unknown } {
    return { frames: this._frames, indexes: this.indexes, base: this._base };
  }
  get frame(): { on_landing: unknown; id: unknown } {
    return { on_landing: this._onlanding, id: this._fid };
  }
  get base_type(): unknown {
    return this._wt;
  }
  get is_on_ground(): boolean {
    return this._on_ground;
  }
  get drop_hurted(): unknown {
    return this._dh;
  }
  set drop_hurted(v: unknown) {
    this._dh = v;
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
  get state(): unknown {
    return this._state;
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
  leave_ground(): void {
    log.push(`${this._id}:leave_ground`);
  }
  handle_ground_velocity_decay(): void {
    log.push(`${this._id}:handle_ground_velocity_decay`);
  }
  find_align_frame(fid: unknown, throwings: unknown, in_the_skys: unknown): unknown {
    log.push(`${this._id}:find_align_frame:${r(fid)}:${r(throwings)}:${r(in_the_skys)}`);
    return this._align;
  }
}

const ent = new FakeEnt("W1");
let state: unknown = 0;
let velocity: unknown = undefined;
let nf: unknown = undefined;
let obj: WeaponState_Base | undefined = undefined;

function stateText(): string {
  return `hp=${r(ent._hp)} hpr=${r(ent._hpr)} dh=${r(ent._dh)}`;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_weapon_state_base.mjs <case-file>\n");
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
      else if (sub === "indexes") ent._indexes_flag = parseValue(t, idx);
      else if (sub === "ionground") ent._ionground = parseValue(t, idx);
      else if (sub === "ithrow") ent._ithrow = parseValue(t, idx);
      else if (sub === "isky") ent._isky = parseValue(t, idx);
      else if (sub === "frames") ent._frames = parseValue(t, idx);
      else if (sub === "onlanding") ent._onlanding = parseValue(t, idx);
      else if (sub === "base") ent._base = parseValue(t, idx);
      else if (sub === "wt") ent._wt = parseValue(t, idx);
      else if (sub === "onground") ent._on_ground = Boolean(parseValue(t, idx));
      else if (sub === "dh") ent._dh = parseValue(t, idx);
      else if (sub === "vstate") ent._state = parseValue(t, idx);
      else if (sub === "fid") ent._fid = parseValue(t, idx);
      else if (sub === "hp") ent._hp = num(t[i++]!);
      else if (sub === "hpr") ent._hpr = num(t[i++]!);
      else if (sub === "vel") velocity = parseValue(t, idx);
      else if (sub === "nf") nf = parseValue(t, idx);
      else if (sub === "align") ent._align = parseValue(t, idx);
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
        obj = new WeaponState_Base(state as never);
        out.push(`run make || ${log.join(",")} | ${stateText()}`);
      } else if (what === "auto") {
        const fn = obj!.get_auto_frame;
        const f = fn ? fn.call(obj, ent as never) : undefined;
        out.push(`run auto || ${log.join(",")} | fid=${r(f)} | ${stateText()}`);
      } else if (what === "landing") {
        const fn = obj!.on_landing;
        if (fn) fn.call(obj, ent as never, velocity as never);
        out.push(`run landing || ${log.join(",")} | ${stateText()}`);
      } else if (what === "update") {
        obj!.update(ent as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
      } else if (what === "leaveground") {
        const fn = obj!.on_leave_ground;
        if (fn) fn.call(obj, ent as never);
        out.push(`run leaveground || ${log.join(",")} | ${stateText()}`);
      } else if (what === "rebound") {
        obj!.hit_ground_rebouncing(ent as never, nf as never, velocity as never);
        out.push(`run rebound || ${log.join(",")} | ${stateText()}`);
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
