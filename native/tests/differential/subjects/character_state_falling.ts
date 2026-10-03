import "../../../../src/LFW/entity/Entity";
import { CharacterState_Falling } from "../../../../src/LFW/state/CharacterState_Falling";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeFighter {
  key: string;
  constructor(key: string) {
    this.key = key;
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`E1:ref_set_velocity:${r(this)}:${r(x)}:${r(y)}:${r(z)}`);
  }
}

class FakeEnt {
  _id: string;
  _px = 0;
  _py = 0;
  _pz = 0;
  _dataid: unknown = undefined;
  _frameid: unknown = undefined;
  _onlanding: unknown = undefined;
  _idxbounce: unknown = undefined;
  _idxfalling: unknown = undefined;
  _idxcritical: unknown = undefined;
  _idxlying: unknown = undefined;
  _shaking: unknown = undefined;
  _wait: unknown = undefined;
  _hp: unknown = undefined;
  _facing: unknown = undefined;
  _vx: unknown = undefined;
  _vy = 0;
  _vz: unknown = undefined;
  _bounced: unknown = undefined;
  _catcher = false;
  _fuse: unknown = undefined;
  _fall: unknown = undefined;
  _fallmax: unknown = undefined;
  _defend: unknown = undefined;
  _defendmax: unknown = undefined;
  _rest: unknown = undefined;
  _restmax: unknown = undefined;
  _finj: unknown = undefined;
  _tinj: unknown = undefined;
  _dvals: Record<string, unknown> = {};

  constructor(id: string) {
    this._id = id;
  }

  get id(): string {
    return this._id;
  }
  get position(): { x: number; y: number; z: number } {
    return { x: this._px, y: this._py, z: this._pz };
  }
  set_position(x: number, y: number, z: number): void {
    this._px = x;
    this._py = y;
    this._pz = z;
  }
  get hp(): unknown {
    return this._hp;
  }
  get facing(): unknown {
    return this._facing;
  }
  get wait(): unknown {
    return this._wait;
  }
  get shaking(): unknown {
    return this._shaking;
  }
  get bounced(): unknown {
    return this._bounced;
  }
  set bounced(v: unknown) {
    this._bounced = v;
  }
  get velocity(): { x: unknown; y: unknown; z: unknown } {
    return { x: this._vx, y: this._vy, z: this._vz };
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
  }
  get fall_value(): unknown {
    return this._fall;
  }
  set fall_value(v: unknown) {
    this._fall = v;
  }
  get fall_value_max(): unknown {
    return this._fallmax;
  }
  get defend_value(): unknown {
    return this._defend;
  }
  set defend_value(v: unknown) {
    this._defend = v;
  }
  get defend_value_max(): unknown {
    return this._defendmax;
  }
  get resting(): unknown {
    return this._rest;
  }
  set resting(v: unknown) {
    this._rest = v;
  }
  get resting_max(): unknown {
    return this._restmax;
  }
  get fallinjury(): unknown {
    return this._finj;
  }
  set fallinjury(v: unknown) {
    this._finj = v;
  }
  get throwinjury(): unknown {
    return this._tinj;
  }
  set throwinjury(v: unknown) {
    this._tinj = v;
  }
  get data(): unknown {
    return {
      id: this._dataid,
      indexes: {
        bouncing: this._idxbounce,
        falling: this._idxfalling,
        critical_hit: this._idxcritical,
        lying: this._idxlying,
      },
    };
  }
  get frame(): unknown {
    return { id: this._frameid, on_landing: this._onlanding };
  }
  get ctrl(): unknown {
    const self = this;
    return {
      reset_key_list(): void {
        log.push(`${self._id}:ctrl_reset_key_list`);
      },
    };
  }
  get catcher(): unknown {
    const self = this;
    if (!this._catcher) return undefined;
    return {
      drop_catching(): void {
        log.push(`${self._id}:catcher_drop_catching`);
      },
    };
  }
  get fuse_bys(): unknown {
    if (this._fuse === undefined) return undefined;
    return (this._fuse as unknown[]).map((k) => new FakeFighter(String(k)));
  }
  get world(): { dataset: Record<string, unknown> } {
    const self = this;
    const proxy = new Proxy(self._dvals, {
      get(target, key) {
        log.push(`${self._id}:world_dataset:${String(key)}`);
        return target[String(key)];
      },
    });
    return { dataset: proxy };
  }
  drop_holding(): void {
    log.push(`${this._id}:drop_holding`);
  }
  leave_ground(): void {
    log.push(`${this._id}:leave_ground`);
  }
  dismiss_fusion(frame_id: unknown): void {
    log.push(`${this._id}:dismiss_fusion:${r(frame_id)}`);
  }
  handle_ground_velocity_decay(factor: unknown = 1): void {
    log.push(`${this._id}:handle_ground_velocity_decay:${r(factor)}`);
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
  }
  buffs_set(key: string, _v: unknown): void {
    log.push(`${this._id}:buffs_set:${key}`);
  }
  buffs_delete(key: string): void {
    log.push(`${this._id}:buffs_delete:${key}`);
  }
}

const ent = new FakeEnt("E1");
let state: unknown = 0;
let velocity: unknown = undefined;
let obj: CharacterState_Falling | undefined = undefined;

function stateText(): string {
  const e = ent;
  return (
    `pos=[${r(e._px)},${r(e._py)},${r(e._pz)}] ` +
    `vel=[${r(e._vx)},${r(e._vy)},${r(e._vz)}] ` +
    `dataid=${r(e._dataid)} frameid=${r(e._frameid)} hp=${r(e._hp)} facing=${r(e._facing)} ` +
    `bounced=${r(e._bounced)} fall=${r(e._fall)}/${r(e._fallmax)} ` +
    `defend=${r(e._defend)}/${r(e._defendmax)} rest=${r(e._rest)}/${r(e._restmax)} ` +
    `finj=${r(e._finj)} tinj=${r(e._tinj)}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_falling.mjs <case-file>\n");
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
      else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        ent._px = Number(v.x);
        ent._py = Number(v.y);
        ent._pz = Number(v.z);
      } else if (sub === "dataid") ent._dataid = parseValue(t, idx);
      else if (sub === "frameid") ent._frameid = parseValue(t, idx);
      else if (sub === "onlanding") ent._onlanding = parseValue(t, idx);
      else if (sub === "idxbounce") ent._idxbounce = parseValue(t, idx);
      else if (sub === "idxfalling") ent._idxfalling = parseValue(t, idx);
      else if (sub === "idxcritical") ent._idxcritical = parseValue(t, idx);
      else if (sub === "idxlying") ent._idxlying = parseValue(t, idx);
      else if (sub === "shaking") ent._shaking = parseValue(t, idx);
      else if (sub === "wait") ent._wait = parseValue(t, idx);
      else if (sub === "hp") ent._hp = parseValue(t, idx);
      else if (sub === "facing") ent._facing = parseValue(t, idx);
      else if (sub === "vx") ent._vx = parseValue(t, idx);
      else if (sub === "vy") ent._vy = num(t[i++]!);
      else if (sub === "vz") ent._vz = parseValue(t, idx);
      else if (sub === "bounced") ent._bounced = parseValue(t, idx);
      else if (sub === "catcher") ent._catcher = Boolean(parseValue(t, idx));
      else if (sub === "fuse") ent._fuse = parseValue(t, idx);
      else if (sub === "fall") ent._fall = parseValue(t, idx);
      else if (sub === "fallmax") ent._fallmax = parseValue(t, idx);
      else if (sub === "defend") ent._defend = parseValue(t, idx);
      else if (sub === "defmax") ent._defendmax = parseValue(t, idx);
      else if (sub === "rest") ent._rest = parseValue(t, idx);
      else if (sub === "restmax") ent._restmax = parseValue(t, idx);
      else if (sub === "finj") ent._finj = parseValue(t, idx);
      else if (sub === "tinj") ent._tinj = parseValue(t, idx);
      else if (sub === "dvals") ent._dvals = (parseValue(t, idx) ?? {}) as Record<string, unknown>;
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
        obj = new CharacterState_Falling(state as never);
        out.push(`run make || ${log.join(",")} | s=${r(obj.state)} | ${stateText()}`);
      } else if (what === "default") {
        obj = new CharacterState_Falling();
        out.push(`run default || ${log.join(",")} | s=${r(obj.state)} | ${stateText()}`);
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
      } else if (what === "leave") {
        obj!.leave(ent as never, undefined as never);
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
