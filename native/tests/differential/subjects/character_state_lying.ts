import "../../../../src/LFW/entity/Entity";
import { CharacterState_Lying } from "../../../../src/LFW/state/CharacterState_Lying";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _px = 0;
  _py = 0;
  _pz = 0;
  _gy: unknown = undefined;
  _frameid: unknown = undefined;
  _hp: unknown = undefined;
  _hpr: unknown = undefined;
  _hpmax: unknown = undefined;
  _holding = false;
  _holdtype: unknown = undefined;
  _holdteam: unknown = undefined;
  _team: unknown = undefined;
  _tough: unknown = undefined;
  _tmax: unknown = undefined;
  _trest: unknown = undefined;
  _la: unknown = undefined;
  _ld: unknown = undefined;
  _lc: unknown = undefined;
  _wait: unknown = undefined;
  _held = "";
  _dvals: Record<string, unknown> = {};
  _deadjoin: unknown = undefined;
  _deadgone: unknown = undefined;
  _reserve: unknown = undefined;
  _wakeup: unknown = undefined;
  _motionless: unknown = undefined;
  _invul: unknown = undefined;
  _blink: unknown = undefined;
  _outline = "";
  _puppets: unknown = undefined;
  _state: unknown = 0;

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
  get ground_y(): unknown {
    return this._gy;
  }
  get frame(): unknown {
    return { id: this._frameid };
  }
  get hp(): unknown {
    return this._hp;
  }
  set hp(v: unknown) {
    this._hp = v;
  }
  get hp_r(): unknown {
    return this._hpr;
  }
  set hp_r(v: unknown) {
    this._hpr = v;
  }
  get hp_max(): unknown {
    return this._hpmax;
  }
  set hp_max(v: unknown) {
    this._hpmax = v;
  }
  get team(): unknown {
    return this._team;
  }
  set team(v: unknown) {
    this._team = v;
  }
  get state(): unknown {
    return this._state;
  }
  get wait(): unknown {
    return this._wait;
  }
  set wait(v: unknown) {
    this._wait = v;
  }
  get holding(): unknown {
    const self = this;
    if (!this._holding) return undefined;
    return {
      base_type: self._holdtype,
      get team(): unknown {
        return self._holdteam;
      },
      set team(v: unknown) {
        log.push(`${self._id}:holding_set_team:${r(v)}`);
        self._holdteam = v;
      },
    };
  }
  drop_holding(): void {
    log.push(`${this._id}:drop_holding`);
  }
  get toughness(): unknown {
    return this._tough;
  }
  set toughness(v: unknown) {
    this._tough = v;
  }
  get toughness_max(): unknown {
    return this._tmax;
  }
  get toughness_resting(): unknown {
    return this._trest;
  }
  set toughness_resting(v: unknown) {
    this._trest = v;
  }
  get lying_a_count(): unknown {
    return this._la;
  }
  set lying_a_count(v: unknown) {
    this._la = v;
  }
  get lying_d_count(): unknown {
    return this._ld;
  }
  set lying_d_count(v: unknown) {
    this._ld = v;
  }
  get lying_c_count(): unknown {
    return this._lc;
  }
  set lying_c_count(v: unknown) {
    this._lc = v;
  }
  get ctrl(): unknown {
    const self = this;
    return {
      reset_key_list(): void {
        log.push(`${self._id}:ctrl_reset_key_list`);
      },
      is_end(key: string): boolean {
        log.push(`${self._id}:ctrl_is_end:${key}`);
        return !self._held.includes(key);
      },
    };
  }
  get dead_join(): unknown {
    return this._deadjoin;
  }
  set dead_join(v: unknown) {
    this._deadjoin = v;
  }
  get dead_gone(): unknown {
    return this._deadgone;
  }
  get reserve(): unknown {
    return this._reserve;
  }
  set reserve(v: unknown) {
    this._reserve = v;
  }
  get wakeup_invuln(): unknown {
    return this._wakeup;
  }
  set wakeup_invuln(v: unknown) {
    this._wakeup = v;
  }
  get invulnerable(): unknown {
    return this._invul;
  }
  set invulnerable(v: unknown) {
    this._invul = v;
  }
  get blinking(): unknown {
    return this._blink;
  }
  set blinking(v: unknown) {
    this._blink = v;
  }
  get motionless(): unknown {
    return this._motionless;
  }
  set motionless(v: unknown) {
    this._motionless = v;
  }
  get outline_color(): string {
    return this._outline;
  }
  set outline_color(v: string) {
    this._outline = v;
  }
  blink_and_respawn(duration: unknown): void {
    log.push(`${this._id}:blink_and_respawn:${r(duration)}`);
  }
  blink_and_gone(duration: unknown): void {
    log.push(`${this._id}:blink_and_gone:${r(duration)}`);
  }
  get world(): unknown {
    const self = this;
    const proxy = new Proxy(self._dvals, {
      get(target, key) {
        log.push(`${self._id}:world_dataset:${String(key)}`);
        return target[String(key)];
      },
    });
    const puppets = new Map<string, unknown>();
    const teams = Array.isArray(self._puppets) ? (self._puppets as unknown[]) : [];
    teams.forEach((team, i) => puppets.set(`P${i}`, { team }));
    return { dataset: proxy, puppets };
  }
  get lfw(): unknown {
    return { world: { etc: (x: unknown, y: unknown, z: unknown, kind: unknown) => {
      log.push(`${this._id}:world_etc:${r(x)}:${r(y)}:${r(z)}:${r(kind)}`);
    } } };
  }
  handle_ground_velocity_decay(factor: unknown = 1): void {
    log.push(`${this._id}:handle_ground_velocity_decay:${r(factor)}`);
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
let obj: CharacterState_Lying | undefined = undefined;

function stateText(): string {
  const e = ent;
  return (
    `pos=[${r(e._px)},${r(e._py)},${r(e._pz)}] ` +
    `hp=${r(e._hp)}/${r(e._hpr)}/${r(e._hpmax)} team=${r(e._team)} ` +
    `tough=${r(e._tough)}/${r(e._tmax)} trest=${r(e._trest)} ` +
    `la=${r(e._la)} ld=${r(e._ld)} lc=${r(e._lc)} wait=${r(e._wait)} ` +
    `holding=${r(e._holding)} holdteam=${r(e._holdteam)} ` +
    `deadjoin=${r(e._deadjoin)} deadgone=${r(e._deadgone)} reserve=${r(e._reserve)} ` +
    `wakeup=${r(e._wakeup)} motionless=${r(e._motionless)} invul=${r(e._invul)} ` +
    `blink=${r(e._blink)} outline=${r(e._outline)} frameid=${r(e._frameid)} gy=${r(e._gy)}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_lying.mjs <case-file>\n");
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
      else if (sub === "estate") ent._state = parseValue(t, idx);
      else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        ent._px = Number(v.x);
        ent._py = Number(v.y);
        ent._pz = Number(v.z);
      } else if (sub === "gy") ent._gy = parseValue(t, idx);
      else if (sub === "frameid") ent._frameid = parseValue(t, idx);
      else if (sub === "hp") ent._hp = parseValue(t, idx);
      else if (sub === "hpr") ent._hpr = parseValue(t, idx);
      else if (sub === "hpmax") ent._hpmax = parseValue(t, idx);
      else if (sub === "holding") ent._holding = Boolean(parseValue(t, idx));
      else if (sub === "holdtype") ent._holdtype = parseValue(t, idx);
      else if (sub === "holdteam") ent._holdteam = parseValue(t, idx);
      else if (sub === "team") ent._team = parseValue(t, idx);
      else if (sub === "tough") ent._tough = parseValue(t, idx);
      else if (sub === "tmax") ent._tmax = parseValue(t, idx);
      else if (sub === "trest") ent._trest = parseValue(t, idx);
      else if (sub === "la") ent._la = parseValue(t, idx);
      else if (sub === "ld") ent._ld = parseValue(t, idx);
      else if (sub === "lc") ent._lc = parseValue(t, idx);
      else if (sub === "wait") ent._wait = parseValue(t, idx);
      else if (sub === "held") ent._held = String(parseValue(t, idx));
      else if (sub === "dvals") ent._dvals = (parseValue(t, idx) ?? {}) as Record<string, unknown>;
      else if (sub === "deadjoin") ent._deadjoin = parseValue(t, idx);
      else if (sub === "deadgone") ent._deadgone = parseValue(t, idx);
      else if (sub === "reserve") ent._reserve = parseValue(t, idx);
      else if (sub === "wakeup") ent._wakeup = parseValue(t, idx);
      else if (sub === "motionless") ent._motionless = parseValue(t, idx);
      else if (sub === "invul") ent._invul = parseValue(t, idx);
      else if (sub === "blink") ent._blink = parseValue(t, idx);
      else if (sub === "outline") ent._outline = String(parseValue(t, idx));
      else if (sub === "puppets") ent._puppets = parseValue(t, idx);
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
        obj = new CharacterState_Lying(state as never);
        out.push(`run make || ${log.join(",")} | s=${r(obj.state)} | ${stateText()}`);
      } else if (what === "default") {
        obj = new CharacterState_Lying();
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
      } else if (what === "dead") {
        const fn = obj!.on_dead;
        if (fn) fn.call(obj, ent as never);
        out.push(`run dead || ${log.join(",")} | ${stateText()}`);
      } else if (what === "findframe") {
        const fn = obj!.find_frame_by_id;
        const res = fn ? fn.call(obj, ent as never, undefined as never) : undefined;
        out.push(`run findframe || ${log.join(",")} | r=${r(res)} | ${stateText()}`);
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
