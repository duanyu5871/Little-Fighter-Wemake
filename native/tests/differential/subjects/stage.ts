// `stage/Expressions` + `stage/Status` 与 `bg/Background` + `bg/Layer` 的 TS 侧台面。
//
// 用例：`cases/stage/expr.txt`（Expressions + Status）、`cases/stage/bg.txt`（Background + Layer）。op：
//   it <b…>     追加一个假表达式（`run` 按脚本吐真假值，跑完最后一个就一直重复）
//   arg <v> | run | flow | next | resetsame | resetcopy | expdump | status
//   data <v> | new | bgdump | layer <i> | upd | disp | lset <i> <字段> <值>
//
// `resetsame` 传的是 `exp.list`（**内部那个数组**）⇒ 走 `reset` 的同一性早退；
// `resetcopy` 传一份内容相同的新数组 ⇒ 走「清空再灌」。两边都实现成 `reset(list)`。
import { Background } from "../../../../src/LFW/bg/Background";
import type { Layer } from "../../../../src/LFW/bg/Layer";
import { Callbacks } from "../../../../src/LFW/base/Callbacks";
import type { IEntityCallbacks } from "../../../../src/LFW/entity/IEntityCallbacks";
import { Randoming } from "../../../../src/LFW/helper/Randoming";
import { Item } from "../../../../src/LFW/stage/Item";
import { MersenneTwister } from "../../../../src/LFW/utils/math/MersenneTwister";
import { Expressions } from "../../../../src/LFW/stage/Expressions";
import { Status } from "../../../../src/LFW/stage/Status";

import { numHex, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

const log: string[] = [];

function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

function num(d: number): string {
  return numHex(d);
}

function vstr(v: unknown): string {
  return renderValue(v);
}

function flag(b: boolean): string {
  return b ? "1" : "0";
}

function number_of(t: string[], i: number[], op: string): number {
  const v = parseValue(t, i);
  if (typeof v !== "number") fail(`${op} expects a number literal`);
  return v;
}

// 假表达式：`run` 按脚本吐真假值（跑完最后一个就一直重复），并把每次调用记进日志。
class FakeExpr {
  readonly index: number;
  readonly script: boolean[];
  cursor = 0;
  constructor(index: number, script: boolean[]) {
    this.index = index;
    this.script = script;
  }
  run(arg: unknown): boolean {
    log.push(`call:${this.index}:arg=${vstr(arg)}`);
    if (this.script.length === 0) return false;
    const v = this.script[Math.min(this.cursor, this.script.length - 1)]!;
    if (this.cursor < this.script.length - 1) this.cursor++;
    return v;
  }
}

const exp = new Expressions<unknown>();
const items: FakeExpr[] = [];
let arg: unknown = undefined;

function dump_expr(): void {
  const index = (exp as unknown as { _index: number })._index;
  log.push(
    `dump|n=${exp.list.length}` +
      `|i=${num(index)}` +
      `|first=${flag(exp.is_first)}` +
      `|last=${flag(exp.is_last)}`,
  );
}

function dump_status(): void {
  log.push(`status|${Status.Running}|${Status.Completed}|${Status.End}|3`);
}

let data: { layers: Record<string, unknown>[] };
let bg: Background | undefined;

function dump_bg(): void {
  if (!bg) {
    log.push("bg|none");
    return;
  }
  const priv = bg as unknown as { _update_times: number; _layer_data_index: number };
  log.push(
    `bg|id=${vstr(bg.id)}` +
      `|name=${vstr(bg.name)}` +
      `|left=${num(bg.left)}` +
      `|right=${num(bg.right)}` +
      `|near=${num(bg.near)}` +
      `|far=${num(bg.far)}` +
      `|width=${num(bg.width)}` +
      `|height=${num(bg.height)}` +
      `|depth=${num(bg.depth)}` +
      `|mid=${num(bg.middle.x)},${num(bg.middle.z)}` +
      `|zoom=${num(bg.zoom_x)},${num(bg.zoom_y)},${num(bg.zoom_z)}` +
      `|n=${bg.layers.length}` +
      `|ut=${num(priv._update_times)}` +
      `|di=${num(priv._layer_data_index)}`,
  );
}

function dump_layer(index: number): void {
  if (!bg) {
    log.push("layer|none");
    return;
  }
  if (index < 0 || index >= bg.layers.length) {
    log.push("layer|oob");
    return;
  }
  const l = bg.layers[index] as Layer;
  log.push(
    `layer|di=${num(l.data_index)}` +
      `|li=${num(l.loop_index)}` +
      `|x=${num(Number(l.info.x))}` +
      `|y=${num(Number(l.info.y))}` +
      `|file=${vstr(l.info.file)}` +
      `|vis=${flag(l.visible)}` +
      `|st=${flag(l.is_static())}`,
  );
}

// ---------------------------------------------------------------- Item 侧

// 假实体：`Item` 用到的那一面（属性写走 setter，方法照抄）。
class FakeItemEntity {
  readonly label: string;
  readonly data: unknown;
  readonly ref: unknown;
  readonly callbacks = new Callbacks<IEntityCallbacks>();
  constructor(label: string, data: unknown) {
    this.label = label;
    this.data = data;
    this.ref = { data }; // `is_fighter(e)` / `is_weapon(e)` 读的是 `e.data`
  }
  set outline_color(v: unknown) {
    log.push(`${this.label}:outline=${vstr(v)}`);
  }
  set stat_bar(v: number) {
    log.push(`${this.label}:stat_bar=${num(v)}`);
  }
  set wakeup_invuln(v: number) {
    log.push(`${this.label}:wakeup_invuln=${num(v)}`);
  }
  set dead_gone(v: number) {
    log.push(`${this.label}:dead_gone=${num(v)}`);
  }
  set reserve(v: unknown) {
    log.push(`${this.label}:reserve=${vstr(v)}`);
  }
  set hp(v: number) {
    log.push(`${this.label}:hp=${num(v)}`);
  }
  set hp_r(v: number) {
    log.push(`${this.label}:hp_r=${num(v)}`);
  }
  set hp_max(v: number) {
    log.push(`${this.label}:hp_max=${num(v)}`);
  }
  set mp(v: number) {
    log.push(`${this.label}:mp=${num(v)}`);
  }
  set mp_max(v: number) {
    log.push(`${this.label}:mp_max=${num(v)}`);
  }
  set name(v: unknown) {
    log.push(`${this.label}:name=${vstr(v)}`);
  }
  set team(v: unknown) {
    log.push(`${this.label}:team=${vstr(v)}`);
  }
  set facing(v: unknown) {
    log.push(`${this.label}:facing=${vstr(v)}`);
  }
  set dead_join(v: unknown) {
    log.push(`${this.label}:dead_join=${vstr(v)}`);
  }
  set_position(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this.label}:pos=${vstr(x)},${vstr(y)},${vstr(z)}`);
  }
  attach(): void {
    log.push(`${this.label}:attach`);
  }
  enter_frame_by_id(v: unknown): void {
    log.push(`${this.label}:frame_id=${vstr(v)}`);
  }
  enter_frame(v: unknown): void {
    log.push(`${this.label}:frame=${vstr(v)}`);
  }
  // 台面自己触发那两件事（`Item` 靠它们出列）。
  fire_dead(): void {
    this.callbacks.call("on_dead", this as never);
  }
  fire_team_changed(): void {
    this.callbacks.call("on_team_changed", this as never, "", "");
  }
}

const host_state = {
  far: 0,
  near: 0,
  team: "" as unknown,
  aboss: false,
  diff: 2 as unknown,
  mt: new MersenneTwister(0),
  datas: new Map<unknown, unknown>(),
  groups: new Map<string, Randoming<unknown>>(),
  entities: [] as FakeItemEntity[],
};

const fakeLfw = {
  mt: host_state.mt,
  datas: {
    find(oid: unknown): unknown {
      if (host_state.datas.has(oid)) {
        const v = host_state.datas.get(oid);
        log.push(`h:find=${vstr(v)}`);
        return v;
      }
      log.push("h:find=u");
      return undefined;
    },
    get_randoming_by_group(oid: unknown): Randoming<unknown> {
      const key = String(oid);
      const hit = host_state.groups.get(key);
      if (hit) {
        log.push(`h:group=${key}:${hit.src.length}`);
        return hit;
      }
      log.push(`h:group=${key}:0`);
      return Randoming.create(key, [], host_state.mt);
    },
  },
  factory: {
    create_entity_with_bot(_player_id: string, _world: unknown, data: unknown): FakeItemEntity {
      const label = `e${host_state.entities.length}`;
      log.push(`h:create=${vstr(data)}`);
      const e = new FakeItemEntity(label, data);
      host_state.entities.push(e);
      return e;
    },
  },
};

const fakeWorld = {
  dataset: {
    get difficulty(): unknown {
      log.push(`h:diff=${vstr(host_state.diff)}`);
      return host_state.diff;
    },
  },
};

const fakeStage = {
  get far(): number {
    log.push("h:far");
    return host_state.far;
  },
  get near(): number {
    log.push("h:near");
    return host_state.near;
  },
  get team(): unknown {
    log.push("h:team");
    return host_state.team;
  },
  all_boss_dead(): boolean {
    log.push("h:aboss");
    return host_state.aboss;
  },
  lfw: fakeLfw,
  world: fakeWorld,
};

let item: Item | undefined;
let phase: unknown = undefined;
let info: unknown = undefined;

function by_label(label: string): FakeItemEntity | undefined {
  return host_state.entities.find((e) => e.label === label);
}

function dump_item(): void {
  if (!item) {
    log.push("item|none");
    return;
  }
  const objs = [...item.objects].map((e) => (e as unknown as FakeItemEntity).label).join(",");
  const delay = item.end_delay.value;
  log.push(
    `item|rel=${flag(item.released)}` +
      `|f=${flag(item.is_fighter)}` +
      `|times=${item.times === undefined ? "u" : num(item.times)}` +
      `|data=${vstr(item.data)}` +
      `|objs=${objs === "" ? "-" : objs}` +
      `|delay=${num(delay)}` +
      `|rq=${flag(!!item.randoming)}`,
  );
}

function main(): void {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_stage.mjs <case-file>");

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];

    if (op === "it") {
      const script: boolean[] = [];
      while (i[0]! < t.length) {
        const v = parseValue(t, i);
        if (typeof v !== "boolean") fail("it expects boolean literals");
        script.push(v);
      }
      items.push(new FakeExpr(items.length, script));
    } else if (op === "arg") {
      arg = parseValue(t, i);
    } else if (op === "run") {
      log.push(`run=${flag(exp.run(arg))}`);
    } else if (op === "flow") {
      log.push(`flow=${flag(exp.flow(arg))}`);
    } else if (op === "next") {
      exp.next();
    } else if (op === "resetsame") {
      exp.reset((exp as unknown as { list: unknown[] }).list as never);
    } else if (op === "resetcopy") {
      exp.reset([...items] as never);
    } else if (op === "expdump") {
      dump_expr();
    } else if (op === "status") {
      dump_status();
    } else if (op === "data") {
      data = parseValue(t, i) as never;
    } else if (op === "new") {
      // `world` 只被存起来、构造里不读 ⇒ 假对象够用。
      bg = new Background({} as never, data as never);
    } else if (op === "bgdump") {
      dump_bg();
    } else if (op === "layer") {
      dump_layer(number_of(t, i, op));
    } else if (op === "upd") {
      bg?.update();
    } else if (op === "disp") {
      bg?.dispose();
    } else if (op === "lset") {
      const index = number_of(t, i, op);
      const field = t[i[0]!++]!;
      const value = parseValue(t, i);
      data.layers[index]![field] = value;
    } else if (op === "mtseed") {
      host_state.mt.reset(number_of(t, i, op));
    } else if (op === "datas") {
      const oid = parseValue(t, i);
      host_state.datas.set(oid, parseValue(t, i));
    } else if (op === "datasgroup") {
      const oid = t[i[0]!++]!;
      const name = t[i[0]!++]!;
      const src: unknown[] = [];
      while (i[0]! < t.length) src.push(parseValue(t, i));
      host_state.groups.set(oid, Randoming.create(name, src, host_state.mt));
    } else if (op === "far") {
      host_state.far = number_of(t, i, op);
    } else if (op === "near") {
      host_state.near = number_of(t, i, op);
    } else if (op === "team") {
      host_state.team = parseValue(t, i);
    } else if (op === "aboss") {
      host_state.aboss = !!parseValue(t, i);
    } else if (op === "diff") {
      host_state.diff = parseValue(t, i);
    } else if (op === "phase") {
      phase = parseValue(t, i);
    } else if (op === "info") {
      info = parseValue(t, i);
    } else if (op === "newitem") {
      item = new Item(fakeStage as never, phase as never, info as never);
    } else if (op === "upd") {
      item?.update();
    } else if (op === "updn") {
      const n = number_of(t, i, op);
      for (let k = 0; k < n; k++) item?.update();
    } else if (op === "spawn") {
      log.push(`spawn=${flag(!!item?.spawn())}`);
    } else if (op === "rel") {
      item?.release();
    } else if (op === "itemdump") {
      dump_item();
    } else if (op === "dead" || op === "teamchg") {
      const e = by_label(t[i[0]!++]!);
      if (!e) fail(`no such entity '${t[1]}`);
      if (op === "dead") e!.fire_dead();
      else e!.fire_team_changed();
    } else {
      fail(`unknown op '${op}'`);
    }

    if (i[0] !== t.length) fail(`trailing token(s): ${raw}`);
    while (log.length) process.stdout.write(log.shift() + "\n");
  }
}

main();
