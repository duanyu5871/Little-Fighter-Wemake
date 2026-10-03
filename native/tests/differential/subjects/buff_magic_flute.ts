import { Buff_MagicFlute } from "../../../../src/LFW/buff/Buff_MagicFlute";
import { Buff_MagicFlute2 } from "../../../../src/LFW/buff/Buff_MagicFlute2";
import { summary_mgr } from "../../../../src/LFW/entity/SummaryMgr";
import { parseValue, readCaseLines, renderValue, splitWs, num } from "./trace_util";

const out: string[] = [];
const log: string[] = [];

const r = (v: unknown): string => renderValue(v);

class FakeEnt {
  _id: string;
  _data: Record<string, unknown> = {};
  _indexes: unknown = undefined;
  _state: unknown = undefined;
  _hp: unknown = undefined;
  _hp_r: unknown = undefined;
  _fallinjury: unknown = undefined;
  _toughness: unknown = undefined;
  _team: unknown = undefined;
  vy = 0;
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
    return this._indexes === undefined ? this._data : { ...this._data, indexes: this._indexes };
  }
  get state(): unknown {
    return this._state;
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
  set hp_r(v: unknown) {
    this._hp_r = v;
    log.push(`${this._id}:set_hp_r:${r(v)}`);
  }
  get fallinjury(): unknown {
    return this._fallinjury;
  }
  set fallinjury(v: unknown) {
    this._fallinjury = v;
    log.push(`${this._id}:set_fallinjury:${r(v)}`);
  }
  get toughness(): unknown {
    return this._toughness;
  }
  set toughness(v: unknown) {
    this._toughness = v;
    log.push(`${this._id}:set_toughness:${r(v)}`);
  }
  get team(): unknown {
    return this._team;
  }
  set team(v: unknown) {
    this._team = v;
    log.push(`${this._id}:set_team:${r(v)}`);
  }
  get velocity(): { x: number; y: number; z: number } {
    return { x: 0, y: this.vy, z: 0 };
  }
  get position(): { x: number; y: number; z: number } {
    return { x: 0, y: 0, z: 0 };
  }
  get frame(): Record<string, unknown> {
    return { centery: 0, height: 0, pic: undefined };
  }
  set_velocity(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this._id}:set_velocity:${r(x)}:${r(y)}:${r(z)}`);
  }
  handle_velocity_decay(accx: number): void {
    log.push(`${this._id}:handle_velocity_decay:${r(accx)}`);
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
const victims: FakeEnt[] = [];
let effect: FakeEnt | undefined = undefined;
let buff: { update: (d: number) => void; init: () => void; duration: number } | undefined;
let buff_id = "B1";
let kind = "MagicFlute";
let cls = "mf1";
let attackerId = "";

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
    `ticks=${buff ? r((buff as unknown as { ticks: number }).ticks) : "-"} ` +
    `dur=${buff ? r((buff as unknown as { duration: number }).duration) : "-"} ` +
    `life=${buff ? r((buff as unknown as { lifetime: number }).lifetime) : "-"} ` +
    `hp=${perVictim((e) => r(e._hp))} ` +
    `hp_r=${perVictim((e) => r(e._hp_r))} ` +
    `fall=${perVictim((e) => r(e._fallinjury))} ` +
    `tough=${perVictim((e) => r(e._toughness))} ` +
    `team=${perVictim((e) => r(e._team))} ` +
    `vy=${perVictim((e) => r(e.vy))} ` +
    `type=${perVictim((e) => r(e._data.type))}`
  );
}

(summary_mgr as unknown as { apply_damage: unknown }).apply_damage = (
  a: FakeEnt,
  injury: unknown,
  v: FakeEnt,
  prev_hp: unknown,
) => {
  log.push(`apply_damage:${a.id}:${r(injury)}:${v.id}:${r(prev_hp)}`);
};

function runMake(): void {
  const Ctor = cls === "mf2" ? Buff_MagicFlute2 : Buff_MagicFlute;
  buff = new Ctor(lfwFake as never, buff_id, kind);
  victims.forEach((v, idx) => {
    if (idx === 0) (buff as unknown as { set_victim: (x: FakeEnt) => void }).set_victim(v);
    else (buff as unknown as { add_victim: (x: FakeEnt) => void }).add_victim(v);
  });
  if (attackerId) (buff as unknown as { set_attacker: (x: string) => void }).set_attacker(attackerId);
  out.push(`run make || ${log.join(",")} | ${stateText()}`);
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) {
    process.stderr.write("usage: lfw_trace_buff_magic_flute.mjs <case-file>\n");
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
      else if (sub === "attacker") attackerId = String(parseValue(t, idx));
      else if (sub === "ateam") ent(attackerId)._team = parseValue(t, idx);
      else if (sub === "victim") {
        const e = ent(String(parseValue(t, idx)));
        const at = victims.indexOf(e);
        if (at >= 0) victims.splice(at, 1);
        victims.push(e);
      } else if (sub === "vtype") {
        const last = victims[victims.length - 1];
        if (last) last._data = { type: parseValue(t, idx) };
      } else if (sub === "vindexes") {
        const last = victims[victims.length - 1];
        if (last) last._indexes = parseValue(t, idx);
      } else if (sub === "vstate") {
        const last = victims[victims.length - 1];
        if (last) last._state = parseValue(t, idx);
      } else if (sub === "vhp") {
        const last = victims[victims.length - 1];
        if (last) last._hp = parseValue(t, idx);
      } else if (sub === "vhp_r") {
        const last = victims[victims.length - 1];
        if (last) last._hp_r = parseValue(t, idx);
      } else if (sub === "vfallinjury") {
        const last = victims[victims.length - 1];
        if (last) last._fallinjury = parseValue(t, idx);
      } else if (sub === "vtough") {
        const last = victims[victims.length - 1];
        if (last) last._toughness = parseValue(t, idx);
      } else if (sub === "vteam") {
        const last = victims[victims.length - 1];
        if (last) last._team = parseValue(t, idx);
      } else if (sub === "vvy") {
        const last = victims[victims.length - 1];
        if (last) last.vy = num(t[i++]!);
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
