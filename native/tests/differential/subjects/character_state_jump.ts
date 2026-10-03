import "../../../../src/LFW/entity/Entity";
import { CharacterState_Jump } from "../../../../src/LFW/state/CharacterState_Jump";
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
  _jx: unknown = undefined;
  _jy: unknown = undefined;
  _jz: unknown = undefined;
  _jt: unknown = undefined;
  _bot = false;
  _held = "";
  _lr = 0;
  _ud = 0;
  _jumpflag: unknown = undefined;
  _dvals: Record<string, unknown> = {};
  _onlanding: unknown = undefined;
  _landing1: unknown = undefined;

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
  get jumping(): { x: unknown; y: unknown; z: unknown; t: unknown } {
    const self = this;
    return {
      get x(): unknown {
        return self._jx;
      },
      set x(v: unknown) {
        self._jx = v;
      },
      get y(): unknown {
        return self._jy;
      },
      set y(v: unknown) {
        self._jy = v;
      },
      get z(): unknown {
        return self._jz;
      },
      set z(v: unknown) {
        self._jz = v;
      },
      get t(): unknown {
        return self._jt;
      },
      set t(v: unknown) {
        self._jt = v;
      },
    };
  }
  get ctrl(): unknown {
    const self = this;
    return {
      __is_bot_ctrl__: this._bot ? true : undefined,
      get LR(): number {
        return self._lr;
      },
      get UD(): number {
        return self._ud;
      },
      is_end(key: string): boolean {
        log.push(`${self._id}:ctrl_is_end:${key}`);
        return !self._held.includes(key);
      },
    };
  }
  get data(): unknown {
    return { indexes: { landing_1: this._landing1 } };
  }
  get frame(): { on_landing: unknown } {
    return { on_landing: this._onlanding };
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

  get_prev_frame(): unknown {
    log.push(`${this._id}:prev_frame`);
    return { jump_flag: this._jumpflag };
  }
  dataset(key: string): unknown {
    log.push(`${this._id}:dataset:${key}`);
    return this._dvals?.[key];
  }
  enter_frame(frame: unknown): void {
    log.push(`${this._id}:enter_frame:${r(frame)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
  }
  update_velocity(v: unknown): void {
    log.push(`${this._id}:update_velocity:${r(v)}`);
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
  }
  handle_ground_velocity_decay(): void {
    log.push(`${this._id}:handle_ground_velocity_decay`);
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
let obj: CharacterState_Jump | undefined = undefined;

function stateText(): string {
  const e = ent;
  return (
    `pos=[${r(e._px)}:${r(e._py)}:${r(e._pz)}] gy=${r(e._gy)} ` +
    `jx=${r(e._jx)} jy=${r(e._jy)} jz=${r(e._jz)} jt=${r(e._jt)}`
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_character_state_jump.mjs <case-file>\n");
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
      } else if (sub === "gy") ent._gy = parseValue(t, idx);
      else if (sub === "jx") ent._jx = parseValue(t, idx);
      else if (sub === "jy") ent._jy = parseValue(t, idx);
      else if (sub === "jz") ent._jz = parseValue(t, idx);
      else if (sub === "jt") ent._jt = parseValue(t, idx);
      else if (sub === "bot") ent._bot = Boolean(parseValue(t, idx));
      else if (sub === "held") ent._held = String(parseValue(t, idx));
      else if (sub === "lr") ent._lr = num(t[i++]!);
      else if (sub === "ud") ent._ud = num(t[i++]!);
      else if (sub === "jumpflag") ent._jumpflag = parseValue(t, idx);
      else if (sub === "dvals") ent._dvals = (parseValue(t, idx) ?? {}) as Record<string, unknown>;
      else if (sub === "onlanding") ent._onlanding = parseValue(t, idx);
      else if (sub === "landing1") ent._landing1 = parseValue(t, idx);
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
        obj = new CharacterState_Jump(state as never);
        out.push(`run make || ${log.join(",")} | s=${r(obj.state)} | ${stateText()}`);
      } else if (what === "default") {
        obj = new CharacterState_Jump();
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
