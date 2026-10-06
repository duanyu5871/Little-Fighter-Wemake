// `Factory`（`src/LFW/Factory.ts`）的 TS 侧台面。
//
// 用例：`cases/factory/all.txt`。op：
//   regent  <key> <label>            注册实体 creator（label == "miss" ⇒ 返回 undefined）
//   regctrl <key> <label>            注册控制器 creator
//   regbuff <kind> <group> […]       注册 buff creator（`GROUPS` 是后面这些 bare token）
//   ce      <key> <data>             `create_entity`
//   cebot   <pid> <key> <data>       `create_entity_with_bot`
//   acq-e   <key>                    `acquire_entity`
//   rec-e   <label>                  `recycle_entity`（该 label 造出来的下一个实体）
//   newctrl <oid> <pid>              `create_ctrl`
//   acq-ctrl <label> <pid>           `acquire_ctrl`（label 对应的 creator）
//   rel-ctrl <label>                 `release_ctrl`（该 label 造出来的下一个控制器）
//   cbuff   <kind> <id>              `create_buff`
//   rec-buff <label>                 `recycle_buff`
//   dump                             四张注册表 / 三个池 / 分组表 / 告警日志
import { Factory } from "../../../../src/LFW/Factory";
import { Ditto } from "../../../../src/LFW/ditto/Instance";

import { keyOf, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const log: string[] = [];
const warns: string[] = [];

function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

type FakeEntity = { data: unknown; ctrl: unknown };
type FakeBuff = { kind: unknown; id: string; inits: number };
type FakeCtrl = { player_id: string; tag: string; resets: number; constructor: unknown };

const entities = new Map<string, FakeEntity[]>();
const ctrls = new Map<string, FakeCtrl[]>();
const buffs = new Map<string, FakeBuff[]>();
const entity_creators = new Map<string, unknown>();
const ctrl_creators = new Map<string, unknown>();
const buff_creators = new Map<string, unknown>();

function pool<T>(m: Map<string, T[]>, label: string): T[] {
  let v = m.get(label);
  if (v === undefined) m.set(label, (v = []));
  return v;
}

function label_of_buff_creator(kind: unknown): string {
  let found = "?";
  for (const [label, cls] of buff_creators) {
    if (renderValue((cls as { KIND: unknown }).KIND) === renderValue(kind)) found = label;
  }
  return found;
}

function label_of_ctrl_creator(cls: unknown): string {
  for (const [label, v] of ctrl_creators) {
    if (v === cls) return label;
  }
  return "?";
}

// `create_buff` 复用池里的实例，所以每个 label 只造一次「模板」；`reset` / `init` 的次数要记下来。
// 实例由 TS 侧 `new B(lfw, id, B.KIND)` 造出来 ⇒ `kind` 由构造参数决定；池的键就是 `buff.kind`。
class FakeBuffClass {
  KIND: unknown;
  readonly GROUPS: string[];
  id = "";
  resets = 0;
  inits = 0;
  constructor(lfw: unknown, id: string, kind: unknown) {
    void lfw;
    this.KIND = kind;
    this.GROUPS = ((this.constructor as unknown as { GROUPS?: string[] }).GROUPS ?? []).slice();
    this.id = id;
  }
  get kind(): unknown {
    return this.KIND;
  }
  reset(id: string): void {
    this.resets += 1;
    this.id = id;
  }
  init(): void {
    this.inits += 1;
  }
}

function install_ditto(): void {
  Ditto.setup({
    warn: (...args: unknown[]) => {
      warns.push(`warn:${String(args[0])}`);
    },
  } as never);
}

function make_entity(label: string, data: unknown): FakeEntity {
  const e: FakeEntity = { data, ctrl: undefined };
  pool(entities, label).push(e);
  log.push(`e:${label}`);
  return e;
}

// 只打「可对齐」的表面：三张注册表的键数、buff 分组表（扁平）、三个池的「键=条数」。
function dump(factory: Factory): void {
  const keys = (m: Map<unknown, unknown>): string => {
    const parts: string[] = [];
    for (const k of m.keys()) parts.push(renderValue(k));
    return `[${parts.join(",")}]`;
  };
  const groups: string[] = [];
  for (const [g, set] of Factory.buff_groups) {
    const kinds: string[] = [];
    for (const k of set) kinds.push(renderValue(k));
    groups.push(`${renderValue(g)}=[${kinds.join(",")}]`);
  }
  const sizes = (m: Map<unknown, unknown>, key_text: (k: unknown) => string): string => {
    const parts: string[] = [];
    for (const [k, v] of m) {
      const l = (v as { l: unknown[] }).l.filter((x) => x !== undefined).length;
      parts.push(`${key_text(k)}=${l}`);
    }
    return `[${parts.join(",")}]`;
  };
  log.push(
    "dump" +
      `|entities=${keys(Factory.entity_creators as never)}` +
      `|ctrls=${keys(Factory.ctrl_creators as never)}` +
      `|buffs=${keys(Factory.buff_creators as never)}` +
      `|groups=[${groups.join(",")}]` +
      `|graves=${sizes(factory.graves_maps as never, renderValue)}` +
      `|bgraves=${sizes(factory.buff_graves_maps as never, renderValue)}` +
      `|cgraves=${sizes(factory.ctrl_graves_maps as never, label_of_ctrl_creator)}`,
  );
}
function main(): void {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_factory.mjs <case-file>");
  install_ditto();
  const factory = new Factory();

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;
    const arg = (): unknown => parseValue(t, i);

    if (op === "regent") {
      const key = arg();
      const label = keyOf(next());
      const creator = (world: unknown, data: unknown): FakeEntity | undefined => {
        void world;
        log.push(`create:${label}|${renderValue(data)}`);
        if (label === "miss") return undefined;
        return make_entity(label, data);
      };
      (creator as unknown as { label: string }).label = label;
      entity_creators.set(label, creator);
      Factory.register_entity(key as never, creator as never);
    } else if (op === "regctrl") {
      const key = arg();
      const label = keyOf(next());
      const Cls = class {
        player_id: string;
        readonly tag: string;
        resets = 0;
        constructor(player_id: string, entity: unknown) {
          void entity;
          this.tag = label;
          this.player_id = player_id;
          pool(ctrls, label).push(this);
          log.push(`newctrl:${label}|${player_id}`);
        }
        reset(player_id: string, entity: unknown): void {
          void entity;
          this.resets += 1;
          this.player_id = player_id;
        }
      };
      ctrl_creators.set(label, Cls);
      Factory.register_ctrl(key as never, Cls as never);
    } else if (op === "regbuff") {
      const kind = arg();
      const label = keyOf(next());
      const groups: string[] = [];
      while (i[0]! < t.length) groups.push(keyOf(next()));
      const Cls = class extends FakeBuffClass {
        constructor(lfw: unknown, id: string, inst_kind: unknown) {
          super(lfw, id, inst_kind);
          pool(buffs, label).push(this);
          log.push(`newbuff:${label}|${renderValue(kind)}`);
        }
      };
      (Cls as unknown as { KIND: unknown }).KIND = kind;
      (Cls as unknown as { GROUPS: string[] }).GROUPS = groups;
      buff_creators.set(label, Cls);
      Factory.register_buff(Cls as never);
    } else if (op === "ce") {
      const key = arg();
      const data = arg();
      const ret = factory.create_entity(null as never, data as never, undefined);
      log.push(`ce:${renderValue(key)}|${ret === undefined ? "u" : "hit"}`);
    } else if (op === "cebot") {
      const pid = keyOf(next());
      const key = arg();
      const data = arg();
      const ret = factory.create_entity_with_bot(pid, null as never, data as never, undefined);
      log.push(`cebot:${renderValue(key)}|${ret === undefined ? "u" : "hit"}|ctrl=${ret?.ctrl === undefined || ret.ctrl === null ? "u" : "hit"}`);
    } else if (op === "acq-e") {
      const key = arg();
      const ret = factory.acquire_entity(key as never);
      log.push(`acq-e:${renderValue(key)}|${ret === undefined ? "u" : `hit:${renderValue((ret as FakeEntity).data)}`}`);
    } else if (op === "rec-e") {
      const label = keyOf(next());
      const list = pool(entities, label);
      const e = list.shift();
      if (e === undefined) fail(`no entity for label '${label}'`);
      factory.recycle_entity(e as never);
      log.push(`rec-e:${label}`);
    } else if (op === "newctrl") {
      const oid = arg();
      const pid = keyOf(next());
      const ret = factory.create_ctrl(oid as never, pid, {} as never);
      log.push(`newctrl-call:${renderValue(oid)}|${ret === undefined ? "u" : "hit"}`);
    } else if (op === "acq-ctrl") {
      const label = keyOf(next());
      const pid = keyOf(next());
      const creator = ctrl_creators.get(label) ?? fail(`unknown ctrl label '${label}'`);
      const ret = factory.acquire_ctrl(creator as never, pid, {} as never);
      log.push(`acq-ctrl:${label}|${ret === undefined ? "u" : `hit:r${(ret as FakeCtrl).resets}|p${(ret as FakeCtrl).player_id}`}`);
    } else if (op === "rel-ctrl") {
      const label = keyOf(next());
      const list = pool(ctrls, label);
      const c = list.shift();
      if (c === undefined) fail(`no ctrl for label '${label}'`);
      factory.release_ctrl(c as never);
      log.push(`rel-ctrl:${label}`);
    } else if (op === "cbuff") {
      const kind = arg();
      const id = keyOf(next());
      const ret = factory.create_buff(kind as never, {} as never, id);
      if (ret !== undefined) {
        const list = pool(buffs, label_of_buff_creator(kind));
        if (!list.includes(ret as FakeBuff)) list.push(ret as FakeBuff);
      }
      log.push(`cbuff:${renderValue(kind)}|${ret === undefined ? "u" : `hit:${(ret as FakeBuff).id}|i${(ret as FakeBuff).inits}`}`);
    } else if (op === "rec-buff") {
      const label = keyOf(next());
      const list = pool(buffs, label);
      const b = list.shift();
      if (b === undefined) fail(`no buff for label '${label}'`);
      factory.recycle_buff(b as never);
      log.push(`rec-buff:${label}`);
    } else if (op === "dump") {
      dump(factory);
    } else {
      fail(`unknown op '${op}'`);
    }

    if (i[0] !== t.length) fail(`trailing token(s): ${raw}`);
    while (warns.length) log.push(warns.shift()!);
    while (log.length) process.stdout.write(log.shift() + "\n");
  }
}

main();
