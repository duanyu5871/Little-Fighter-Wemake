import { Buff_Electrify } from "../../../../src/LFW/buff/Buff_Electrify";
import { Buff_GroupAttack } from "../../../../src/LFW/buff/Buff_GroupAttack";
import { parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  px = 0;
  py = 0;
  pz = 0;
  centery = 0;
  height = 0;
  pic_h = 0;
  marks = new Map<string, string>();
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
  get position(): { x: number; y: number; z: number } {
    return { x: this.px, y: this.py, z: this.pz };
  }
  get frame(): Record<string, unknown> {
    return {
      centery: this.centery,
      height: this.height,
      pic: this.pic_h ? { h: this.pic_h } : undefined,
    };
  }
  set_position(x: number, y: number, z: number): void {
    this.px = x;
    this.py = y;
    this.pz = z;
    log.push(`${this._id}:set_position:${r(x)}:${r(y)}:${r(z)}`);
  }
  set_frame(info: unknown): void {
    log.push(`${this._id}:set_frame:${r((info as Record<string, unknown>)?.id)}`);
  }
  set_mark(key: string, value: string, prev?: string): boolean {
    if (prev === undefined || this.marks.get(key) === prev) {
      log.push(`${this._id}:set_mark:${key}:${value}`);
      this.marks.set(key, value);
      return true;
    }
    return false;
  }
  del_mark(key: string, value?: string): boolean {
    if (value === undefined || this.marks.get(key) === value) {
      const ok = this.marks.delete(key);
      log.push(ok ? `${this._id}:del_mark:${key}` : `${this._id}:del_mark_miss:${key}`);
      return ok;
    }
    return false;
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
const victims: FakeEnt[] = [];
let effect: FakeEnt | undefined = undefined;
let buff: { update: (d: number) => void; mount: () => void; unmount: () => void } | undefined;
let buff_id = "B1";
let kind = "GroupAttack";
let cls = "group_attack";

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
  datas: {
    find: (oid: string) => {
      if (!oid) return undefined;
      log.push(`find_data:${oid}`);
      return { id: oid };
    },
  },
  factory: {
    create_entity: (_w: unknown, _d: unknown) => {
      if (!effect) {
        effect = new FakeEnt("fx1");
        ents.push(effect);
      }
      return effect;
    },
  },
};

function marksText(): string {
  return `[${ents
    .map((e) => `${e._id}{${[...e.marks.entries()].map(([k, v]) => `${k}=${v}`).join(",")}}`)
    .join(",")}]`;
}

function fxText(): string {
  if (!effect) return "none";
  return `${r(effect.px)}/${r(effect.py)}/${r(effect.pz)}`;
}

function stateText(): string {
  return (
    `id=${buff_id} ` +
    `victims=[${victims.map((v) => v._id).join(",")}] marks=${marksText()} fx=${fxText()}`
  );
}

function runMake(): void {
  buff =
    cls === "electrify"
      ? new Buff_Electrify(lfwFake as never, buff_id, kind)
      : new Buff_GroupAttack(lfwFake as never, buff_id, kind);
  victims.forEach((v, idx) => {
    if (idx === 0) (buff as { set_victim: (x: FakeEnt) => void }).set_victim(v);
    else (buff as { add_victim: (x: FakeEnt) => void }).add_victim(v);
  });
  out.push(`run make || ${log.join(",")} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_buff_marks.mjs <case-file>\n");
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
      if (sub === "id") buff_id = String(parseValue(t, idx));
      else if (sub === "kind") kind = String(parseValue(t, idx));
      else if (sub === "cls") cls = String(parseValue(t, idx));
      else if (sub === "victim") victims.push(ent(String(parseValue(t, idx))));
      else if (sub === "vpos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        const last = victims[victims.length - 1];
        if (last) {
          last.px = Number(v.x);
          last.py = Number(v.y);
          last.pz = Number(v.z);
        }
      } else if (sub === "vframe") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        const last = victims[victims.length - 1];
        if (last) {
          last.centery = Number(v.centery);
          last.height = Number(v.height);
          last.pic_h = Number(v.pic_h);
        }
      } else if (sub === "mark") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        const last = victims[victims.length - 1];
        if (last) last.marks.set(String(v.key), String(v.value));
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
        runMake();
      } else if (what === "mount") {
        buff!.mount();
        out.push(`run mount || ${log.join(",")} | ${stateText()}`);
      } else if (what === "unmount") {
        buff!.unmount();
        out.push(`run unmount || ${log.join(",")} | ${stateText()}`);
      } else if (what === "effect") {
        buff!.update(0);
        out.push(`run effect || ${log.join(",")} | ${stateText()}`);
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
