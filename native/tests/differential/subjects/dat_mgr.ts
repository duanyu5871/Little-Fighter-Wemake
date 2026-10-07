// `loader/DatMgr`（数据表管理器）的 TS 侧台面，op 与 `subjects/dat_mgr.cpp` 一一对应。
//
// 资源链走的是**真 `Resources` + 空 `ZipMgr`**（都 miss ⇒ 落到 `Ditto.Importer` 桩）；
// `Ditto.Importer` / `Ditto.XML` / `Ditto.warn` / `Ditto.error` 全是脚本化假件。
// `lfw` 只给 `resources` / `images.load_img` / `mt` / `emit_progress` 四个成员。
//
// op 一览（两侧同名）：
//   jfile <path> <value>        脚本化 import_as_json 的返回
//   xtree <path> <tree>         脚本化 import_as_text（文本 = path）+ XML.parse 的元素树
//   jfail <path> <msg>          import_as_json 抛错
//   tfail <path> <msg>          import_as_text 抛错
//   clearat <json|text> <path>  该次 import 前先 `mgr.clear()`（模拟 await 期间被取消）
//   clearimg <path>             load_img(path) 时 `mgr.clear()`
//   spark                       脚本化内置数据 `data/spark.obj.json5`
//   load <path...>              跑 `mgr.load(paths)`
//   dump / find / findbot / findmoves / fwv / fwpred / fobjv / fentv / ffv / fbgv
//   objg / fg / wg / fng / bgg  分组查询（打 id 列表）
//   randg / randgc / bgr / rbg  随机组（固定 mt，种子 12345）
//   ctrls / mkctrl              `Factory.ctrl_creators` 表 / 造一个控制器看注册效果
//   clear / dispose / innerid
import { BotController } from "../../../../src/LFW/bot/BotController";
import { BallController } from "../../../../src/LFW/controller/BallController";
import { Ditto } from "../../../../src/LFW/ditto/Instance";
import { Factory } from "../../../../src/LFW/Factory";
import { DatMgr } from "../../../../src/LFW/loader/DatMgr";
import { Resources } from "../../../../src/LFW/Resources";
import { MersenneTwister } from "../../../../src/LFW/utils/math/MersenneTwister";
import { ZipMgr } from "../../../../src/LFW/ZipMgr";
import { ToolXML } from "../../../../tool/src/xml";

import { keyOf, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

type Rec = Record<string, any>;

const log: string[] = [];
function push(s: string): void {
  log.push(s);
}
function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

const xml = new ToolXML();
const mt = new MersenneTwister(12345);
const jfiles = new Map<string, unknown>();
const tfiles = new Map<string, string>();
const trees = new Map<string, unknown>();
const jfails = new Map<string, string>();
const tfails = new Map<string, string>();
const clear_at = new Set<string>();
let clear_img = "";

// `xtree` 的树字面量：`o 3 tag s "entity" attrs o <n> … kids a <n> …`（递归）。
// 用 `create` / `set_attr` / `insert` 搭出来（`ToolXML.parse` 端口未搬，见 i_xml.h）。
function build_el(v: unknown): unknown {
  const rec = v as Rec;
  const el = xml.create(String(rec["tag"]));
  const attrs = rec["attrs"] as Rec | undefined;
  if (attrs) for (const k of Object.keys(attrs)) el.set_attr(k, attrs[k]);
  const kids = rec["kids"] as unknown[] | undefined;
  if (kids) for (const kid of kids) el.insert(build_el(kid) as never);
  return el;
}

let mgr: DatMgr;
const resources = new Resources(new ZipMgr());
const factory = new Factory();
const fake_entity = { lfw: { players: { get: () => undefined } }, world: {} };

function import_json(urls: string[]): [unknown, unknown] {
  push(`imp:json|${urls.join(",")}`);
  const p = urls[0]!;
  const f = jfails.get(p);
  if (f !== undefined) throw new Error(f);
  if (clear_at.has(`json:${p}`)) mgr.clear();
  const v = jfiles.get(p);
  if (v === undefined) fail(`unscripted import_as_json '${p}'`);
  // 真导入（file.json() / Importer）每次都给**新对象** ⇒ 台面也克隆一份：
  // 否则第二次 cook 会撞上 `preprocess_entity_data` 的 `data.xml = …`（DatMgr 先挂了
  // 只读 getter）——那是「复用同一实例」才有的假象。
  return [JSON.parse(JSON.stringify(v)), undefined];
}

function import_text(urls: string[]): [unknown, unknown] {
  push(`imp:text|${urls.join(",")}`);
  const p = urls[0]!;
  const f = tfails.get(p);
  if (f !== undefined) throw new Error(f);
  if (clear_at.has(`text:${p}`)) mgr.clear();
  const v = tfiles.get(p);
  if (v === undefined) fail(`unscripted import_as_text '${p}'`);
  return [v, undefined];
}

function install_ditto(): void {
  const render_args = (prefix: string, args: unknown[]): string =>
    args.length > 1
      ? `${prefix}:${renderValue(args[0])}:${renderValue(args[1])}`
      : `${prefix}:${renderValue(args[0])}`;
  Ditto.setup({
    warn: (...args: unknown[]) => push(render_args("warn", args)),
    error: (...args: unknown[]) => push(render_args("error", args)),
    XML: {
      parse(text: string) {
        push(`xml:${String(text)}`);
        const t = trees.get(String(text));
        if (t === undefined) fail(`unscripted xml_parse '${text}'`);
        return t as never;
      },
    },
    Importer: {
      async import_as_json(urls: string[]) {
        return import_json(urls) as never;
      },
      async import_as_text(urls: string[]) {
        return import_text(urls) as never;
      },
      async import_as_blob_url() {
        fail("unscripted import_as_blob_url");
      },
      async import_as_array_buffer() {
        fail("unscripted import_as_array_buffer");
      },
      async import_as_image_bitmap() {
        fail("unscripted import_as_image_bitmap");
      },
    },
  } as never);
}

const fake_lfw = {
  resources,
  images: {
    load_img: (path: string) => {
      push(`img:${path}`);
      if (clear_img === path) mgr.clear();
    },
    // `preprocess_entity_data` 的 `files` 会调它；端口不落地加载任务 ⇒ 假件静默。
    load_by_pic_info: () => undefined,
  },
  mt,
  emit_progress: (content: string, progress: number) => {
    push(`prog:${content}|${renderValue(progress)}`);
  },
};

function id_of(v: unknown): string {
  return String((v as Rec | undefined)?.id);
}
function ids_of(list: unknown[]): string {
  return list.map((v) => id_of(v)).join(",");
}
function hit_id(v: unknown): string {
  return v === undefined || v === null ? "u" : id_of(v);
}

async function do_load(paths: string[]): Promise<void> {
  try {
    await mgr.load(paths);
    push("load:ok");
  } catch (e) {
    push(`load:fail:${e instanceof Error ? e.message : String(e)}`);
  }
}

function dump(): void {
  push(
    `dump|inner=${mgr.inner_id}` +
      `|bots=${ids_of(mgr.bots)}` +
      `|moves=${ids_of(mgr.moves)}` +
      `|objects=${ids_of(mgr.objects)}` +
      `|fighters=${ids_of(mgr.fighters)}` +
      `|weapons=${ids_of(mgr.weapons)}` +
      `|balls=${ids_of(mgr.balls)}` +
      `|entities=${ids_of(mgr.entities)}` +
      `|bgs=${ids_of(mgr.backgrounds)}` +
      `|stages=${ids_of(mgr.stages)}`,
  );
}

async function main(): Promise<void> {
  const casePath = process.argv[2];
  if (!casePath) fail("usage: lfw_trace_dat_mgr.mjs <case-file>");
  install_ditto();
  mgr = new DatMgr(fake_lfw as never);

  for (const raw of readCaseLines(casePath)) {
    const t = splitWs(raw);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;
    const arg = (): unknown => parseValue(t, i);

    if (op === "jfile") {
      jfiles.set(keyOf(next()), arg());
    } else if (op === "xtree") {
      const p = keyOf(next());
      tfiles.set(p, p);
      trees.set(p, build_el(arg()));
    } else if (op === "jfail") {
      jfails.set(keyOf(next()), keyOf(next()));
    } else if (op === "tfail") {
      tfails.set(keyOf(next()), keyOf(next()));
    } else if (op === "clearat") {
      clear_at.add(`${next()}:${keyOf(next())}`);
    } else if (op === "clearimg") {
      const p = keyOf(next());
      clear_img = p === "-" ? "" : p;
    } else if (op === "unhook") {
      clear_at.clear();
      clear_img = "";
    } else if (op === "spark") {
      jfiles.set("data/spark.obj.json5", { id: "spark", type: 4, base: { name: "Spark" } });
    } else if (op === "load") {
      const paths: string[] = [];
      while (i[0]! < t.length) paths.push(keyOf(next()));
      await do_load(paths);
    } else if (op === "dump") {
      dump();
    } else if (op === "find") {
      push(`find:${hit_id(mgr.find(keyOf(next())))}`);
    } else if (op === "botof") {
      const d = mgr.find(keyOf(next())) as Rec | undefined;
      push(`botof:${hit_id(d?.base?.bot)}`);
    } else if (op === "findbot") {
      push(`findbot:${hit_id(mgr.find_bot(keyOf(next())))}`);
    } else if (op === "findmoves") {
      push(`findmoves:${hit_id(mgr.find_moves(keyOf(next())))}`);
    } else if (op === "fwv") {
      push(`fwv:${hit_id(mgr.find_weapon(keyOf(next())))}`);
    } else if (op === "fwpred") {
      const n = Number(next());
      push(`fwpred:${hit_id(mgr.find_weapon((_v, idx) => idx === n))}`);
    } else if (op === "fobjv") {
      push(`fobjv:${hit_id(mgr.find_object(keyOf(next())))}`);
    } else if (op === "fentv") {
      push(`fentv:${hit_id(mgr.find_entity(keyOf(next())))}`);
    } else if (op === "ffv") {
      push(`ffv:${hit_id(mgr.find_fighter(keyOf(next())))}`);
    } else if (op === "fbgv") {
      push(`fbgv:${hit_id(mgr.find_background(keyOf(next())))}`);
    } else if (op === "bgh") {
      const d = mgr.find_background(keyOf(next())) as Rec | undefined;
      push(`bgh:${d === undefined ? "u" : String(d?.base?.height)}`);
    } else if (op === "stg") {
      const id = keyOf(next());
      const s = mgr.stages.find((v) => (v as Rec)?.id === id) as Rec | undefined;
      push(`stg:${s === undefined ? "u" : String(s?.name)}`);
    } else if (op === "stgz") {
      const id = keyOf(next());
      const s = mgr.stages.find((v) => (v as Rec)?.id === id);
      push(`stgz:${s === undefined ? "u" : renderValue(s)}`);
    } else if (op === "botst") {
      const b = mgr.find_bot(keyOf(next())) as Rec | undefined;
      push(`botst:${b === undefined ? "u" : String(Object.keys(b?.states ?? {}).length)}`);
    } else if (op === "objg") {
      push(`objg:${ids_of(mgr.get_objects_of_group(keyOf(next())))}`);
    } else if (op === "fg") {
      push(`fg:${ids_of(mgr.get_fighters_of_group(keyOf(next())))}`);
    } else if (op === "wg") {
      push(`wg:${ids_of(mgr.get_weapons_of_group(keyOf(next())))}`);
    } else if (op === "fng") {
      push(`fng:${ids_of(mgr.get_fighters_not_in_group(keyOf(next())))}`);
    } else if (op === "bgg") {
      push(`bgg:${ids_of(mgr.get_backgrouds_of_group(keyOf(next())))}`);
    } else if (op === "randg") {
      const r = mgr.get_randoming_by_group(keyOf(next())) as unknown as Rec;
      const got = (r as { get: () => unknown }).get();
      push(`randg|${String(r["name"])}|${ids_of(r["_src"] as unknown[])}|${hit_id(got)}`);
    } else if (op === "randgc") {
      const key = keyOf(next());
      const r1 = mgr.get_randoming_by_group(key);
      const r2 = mgr.get_randoming_by_group(key);
      const got = r2.get() as unknown;
      push(`randgc:${r1 === r2 ? 1 : 0}|${hit_id(got)}`);
    } else if (op === "bgr") {
      const groups = keyOf(next()).split(",");
      const r = mgr.get_bg_randoming_of_group(groups) as unknown as Rec;
      const got = (r as { get: () => unknown }).get();
      push(`bgr|${String(r["name"])}|${ids_of(r["_src"] as unknown[])}|${hit_id(got)}`);
    } else if (op === "rbg") {
      const groups = keyOf(next()).split(",");
      push(`rbg:${hit_id(mgr.get_random_bg(groups))}`);
    } else if (op === "ctrls") {
      push(`ctrls:${[...Factory.ctrl_creators.keys()].map((k) => renderValue(k)).join(",")}`);
    } else if (op === "mkctrl") {
      const oid = keyOf(next());
      try {
        const c = factory.create_ctrl(oid, "p1", fake_entity as never);
        if (c === undefined) {
          push("mkctrl:u");
        } else {
          const label =
            c instanceof BallController ? "ball" : c instanceof BotController ? "bot" : "?";
          push(`mkctrl:${label}:${c.player_id}`);
        }
      } catch (e) {
        push(`mkctrl:throw:${e instanceof Error ? e.message : String(e)}`);
      }
    } else if (op === "clear") {
      mgr.clear();
      push(`clear:${mgr.inner_id}`);
    } else if (op === "dispose") {
      mgr.dispose();
      push(`dispose:${mgr.inner_id}`);
    } else if (op === "innerid") {
      push(`innerid:${mgr.inner_id}`);
    } else {
      fail(`unknown op '${op}'`);
    }
  }

  process.stdout.write(log.join("\n") + "\n");
}

void main();
