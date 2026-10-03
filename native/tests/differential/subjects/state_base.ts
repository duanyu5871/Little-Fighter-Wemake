import { Buff_Healing } from "../../../../src/LFW/buff/Buff_Healing";
import { State_Base } from "../../../../src/LFW/state/State_Base";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _dataset: Record<string, unknown> = {};
  _vx: unknown = undefined;
  _vz: unknown = undefined;
  position = { x: 0, y: 0, z: 0 };
  marks = new Map<string, unknown>();
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
  get lfw(): unknown {
    return lfwFake;
  }
  get world(): unknown {
    return worldFake;
  }
  get velocity(): { x: unknown; y: number; z: unknown } {
    return { x: this._vx, y: 0, z: this._vz };
  }
  get frame(): Record<string, unknown> {
    return { centery: 0, height: 0, pic: undefined };
  }
  dataset(key: string): unknown {
    log.push(`${this._id}:dataset:${key}`);
    return this._dataset[key];
  }
  set_mark(key: unknown, value: unknown, prev?: unknown): void {
    log.push(`${this._id}:set_mark:${String(key)}:${r(value)}`);
    const k = String(key);
    if (prev === undefined || this.marks.get(k) === prev) this.marks.set(k, value);
  }
  del_mark(key: unknown, value: unknown): void {
    log.push(`${this._id}:del_mark:${String(key)}:${r(value)}`);
    const k = String(key);
    if (value === undefined || this.marks.get(k) === value) this.marks.delete(k);
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
    if (x !== null && x !== undefined) this._vx = x;
    if (z !== null && z !== undefined) this._vz = z;
  }
  set_position(x: number, y: number, z: number): void {
    log.push(`${this._id}:set_position:${r(x)}:${r(y)}:${r(z)}`);
  }
  set_frame(info: unknown): void {
    log.push(`${this._id}:set_frame:${r((info as Record<string, unknown>)?.id)}`);
  }
  enter_frame_by_id(id: unknown): void {
    log.push(`${this._id}:enter_frame_by_id:${r(id)}`);
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
let created: { id: string; duration: number } | undefined = undefined;
let state: unknown = 0;
let stateObj: State_Base | undefined = undefined;

function findEnt(id: string): FakeEnt | undefined {
  return ents.find((e) => e._id === id);
}

function ent(id: string): FakeEnt {
  const e = findEnt(id);
  if (e) return e;
  const createdEnt = new FakeEnt(id);
  ents.push(createdEnt);
  return createdEnt;
}

const worldFake = {
  find_entity: (id: string) => findEnt(id),
  buffs: {
    get: (_id: string) => undefined as unknown,
    set: (id: string, _b: unknown) => {
      log.push(`world_buffs_set:${id}`);
      return undefined;
    },
    delete: (_id: string) => true,
  },
};

const lfwFake = {
  world: worldFake,
  datas: { find: (_oid: string) => undefined as unknown },
  factory: {
    create_entity: (_w: unknown, _d: unknown) => undefined as unknown,
    create_buff: (kind: string, lfw: unknown, id: string) => {
      log.push(`create_buff:${kind}:${id}`);
      const b = new Buff_Healing(lfw as never, id, kind);
      created = b as unknown as { id: string; duration: number };
      return b;
    },
  },
};

function stateText(): string {
  const pos = victim ? `[${r(victim.position.x)}:${r(victim.position.y)}:${r(victim.position.z)}]` : "-";
  const vel = victim ? `[${r(victim._vx)}:${r(victim._vz)}]` : "-";
  const granted = created ? `${created.id}:${r(created.duration)}` : "-";
  const marks = `[${victim ? [...victim.marks.entries()].map(([k, v]) => `${k}=${r(v)}`).join(",") : ""}]`;
  return `state=${r(state)} pos=${pos} vel=${vel} granted=${granted} marks=${marks}`;
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_state_base.mjs <case-file>\n");
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
      else if (sub === "dataset") {
        if (victim) victim._dataset = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "pos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        if (victim) victim.position = { x: Number(v.x), y: Number(v.y), z: Number(v.z) };
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
        stateObj = new State_Base(state as never);
        created = undefined;
        out.push(`run make || ${log.join(",")} | ${stateText()}`);
      } else if (what === "leave") {
        stateObj!.leave(victim as never, undefined as never);
        out.push(`run leave || ${log.join(",")} | ${stateText()}`);
      } else if (what === "restrict") {
        const x = num(t[i++]!);
        const y = num(t[i++]!);
        const z = num(t[i++]!);
        stateObj!.on_restrict(victim as never, x, y, z);
        out.push(`run restrict || ${log.join(",")} | ${stateText()}`);
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
