import "../../../../src/LFW/entity/Entity";
import { WeaponState_Base } from "../../../../src/LFW/state/WeaponState_Base";
import { WeaponState_InTheSky } from "../../../../src/LFW/state/WeaponState_InTheSky";
import { WeaponState_OnGround } from "../../../../src/LFW/state/WeaponState_OnGround";
import { WeaponState_OnHand } from "../../../../src/LFW/state/WeaponState_OnHand";
import { WeaponState_Throwing } from "../../../../src/LFW/state/WeaponState_Throwing";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _team: unknown = undefined;
  _new_team: unknown = undefined;
  _motionless: unknown = undefined;
  _has_bearer = false;
  _bmotion: unknown = undefined;
  _dh: unknown = undefined;
  dropping: unknown = undefined;
  _hp = 0;
  _hpr = 0;
  _base: unknown = undefined;
  _wt: unknown = undefined;
  _behavior: unknown = undefined;
  _fid: unknown = undefined;
  _onlanding: unknown = undefined;
  _vstate: unknown = undefined;
  _velx: unknown = undefined;
  _vely: unknown = undefined;
  _velz: unknown = undefined;
  _align: unknown = undefined;
  _indexes_flag: unknown = undefined;
  _throw_on_ground: unknown = undefined;
  _just_on_ground: unknown = undefined;
  _isky: unknown = undefined;
  _ithrow: unknown = undefined;
  _gval: unknown = undefined;

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get team(): unknown {
    return this._team;
  }
  set team(v: unknown) {
    this._team = v;
  }
  get motionless(): unknown {
    return this._motionless;
  }
  get bearer(): { motionless: unknown } | null {
    if (!this._has_bearer) return null;
    const self = this;
    return {
      get motionless(): unknown {
        return self._bmotion;
      },
      set motionless(v: unknown) {
        self._bmotion = v;
      },
    };
  }
  get lfw(): { new_team: unknown } {
    return { new_team: this._new_team };
  }
  get data(): { indexes: unknown; base: unknown } {
    if (!this._indexes_flag) return { indexes: undefined, base: this._base };
    return {
      indexes: {
        throw_on_ground: this._throw_on_ground,
        just_on_ground: this._just_on_ground,
        in_the_skys: this._isky,
        throwings: this._ithrow,
      },
      base: this._base,
    };
  }
  get frame(): { behavior: unknown; on_landing: unknown; id: unknown } {
    return { behavior: this._behavior, on_landing: this._onlanding, id: this._fid };
  }
  get base_type(): unknown {
    return this._wt;
  }
  get velocity(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._velx, y: this._vely, z: this._velz };
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
    return this._vstate;
  }

  dataset(key: string): unknown {
    log.push(`${this._id}:dataset:${key}`);
    return key === "weapon_throwing_gravity" ? this._gval : undefined;
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
let cls = "onground";
let state: unknown = 0;
let velocity: unknown = undefined;
let obj: WeaponState_Base | undefined = undefined;

function stateText(): string {
  const e = ent;
  return (
    `team=${r(e._team)} dh=${r(e._dh)} hp=${r(e._hp)} hpr=${r(e._hpr)} ` +
    `motionless=${r(e._motionless)} bmotion=${r(e._bmotion)} dropping=${r(e.dropping)}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_weapon_state_misc.mjs <case-file>\n");
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
      else if (sub === "newteam") ent._new_team = parseValue(t, idx);
      else if (sub === "team") ent._team = parseValue(t, idx);
      else if (sub === "motionless") ent._motionless = parseValue(t, idx);
      else if (sub === "bearer") ent._has_bearer = Boolean(parseValue(t, idx));
      else if (sub === "bmotion") ent._bmotion = parseValue(t, idx);
      else if (sub === "dh") ent._dh = parseValue(t, idx);
      else if (sub === "dropping") ent.dropping = Boolean(parseValue(t, idx));
      else if (sub === "hp") ent._hp = num(t[i++]!);
      else if (sub === "hpr") ent._hpr = num(t[i++]!);
      else if (sub === "base") ent._base = parseValue(t, idx);
      else if (sub === "wt") ent._wt = parseValue(t, idx);
      else if (sub === "behavior") ent._behavior = parseValue(t, idx);
      else if (sub === "fid") ent._fid = parseValue(t, idx);
      else if (sub === "onlanding") ent._onlanding = parseValue(t, idx);
      else if (sub === "vstate") ent._vstate = parseValue(t, idx);
      else if (sub === "vx") ent._velx = parseValue(t, idx);
      else if (sub === "vy") ent._vely = parseValue(t, idx);
      else if (sub === "vz") ent._velz = parseValue(t, idx);
      else if (sub === "align") ent._align = parseValue(t, idx);
      else if (sub === "indexes") ent._indexes_flag = parseValue(t, idx);
      else if (sub === "throwg") ent._throw_on_ground = parseValue(t, idx);
      else if (sub === "justg") ent._just_on_ground = parseValue(t, idx);
      else if (sub === "isky") ent._isky = parseValue(t, idx);
      else if (sub === "ithrow") ent._ithrow = parseValue(t, idx);
      else if (sub === "gval") ent._gval = parseValue(t, idx);
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
        if (cls === "onhand") obj = new WeaponState_OnHand(state as never);
        else if (cls === "throwing") obj = new WeaponState_Throwing(state as never);
        else if (cls === "inthesky") obj = new WeaponState_InTheSky(state as never);
        else obj = new WeaponState_OnGround(state as never);
        out.push(`run make || ${log.join(",")} | s=${r(obj.state)} | ${stateText()}`);
      } else if (what === "enter") {
        const fn = obj!.enter;
        if (fn) fn.call(obj, ent as never, undefined as never);
        out.push(`run enter || ${log.join(",")} | ${stateText()}`);
      } else if (what === "update") {
        obj!.update(ent as never);
        out.push(`run update || ${log.join(",")} | ${stateText()}`);
      } else if (what === "landing") {
        const fn = obj!.on_landing;
        if (fn) fn.call(obj, ent as never, velocity as never);
        out.push(`run landing || ${log.join(",")} | ${stateText()}`);
      } else if (what === "preupdate") {
        const fn = obj!.pre_update;
        if (fn) fn.call(obj, ent as never);
        out.push(`run preupdate || ${log.join(",")} | ${stateText()}`);
      } else if (what === "gravity") {
        const fn = obj!.get_gravity;
        const result = fn ? fn.call(obj, ent as never) : undefined;
        out.push(`run gravity || ${log.join(",")} | r=${r(result)} | ${stateText()}`);
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
