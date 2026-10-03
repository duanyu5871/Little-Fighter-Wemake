import { Buff_Healing } from "../../../../src/LFW/buff/Buff_Healing";
import { Buff_MpHealing } from "../../../../src/LFW/buff/Buff_MpHealing";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  dataset_values: Record<string, unknown> = {};
  _hp: unknown = undefined;
  _hp_r: unknown = undefined;
  _mp: unknown = undefined;
  _mp_max: unknown = undefined;
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
  dataset(key: string): unknown {
    return this.dataset_values[key];
  }
  get hp(): unknown {
    return this._hp;
  }
  set hp(v: unknown) {
    this._hp = v;
    log.push(`${this._id}:set_hp:${r(v)}`);
  }
  get hp_r(): unknown {
    return this._hp_r;
  }
  get mp(): unknown {
    return this._mp;
  }
  set mp(v: unknown) {
    this._mp = v;
    log.push(`${this._id}:set_mp:${r(v)}`);
  }
  get mp_max(): unknown {
    return this._mp_max;
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
}

const ents: FakeEnt[] = [];
const victims: FakeEnt[] = [];
let buff:
  | {
      update: (d: number) => void;
      mount: () => void;
      unmount: () => void;
      set_ticks: (n: number) => void;
      set_duration: (n: number) => void;
      ticks: number;
      lifetime: number;
      duration: number;
    }
  | undefined;
let buff_id = "B1";
let kind = "Healing";
let cls = "healing";

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

const lfwFake = { world: worldFake, datas: { find: (_o: string) => undefined } };

function marksText(): string {
  return `[${ents
    .map((e) => `${e._id}{${[...e.marks.entries()].map(([k, v]) => `${k}=${v}`).join(",")}}`)
    .join(",")}]`;
}

function statsText(): string {
  const hp = victims.map((f) => `${f._id}:${r(f._hp)}/${r(f._hp_r)}`);
  const mp = victims.map((f) => `${f._id}:${r(f._mp)}/${r(f._mp_max)}`);
  return `hp=[${hp.join(",")}] mp=[${mp.join(",")}]`;
}

function stateText(): string {
  return (
    `id=${buff_id} victims=[${victims.map((v) => v._id).join(",")}] marks=${marksText()} ` +
    `${statsText()} ticks=${buff ? r(buff.ticks) : "-"} ` +
    `life=${buff ? r(buff.lifetime) : "-"} dur=${buff ? r(buff.duration) : "-"}`
  );
}

function runMake(): void {
  buff =
    cls === "mp_healing"
      ? new Buff_MpHealing(lfwFake as never, buff_id, kind)
      : new Buff_Healing(lfwFake as never, buff_id, kind);
  victims.forEach((v, idx) => {
    if (idx === 0) (buff as unknown as { set_victim: (x: FakeEnt) => void }).set_victim(v);
    else (buff as unknown as { add_victim: (x: FakeEnt) => void }).add_victim(v);
  });
  out.push(`run make || ${log.join(",")} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_buff_healing.mjs <case-file>\n");
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
      else if (sub === "vdata") {
        const last = victims[victims.length - 1];
        if (last) last.dataset_values = parseValue(t, idx) as Record<string, unknown>;
      } else if (sub === "vhp") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        const last = victims[victims.length - 1];
        if (last) {
          last._hp = v.hp;
          last._hp_r = v.hp_r;
        }
      } else if (sub === "vmp") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        const last = victims[victims.length - 1];
        if (last) {
          last._mp = v.mp;
          last._mp_max = v.mp_max;
        }
      } else if (sub === "mark") {
        const v = parseValue(t, idx) as Record<string, unknown>;
        const last = victims[victims.length - 1];
        if (last) last.marks.set(String(v.key), String(v.value));
      } else if (sub === "ticks") {
        buff!.set_ticks(num(t[i++]!));
      } else if (sub === "duration") {
        buff!.set_duration(num(t[i++]!));
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
      } else if (what === "tick") {
        buff!.update(num(t[i++]!));
        out.push(`run tick || ${log.join(",")} | ${stateText()}`);
      } else if (what === "duration") {
        const amount = num(t[i++]!);
        const first = victims[0];
        const d = first
          ? cls === "mp_healing"
            ? Buff_MpHealing.duration_of(first as never, amount)
            : Buff_Healing.duration_of(first as never, amount)
          : 0;
        out.push(`run duration || ${log.join(",")} | d=${r(d)}`);
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
