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
import { Ditto } from "../../../../src/LFW/ditto";
import type { IEntityCallbacks } from "../../../../src/LFW/entity/IEntityCallbacks";
import { Randoming } from "../../../../src/LFW/helper/Randoming";
import { Item } from "../../../../src/LFW/stage/Item";
import { Stage } from "../../../../src/LFW/stage/Stage";
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

// `Ditto.warn` 是全局告警（台面没装 sink 时是 “not a function”）⇒ 装一个记日志的
// （C++ 侧对应 `Stage::set_warn`）。
(Ditto as unknown as Record<string, unknown>).warn = (where: string, text: string) => {
  log.push(`warn:${where}:${text}`);
};

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
  private _team: unknown = "";
  get team(): unknown {
    return this._team;
  }
  set team(v: unknown) {
    this._team = v;
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

// ---------------------------------------------------------------- Stage 侧

// 假实体：`Stage` 用到的那一面。**读静默**（TS 那边都是属性读）、**写打日志**
// （`hp = hp_r = …` 这类赋值顺序与次数才是可比量）。
class FakeStageEntity {
  readonly label: string;
  data: unknown = undefined;
  team: unknown = "";
  ctrl: unknown = undefined;
  hp_max = 0;
  mp_max = 0;
  mounted = 0;
  position = { x: 0 };
  bearer: FakeStageEntity | undefined = undefined;
  private _hp = 0;
  private _hp_r = 0;
  private _mp = 0;
  private _facing = 1;
  constructor(label: string) {
    this.label = label;
  }
  get hp(): number {
    return this._hp;
  }
  set hp(v: number) {
    this._hp = v;
    log.push(`${this.label}:hp=${num(v)}`);
  }
  get hp_r(): number {
    return this._hp_r;
  }
  set hp_r(v: number) {
    this._hp_r = v;
    log.push(`${this.label}:hp_r=${num(v)}`);
  }
  get mp(): number {
    return this._mp;
  }
  set mp(v: number) {
    this._mp = v;
    log.push(`${this.label}:mp=${num(v)}`);
  }
  get facing(): number {
    return this._facing;
  }
  set facing(v: number) {
    this._facing = v;
    log.push(`${this.label}:facing=${vstr(v)}`);
  }
  set_position(x: unknown, y: unknown, z: unknown): void {
    log.push(`${this.label}:pos=${vstr(x)},${vstr(y)},${vstr(z)}`);
  }
  // 台面直接摆初值（**不打日志**）：与 `Stage` 写的那些属性区分开（C++ 侧同名方法）。
  set_hp_value(v: number): void {
    this._hp = v;
  }
  set_hp_r_value(v: number): void {
    this._hp_r = v;
  }
  set_mp_value(v: number): void {
    this._mp = v;
  }
}

// 假表达式：脚本来自 phase / dialog 数据里的 `__test` 数组（`Expr` 语义已被 `expr` 用例钉住，
// 这里只关心「谁的表达式被跑了、跑了几次」）。
class FakeStageExpr {
  readonly index: number;
  readonly script: boolean[];
  cursor = 0;
  constructor(index: number, script: boolean[]) {
    this.index = index;
    this.script = script;
  }
  run(_arg: unknown): boolean {
    log.push(`scall:${this.index}`);
    if (this.script.length === 0) return false;
    const v = this.script[Math.min(this.cursor, this.script.length - 1)]!;
    if (this.cursor < this.script.length - 1) this.cursor++;
    return v;
  }
}

const sstate = {
  bg: undefined as Background | undefined,
  has_stage: true,
  self: undefined as unknown,
  entities: [] as FakeStageEntity[],
  puppets: [] as FakeStageEntity[],
  diff: 2 as unknown,
  cam_x: 0,
  mt: new MersenneTwister(0),
  backgrounds: new Map<unknown, unknown>(),
  stages: new Map<unknown, unknown>(),
  players: [] as unknown[],
  team_counter: 0,
};

const sLfw = {
  // `Stage` 里 `this.lfw.world.*` 的几处（`set_phase` 的玩家表、`dispose`）与 `this.world.*` 同源。
  get world(): unknown {
    return sWorld;
  },
  get mt(): MersenneTwister {
    return sstate.mt;
  },
  datas: {
    backgrounds: {
      find(predicate: (v: never) => boolean): unknown {
        for (const v of sstate.backgrounds.values()) {
          if (predicate(v as never)) {
            log.push(`h:bgfind=${vstr((v as { id?: unknown }).id)}`);
            return v;
          }
        }
        log.push("h:bgfind=u");
        return undefined;
      },
    },
    stages: {
      find(predicate: (v: never) => boolean): unknown {
        for (const v of sstate.stages.values()) {
          if (predicate(v as never)) {
            log.push(`h:stagefind=${vstr((v as { id?: unknown }).id)}`);
            return v;
          }
        }
        log.push("h:stagefind=u");
        return undefined;
      },
    },
    find(oid: unknown): unknown {
      return fakeLfw.datas.find(oid);
    },
    get_randoming_by_group(oid: unknown): Randoming<unknown> {
      return fakeLfw.datas.get_randoming_by_group(oid);
    },
  },
  get new_team(): string {
    sstate.team_counter++;
    const v = `team_${sstate.team_counter}`;
    log.push(`h:newteam=${v}`);
    return v;
  },
  players: {
    has(id: unknown): boolean {
      log.push(`h:players=${vstr(id)}`);
      return sstate.players.includes(id);
    },
  },
  sounds: {
    play_bgm(music: unknown): () => void {
      log.push(`h:playbgm=${vstr(music)}`);
      return () => log.push("h:stopbgm");
    },
    stop_bgm(): void {
      log.push("h:stopbgm_now");
    },
    play(path: unknown, x: unknown, y: unknown, z: unknown): void {
      log.push(`h:sound=${vstr(path)},${vstr(x)},${vstr(y)},${vstr(z)}`);
    },
  },
  factory: fakeLfw.factory,
};

const sWorld = {
  lfw: sLfw,
  get bg(): Background | undefined {
    log.push("h:bg");
    return sstate.bg;
  },
  set bg(v: Background | undefined) {
    const old = sstate.bg
      ? `|old=${vstr(sstate.bg.id)}:n=${sstate.bg.layers.length}`
      : "|old=u";
    log.push(`h:setbg=${v ? vstr(v.id) : "u"}${old}`);
    sstate.bg = v;
  },
  get stage(): unknown {
    log.push(`h:stage=${flag(!!sstate.self)}`);
    return sstate.self;
  },
  get entities(): FakeStageEntity[] {
    log.push(`h:ents=${sstate.entities.length}`);
    return sstate.entities;
  },
  get puppets(): Map<string, FakeStageEntity> {
    log.push(`h:pupts=${sstate.puppets.length}`);
    const m = new Map<string, FakeStageEntity>();
    sstate.puppets.forEach((e, k) => m.set(`${k}`, e));
    return m;
  },
  dataset: {
    get difficulty(): unknown {
      log.push(`h:diff=${vstr(sstate.diff)}`);
      return sstate.diff;
    },
  },
  camera: {
    jump_x(x: number): void {
      sstate.cam_x = x;
      log.push(`h:camjump=${vstr(x)}`);
    },
  },
  del_entities(es: FakeStageEntity[]): void {
    const names = es.map((e) => e.label).join(",");
    log.push(`h:del=${names === "" ? "-" : names}`);
  },
};

let stage_data: unknown = undefined;
let stage: Stage | undefined = undefined;

const all_sentities: FakeStageEntity[] = [];

function stage_entity(label: string): FakeStageEntity | undefined {
  return all_sentities.find((e) => e.label === label);
}

// `phase.__end_testers` 在 TS 里是**真的表达式实例数组**；C++ 侧 `Value` 装不下 ⇒ 由宿主按
// 「同一份数据里的 `__test`」交出同样的列表。台面把 `__test` 就地转成 `__end_testers`。
function make_testers(owner: unknown): void {
  if (owner === null || typeof owner !== "object") return;
  const o = owner as { __test?: unknown; __end_testers?: unknown };
  const script = Array.isArray(o.__test) ? o.__test : [];
  o.__end_testers = script.map(
    (item: unknown, i: number) =>
      new FakeStageExpr(i, Array.isArray(item) ? (item as boolean[]) : [!!item]),
  );
}

function prepare_data(v: unknown): void {
  if (v === null || typeof v !== "object") return;
  const o = v as { phases?: unknown[]; dialogs?: unknown[] };
  if (Array.isArray(o.phases)) {
    for (const p of o.phases) {
      make_testers(p);
      const po = p as { dialogs?: unknown[] };
      if (Array.isArray(po.dialogs)) for (const d of po.dialogs) make_testers(d);
    }
  }
}

function dump_stage(): void {
  if (!stage) {
    log.push("stage|none");
    return;
  }
  const s = stage;
  const priv = s as unknown as { _dialogs: { index: number; list: unknown[] }; fsm: { time: number; state: { key: unknown } | undefined } };
  const objs = [...s.items].map((it) => `[${it.objects.size}]`).join(",");
  log.push(
    `stage|id=${vstr(s.id)}` +
      `|name=${vstr(s.name)}` +
      `|title=${vstr(s.title)}` +
      `|team=${vstr(s.team)}` +
      `|ph=${num(s.phase_idx)}` +
      `|fin=${flag(s.is_stage_finish)}` +
      `|cf=${flag(s.is_chapter_finish)}` +
      `|pt=${num(s.phase_time)}` +
      `|dt=${num(s.dialog_time)}` +
      `|di=${num(priv._dialogs.index)}` +
      `|dlg=${s.dialog ? vstr((s.dialog as { id?: unknown }).id) : "u"}` +
      `|fsm=${vstr(priv.fsm.state?.key)}` +
      `|ft=${num(priv.fsm.time)}` +
      `|L=${num(s.left)}` +
      `|R=${num(s.right)}` +
      `|n=${num(s.near)}` +
      `|f=${num(s.far)}` +
      `|w=${num(s.width)}` +
      `|d=${num(s.depth)}` +
      `|mid=${num(s.middle.x)},${num(s.middle.z)}` +
      `|pl=${num(s.player_l)}` +
      `|pr=${num(s.player_r)}` +
      `|cl=${num(s.cam_l)}` +
      `|cr=${num(s.cam_r)}` +
      `|el=${num(s.enemy_l)}` +
      `|er=${num(s.enemy_r)}` +
      `|dkl=${num(s.drink_l)}` +
      `|dkr=${num(s.drink_r)}` +
      `|bg=${sstate.bg ? vstr(sstate.bg.id) : "u"}` +
      `|items=${objs === "" ? "-" : objs}`,
  );
}

function dump_stage_quest(): void {  if (!stage) {
    log.push("squest|none");
    return;
  }
  const s = stage;
  log.push(
    `squest|ce=${num(s.ce)}` +
      `|pend=${flag(s.is_phase_end())}` +
      `|dend=${flag(s.is_dialog_end())}` +
      `|abd=${flag(s.all_boss_dead())}` +
      `|afd=${flag(s.all_fighter_dead())}` +
      `|dcl=${flag(s.dialog_cleared())}` +
      `|goto=${flag(s.should_goto_next_stage)}` +
      `|wp=${flag(s.world_pause)}` +
      `|cd=${flag(s.control_disabled)}` +
      `|wrd=${flag(s.weapon_rain_disabled)}` +
      `|next=${vstr(s.next_stage)}`,
  );
}

// 回调里只打「能不能分辨出是谁」的摘要（整对象渲染会牵扯键序）。
function phase_brief(v: unknown): string {
  return v ? vstr((v as { id?: unknown }).id) : "u";
}

function dlg_brief(v: unknown): string {
  const o = v as { index?: unknown; list?: unknown[] };
  return `${vstr(o.index)}/${Array.isArray(o.list) ? o.list.length : 0}`;
}

function main(): void {  const casePath = process.argv[2];
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
      // Stage 侧
    } else if (op === "sbgfind") {
      sstate.backgrounds.set(parseValue(t, i), parseValue(t, i));
    } else if (op === "sstagefind") {
      sstate.stages.set(parseValue(t, i), parseValue(t, i));
    } else if (op === "sbg") {
      sstate.bg = new Background({} as never, parseValue(t, i) as never);
    } else if (op === "schangebg") {
      stage?.change_bg(parseValue(t, i) as never);
    } else if (op === "sdiff") {
      sstate.diff = parseValue(t, i);
    } else if (op === "splayer") {
      sstate.players.push(parseValue(t, i));
    } else if (op === "stmseed") {
      sstate.mt.reset(number_of(t, i, op));
    } else if (op === "smtmark") {
      log.push(`mtmark=${vstr((sstate.mt as unknown as { mark?: unknown }).mark)}`);
    } else if (op === "steamlike") {
      const e = stage_entity(t[i[0]!++]!);
      if (!e) fail("no such entity");
      const t2 = stage ? (stage as unknown as { team: unknown }).team : undefined;
      e!.team = t2;
      log.push(`h:teamlike=${vstr(t2)}`);
    } else if (op === "sprop") {
      sstate.self = stage;
    } else if (op === "sent") {
      all_sentities.push(new FakeStageEntity(t[i[0]!++]!));
    } else if (op === "sentdata") {
      const e = stage_entity(t[i[0]!++]!);
      if (!e) fail("no such entity");
      e!.data = parseValue(t, i);
    } else if (op === "sentce") {
      const e = stage_entity(t[i[0]!++]!);
      if (!e) fail("no such entity");
      const ce = parseValue(t, i);
      const d = (e!.data && typeof e!.data === "object" ? e!.data : {}) as Record<string, unknown>;
      const base = (d.base && typeof d.base === "object" ? d.base : {}) as Record<string, unknown>;
      d.base = { ...base, ce };
      e!.data = d;
    } else if (op === "sentteam") {
      const e = stage_entity(t[i[0]!++]!);
      if (!e) fail("no such entity");
      e!.team = parseValue(t, i);
    } else if (op === "sentctrl") {
      const e = stage_entity(t[i[0]!++]!);
      if (!e) fail("no such entity");
      e!.ctrl = parseValue(t, i);
    } else if (op === "senthp" || op === "senthpmax" || op === "senthpr" || op === "sentmp" ||
               op === "sentmpmax" || op === "sentmounted" || op === "sentx") {
      const e = stage_entity(t[i[0]!++]!);
      if (!e) fail("no such entity");
      const v = number_of(t, i, op);
      if (op === "senthp") e!.set_hp_value(v);
      else if (op === "senthpmax") e!.hp_max = v;
      else if (op === "senthpr") e!.set_hp_r_value(v);
      else if (op === "sentmp") e!.set_mp_value(v);
      else if (op === "sentmpmax") e!.mp_max = v;
      else if (op === "sentmounted") e!.mounted = v;
      else e!.position.x = v;
    } else if (op === "sentbearer") {
      const e = stage_entity(t[i[0]!++]!);
      const b = stage_entity(t[i[0]!++]!);
      if (!e) fail("no such entity");
      e!.bearer = b;
    } else if (op === "sentities") {
      sstate.entities = [];
      while (i[0]! < t.length) {
        const e = stage_entity(t[i[0]!++]!);
        if (!e) fail("no such entity");
        sstate.entities.push(e!);
      }
    } else if (op === "spuppets") {
      sstate.puppets = [];
      while (i[0]! < t.length) {
        const e = stage_entity(t[i[0]!++]!);
        if (!e) fail("no such entity");
        sstate.puppets.push(e!);
      }
    } else if (op === "sdata") {
      stage_data = parseValue(t, i);
    } else if (op === "snew") {
      prepare_data(stage_data);
      stage = new Stage(sWorld as never, stage_data as never);
      const any = stage as unknown as {
        callbacks: { add(v: unknown): void };
      };
      any.callbacks.add({
        on_stage_finish: () => log.push("cb:stage_finish"),
        on_chapter_finish: () => log.push("cb:chapter_finish"),
        on_requrie_goto_next_stage: () => log.push("cb:goto_next"),
        on_phase_changed: (_s: unknown, curr: unknown, prev: unknown) =>
          log.push(`cb:phase=${phase_brief(curr)},${phase_brief(prev)}`),
        on_dialogs_changed: (curr: unknown, prev: unknown) =>
          log.push(`cb:dlg=${dlg_brief(curr)},${dlg_brief(prev)}`),
      });
    } else if (op === "sfree") {
      stage = undefined;
    } else if (op === "sdump") {
      dump_stage();
    } else if (op === "squest") {
      dump_stage_quest();
    } else if (op === "sphase") {
      stage?.enter_phase(number_of(t, i, op));
    } else if (op === "supd") {
      const n = i[0]! < t.length ? number_of(t, i, op) : 1;
      for (let k = 0; k < n; k++) stage?.update();
    } else if (op === "sdisp") {
      stage?.dispose();
    } else if (op === "skill") {
      const which = t[i[0]!++]!;
      if (stage) {
        if (which === "all") stage.kill_all();
        else if (which === "soldiers") stage.kill_soliders();
        else if (which === "boss") stage.kill_boss();
        else if (which === "others") stage.kill_others();
        else fail(`bad kill target '${which}'`);
      }
    } else if (op === "spushd") {
      const more = parseValue(t, i);
      // `spushd` 传进来的那些对话框没走过 `prepare_data` ⇒ 就地补上 `__end_testers`
      // （C++ 侧是 `end_testers(owner)` 按需生成，所以不需要这一步）。
      for (const d of Array.isArray(more) ? more : []) make_testers(d);
      stage?.push_dialogs(more as never);
    } else if (op === "snextd") {
      stage?.next_dialog();
    } else if (op === "scleard") {
      stage?.clear_dialogs();
    } else if (op === "sstopbgm") {
      stage?.stop_bgm();
    } else {
      fail(`unknown op '${op}'`);
    }

    if (i[0] !== t.length) fail(`trailing token(s): ${raw}`);
    while (log.length) process.stdout.write(log.shift() + "\n");
  }
}

main();
