import { Buff_Electroshock } from "../../../../src/LFW/buff/Buff_Electroshock";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _data: Record<string, unknown> = { type: 8 };
  _state: unknown = undefined;
  _wait: unknown = undefined;
  px = 0;
  py = 0;
  pz = 0;
  centery = 0;
  height = 0;
  pic_h = 0;
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
  get data(): Record<string, unknown> {
    return this._data;
  }
  get state(): unknown {
    return this._state;
  }
  get wait(): unknown {
    return this._wait;
  }
  set wait(v: unknown) {
    this._wait = v;
    log.push(`${this._id}:set_wait:${r(v)}`);
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
let buff:
  | {
      update: (d: number) => void;
      init: () => void;
      mount: () => void;
      unmount: () => void;
      set_duration: (n: number) => void;
      ticks: number;
      lifetime: number;
      duration: number;
    }
  | undefined;
let buff_id = "B1";
let kind = "Electroshock";

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

function perVictim(fn: (e: FakeEnt) => string): string {
  return `[${victims.map((v) => `${v._id}:${fn(v)}`).join(",")}]`;
}

function stateText(): string {
  return (
    `id=${buff_id} victims=[${victims.map((v) => v._id).join(",")}] ` +
    `ticks=${buff ? r(buff.ticks) : "-"} dur=${buff ? r(buff.duration) : "-"} ` +
    `life=${buff ? r(buff.lifetime) : "-"} ` +
    `state=${perVictim((e) => r(e._state))} ` +
    `type=${perVictim((e) => r(e._data.type))} ` +
    `wait=${perVictim((e) => r(e._wait))}`
  );
}

function runMake(): void {
  buff = new Buff_Electroshock(lfwFake as never, buff_id, kind);
  victims.forEach((v, idx) => {
    if (idx === 0) (buff as unknown as { set_victim: (x: FakeEnt) => void }).set_victim(v);
    else (buff as unknown as { add_victim: (x: FakeEnt) => void }).add_victim(v);
  });
  out.push(`run make || ${log.join(",")} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_buff_electroshock.mjs <case-file>\n");
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
      else if (sub === "victim") {
        const e = ent(String(parseValue(t, idx)));
        const at = victims.indexOf(e);
        if (at >= 0) victims.splice(at, 1);
        victims.push(e);
      }
      else if (sub === "vtype") {
        const last = victims[victims.length - 1];
        if (last) last._data = { type: parseValue(t, idx) };
      } else if (sub === "vstate") {
        const last = victims[victims.length - 1];
        if (last) last._state = parseValue(t, idx);
      } else if (sub === "vwait") {
        const last = victims[victims.length - 1];
        if (last) last._wait = num(t[i++]!);
      } else if (sub === "vframe") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        const last = victims[victims.length - 1];
        if (last) {
          last.centery = Number(v.centery);
          last.height = Number(v.height);
          last.pic_h = Number(v.pic_h);
        }
      } else if (sub === "vpos") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        const last = victims[victims.length - 1];
        if (last) {
          last.px = Number(v.x);
          last.py = Number(v.y);
          last.pz = Number(v.z);
        }
      } else if (sub === "duration") {
        buff!.duration = num(t[i++]!);
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
      } else if (what === "init") {
        buff!.init();
        out.push(`run init || ${log.join(",")} | ${stateText()}`);
      } else if (what === "mount") {
        buff!.mount();
        out.push(`run mount || ${log.join(",")} | ${stateText()}`);
      } else if (what === "unmount") {
        buff!.unmount();
        out.push(`run unmount || ${log.join(",")} | ${stateText()}`);
      } else if (what === "tick") {
        buff!.update(num(t[i++]!));
        out.push(`run tick || ${log.join(",")} | ${stateText()}`);
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
