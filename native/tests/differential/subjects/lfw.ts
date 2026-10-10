// `LFW`（门面 4AB）的 TS 侧台面，op 与 `subjects/lfw.cpp` 一一对应。
//
// `Ditto` 的宿主包全脚本化（Sounds/ImageMgr/Keyboard/Pointings/UIInputHandle/WorldRender/
// Cache/Zip/Clock/Render/Timeout/…）；`Date.now` 固定成 12345（端口侧 `host.now()`）。
import { Ditto } from "../../../../src/LFW/ditto/Instance";
import { ImageInfo } from "../../../../src/LFW/ditto";
import { Expression } from "../../../../src/LFW/base/Expression";
import { Factory } from "../../../../src/LFW/Factory";
import { LFW } from "../../../../src/LFW/LFW";
import { Randoming } from "../../../../src/LFW/helper/Randoming";
import { MersenneTwister } from "../../../../src/LFW/utils/math/MersenneTwister";
import { get_val_getter_from_stage } from "../../../../src/LFW/loader/get_val_getter_from_stage";
import { PlayerInfo } from "../../../../src/LFW/PlayerInfo";
import { Stage } from "../../../../src/LFW/stage/Stage";
import { cook_ui_info, find_ui_template, merge_ui_template } from "../../../../src/LFW/ui/cook_ui_info";
import { LFWKeyEvent } from "../../../../src/LFW/ui/LFWKeyEvent";
import { LFWPointerEvent } from "../../../../src/LFW/ui/LFWPointerEvent";
import { UIImgLoader } from "../../../../src/LFW/ui/UIImgLoader";
import { Style } from "../../../../src/LFW/ui/Style";
import { UILayers, type IPopPageOpts, type UILayer } from "../../../../src/LFW/ui/UILayer";
import { actor } from "../../../../src/LFW/ui/action/Actor";
import { UIComponent } from "../../../../src/LFW/ui/component/UIComponent";
import { SmoothNumber } from "../../../../src/LFW/ui/component/SmoothNumber";
import { TextInput } from "../../../../src/LFW/ui/component/TextInput";
import { PlayerName } from "../../../../src/LFW/ui/component/PlayerName";
import { PlayerTeamName } from "../../../../src/LFW/ui/component/PlayerTeamName";
import { FighterName } from "../../../../src/LFW/ui/component/FighterName";
import { IntegerPicker } from "../../../../src/LFW/ui/component/IntegerPicker";
import { Alignment } from "../../../../src/LFW/ui/component/Alignment";
import { Players } from "../../../../src/LFW/ui/component/Players";
import { Sounds } from "../../../../src/LFW/ui/component/Sounds";
import { LabelButton } from "../../../../src/LFW/ui/component/LabelButton";
import { PlayerCtrlType } from "../../../../src/LFW/ui/component/PlayerCtrlType";
import { WeaponRain } from "../../../../src/LFW/ui/component/WeaponRain";
import { FighterHead } from "../../../../src/LFW/ui/component/FighterHead";
import { FighterThumb } from "../../../../src/LFW/ui/component/FighterThumb";
import { ComponentsPlayer } from "../../../../src/LFW/ui/component/ComponentsPlayer";
import { StageSwitcher } from "../../../../src/LFW/ui/component/StageSwitcher";
import { BackgroundSwitcher } from "../../../../src/LFW/ui/component/BackgroundSwitcher";
import { Flex } from "../../../../src/LFW/ui/component/Flex";
import { Jalousie } from "../../../../src/LFW/ui/component/Jalousie";
import { Dialogs } from "../../../../src/LFW/ui/component/Dialogs";
import { BGMSwitcher } from "../../../../src/LFW/ui/component/BGMSwitcher";
import { FSM } from "../../../../src/LFW/base/FSM";
import { ModeState } from "../../../../src/LFW/ui/component/ModeState";
import {
  GameModeFSMState_Running,
  GameModeFSMState_BeforeEnd,
  GameModeFSMState_End,
} from "../../../../src/LFW/ui/component/GameModeFSMState";
import { UINode } from "../../../../src/LFW/ui/UINode";
import { ui_load_img } from "../../../../src/LFW/ui/ui_load_img";

import { esc, keyOf, parseValue, readCaseLines, renderValue, splitWs } from "./trace_util";

type Rec = Record<string, any>;

const log: string[] = [];
function push(s: string): void {
  log.push(s);
}
function fail(msg: string): never {
  process.stderr.write(msg + "\n");
  process.exit(2);
}

const render_value = (v: unknown): string => renderValue(v);
const num = (n: number): string => renderValue(n);

// `renderValue` 的环安全版（cook 的 `items` 会挂 parent 指针形成环；DAG 共用照常展开）。
function render_cycle_safe(v: unknown, stack: Set<unknown> = new Set()): string {
  if (v !== null && typeof v === "object") {
    if (stack.has(v)) return "~circ";
    stack.add(v);
    let out: string;
    if (Array.isArray(v)) {
      out = "[" + v.map((x) => render_cycle_safe(x, stack)).join(",") + "]";
    } else {
      const o = v as Record<string, unknown>;
      out =
        "{" +
        Object.keys(o)
          .map((k) => `${esc(k)}:${render_cycle_safe(o[k], stack)}`)
          .join(",") +
        "}";
    }
    stack.delete(v);
    return out;
  }
  return renderValue(v);
}

let clock_ms = 0;

// `imp`/`lzadd` 的 kind 迷你语言（两侧一致）：`-`/`o`/`a`/`k:<name>`/`str:<v>`/`e`（失败，抛 "boom"）。
const imports = new Map<string, () => unknown>();
const import_fails = new Map<string, string>();
// 4AJ：`find_ui_template` 要吞的「ImportError 形状」失败（C++ 侧就是 `impfail`）。
const import_soft_fails = new Set<string>();
// 4AJ：`uinew`/`uinest` 的 UI 值脚本。
const ui_vals = new Map<string, Rec>();
// 4AK：`imgset` 的图片脚本（FakeImageMgr.load_img 按 img_key 查）。
const image_vals = new Map<string, unknown>();

// 4AM：事件层与 UIImgLoader 的脚本。
const pevs = new Map<string, LFWPointerEvent>();
const kevs = new Map<string, LFWKeyEvent>();
const img_nodes = new Map<string, Rec>();
const loaders = new Map<string, UIImgLoader>();
// 4AX：SmoothNumber 脚本。
const smooths = new Map<string, SmoothNumber>();
const flush_microtasks = async (): Promise<void> => {
  for (let k = 0; k < 12; k++) await Promise.resolve();
};
// 4AN：UINode 脚本。
const tnodes = new Map<string, UINode>();

// 4AP：真 `UILayers`（台面直建，与 `lfw.layers` 的补丁面互不干扰）。
const real_layers: { v: UILayers | null } = { v: null };

function tree_str(n: UINode): string {
  let s = `${String(n.id ?? "u")}(${n.children.length})`;
  if (n.children.length) s += "[" + n.children.map((c) => tree_str(c as UINode)).join(",") + "]";
  return s;
}

function nref(p: UINode | undefined | null): string {
  return p ? renderValue(p.id) : "u";
}

// 4AS：台面假组件（与 C++ 侧 `bench::FakeComp` 逐字同日志）。
function iref(v: unknown): string {
  if (v instanceof UINode) return "node:" + String(v.id);
  if (v instanceof UIComponent) return "comp:" + (v as Rec).f_name + "#" + (v as Rec).id;
  return renderValue(v);
}

class FakeComponent extends UIComponent<any> {
  static readonly TAGS = ["FakeComp"];
  static readonly PROPS = {
    n: { key: "n", type: "number" },
    s: { key: "s", type: "string" },
    pick: { key: "pick", type: "string", nullable: true },
    ok: { key: "ok", type: "boolean", nullable: true },
    arr: { key: "arr", type: "array", nullable: true, items: { key: "arr", type: "number" } },
    strs: { key: "strs", type: "array", nullable: true, items: { key: "strs", type: "string" } },
    b0: { key: "b0", type: "string", nullable: true },
    sub: { key: "sub", type: UINode },
    other: { key: "other", type: UIComponent },
    stop_click: { key: "stop_click", type: "boolean", nullable: true },
    stop_key: { key: "stop_key", type: "string", nullable: true },
    del_at: { key: "del_at", type: "number", nullable: true },
  };
  private _updates = 0;
  // 4BK：FSM 宿主（`GameModeFSMState` 的 `owner.fsm`）。
  fsm?: Rec;
  private tag(): string {
    return `fc|${this.f_name}`;
  }
  dump(): void {
    const f = `${this.tag()}|props`;
    try {
      void this.props;
    } catch (e) {
      push(`${f}|err|${((e as Rec).errors ?? []).join("|")}`);
      return;
    }
    const h = this.props_holder;
    let line = `${f}|n=${renderValue(h.num("n"))}|s=${renderValue(h.str("s"))}|pick=${renderValue(
      h.str("pick", ["a", "b"])
    )}|ok=${renderValue(h.bool("ok"))}`;
    try {
      const a = h.nums("arr", 2);
      line += `|arr=${a ? a.join(",") : "z"}`;
    } catch {
      line += "|arr=err";
    }
    const ss = h.strs("strs");
    line += `|strs=${ss ? ss.join("+") : "z"}`;
    const ls = h.strs("s");
    line += `|lstrs=${ls ? ls.join("+") : "z"}`;
    line += `|b0=${renderValue(h.bool("b0"))}`;
    line += `|sub=${iref((this.props as Rec).sub)}|other=${iref((this.props as Rec).other)}`;
    push(line);
  }
  override init(): void {
    push(`${this.tag()}|init|${this.id}`);
  }
  override on_add(): void {
    push(`${this.tag()}|add|${this.id}`);
    this.dump();
  }
  override on_del(): void {
    push(`${this.tag()}|del|${this.id}`);
  }
  override on_start(): void {
    push(`${this.tag()}|start`);
    push(`${this.tag()}|lrud|${this.LR},${this.UD}`);
  }
  override on_stop(): void {
    push(`${this.tag()}|stop`);
  }
  override on_resume(): void {
    push(`${this.tag()}|resume`);
    push(`${this.tag()}|lrud|${this.LR},${this.UD}`);
  }
  override on_pause(): void {
    push(`${this.tag()}|pause`);
  }
  override on_show(): void {
    push(`${this.tag()}|show`);
  }
  override on_hide(): void {
    push(`${this.tag()}|hide`);
  }
  override on_foucs(): void {
    push(`${this.tag()}|foucs`);
  }
  override on_blur(): void {
    push(`${this.tag()}|blur`);
  }
  override on_click(e: LFWPointerEvent): void {
    push(`${this.tag()}|click|${num(e.button)}`);
    if (this.props_holder.bool("stop_click")) e.stop_immediate_propagation();
  }
  override on_key_down(e: LFWKeyEvent): void {
    push(`${this.tag()}|kdown|${renderValue(e.game_key)}`);
    if (this.props_holder.str("stop_key") === e.game_key) e.stop_immediate_propagation();
  }
  override on_key_up(e: LFWKeyEvent): void {
    push(`${this.tag()}|kup|${renderValue(e.game_key)}`);
  }
  override on_pointer_down(): void {
    push(`${this.tag()}|pdown`);
  }
  override on_pointer_move(): void {
    push(`${this.tag()}|pmove`);
  }
  override on_pointer_up(): void {
    push(`${this.tag()}|pup`);
  }
  override on_pointer_cancel(): void {
    push(`${this.tag()}|pcancel`);
  }
  override on_pointer_leave(): void {
    push(`${this.tag()}|pleave`);
  }
  override on_pointer_enter(): void {
    push(`${this.tag()}|penter`);
  }
  override update(): void {
    this._updates += 1;
    push(`${this.tag()}|update|${num(this._updates)}`);
    if (this.props_holder.num("del_at") === this._updates) {
      push(`${this.tag()}|delreq`);
      this.node.del_components(this);
    }
  }
}

Factory.register_component(FakeComponent as never);

function make_img_node(lfw_: LFW): Rec {
  const node: Rec = {
    lfw: lfw_,
    _image: null,
    resize(w: number, h: number): void {
      push(`imnode:resize|${num(w)}|${num(h)}`);
    },
  };
  Object.defineProperty(node, "image", {
    get() {
      return node["_image"];
    },
    set(v: unknown) {
      node["_image"] = v;
      push(`imnode:image|${renderValue(v)}`);
    },
  });
  return node;
}

// 4AD：URL 流程脚本。
const zip_stored = new Map<string, string>(); // `zip_url|md5` → blob token
const blob_zip = new Map<string, string>(); // blob/data token → zid
const dl_scripts = new Map<string, { mode: string; token: string; md5?: string; size: number }>();
const cache_vals = new Map<string, unknown>();
const cache_late = new Map<string, unknown>();
let cache_log = false;

function kind_value(k: string): unknown {
  if (k === "o") return {};
  if (k === "a") return [];
  if (k.startsWith("k:")) return { [k.slice(2)]: "1" };
  if (k.startsWith("str:")) return k.slice(4);
  if (k.startsWith("md5:")) return { md5: k.slice(4) };
  if (k.startsWith("w:")) return { "": { [k.slice(2)]: "1" } };
  if (k === "spk") return { id: "spark", type: 4, base: { name: "Spark" } };
  // 4BG：切换器夹具。索引条目是**文件描述**（`{id,file}` / 路径串），真内容走 `imp` 缝。
  const bg_fixture = (id: string, name: string, group: string | null) => ({
    type: "background",
    id,
    base: {
      name,
      shadow: "",
      height: 550,
      left: 0,
      right: 1588,
      far: -40,
      near: 0,
      group: group ? [group] : [],
    },
    layers: [],
  });
  const stage_fixture = (
    id: string,
    name: string,
    bg_id: string,
    group: string | null,
    start_name: string | null,
  ) => ({
    id,
    name,
    bg: bg_id,
    ...(start_name !== null ? { is_starting: true, starting_name: start_name } : {}),
    group: group ? [group] : [],
    phases: [],
  });
  if (k === "bgr") return bg_fixture("bg_reg", "B1", "regular");
  if (k === "bgh") return bg_fixture("bg_hid", "B2", "hidden");
  if (k === "bgo") return bg_fixture("bg_oth", "B3", null);
  // `data.stages` 的条目是 `{file, skipped}` 描述符；文件内容是**列表**。
  if (k === "srg") return [stage_fixture("st_reg", "S1", "bg_reg", null, "SR")];
  if (k === "srh") return [stage_fixture("st_hid", "S2", "bg_hid", "hidden", null)];
  if (k === "srd") return [stage_fixture("st_dev", "S3", "bg_oth", "Dev", "DD")];
  if (k === "swf") {
    return {
      backgrounds: [
        { id: "bg_reg", file: "bg/reg.json5" },
        { id: "bg_hid", file: "bg/hid.json5" },
        { id: "bg_oth", file: "bg/oth.json5" },
      ],
      stages: [{ file: "st/reg.json5" }, { file: "st/hid.json5" }, { file: "st/dev.json5" }],
    };
  }
  if (k === "ftr") {
    // ⚠ 不带 head：端口预处理不落图片任务 ⇒ 带 head 会多 img:load 漂移。
    return { id: "fg1", type: 8, base: { name: "FIGHTER ONE" }, frames: [] };
  }
  if (k === "idg") {
    return {
      objects: [{ id: "fg1", file: "data/fg1.json5" }],
    };
  }
  return undefined;
}

function err_msg(e: unknown): string {
  return typeof e === "string" ? e : e instanceof Error ? e.message : String(e);
}

// `cook` 给数据挂的 `xml*` 访问器/函数：C++ 侧不建形 ⇒ 渲染前剥掉（两边 warn 输出才能同形）。
function safe_render(v: unknown): unknown {
  if (Array.isArray(v)) return v.map(safe_render);
  if (v !== null && typeof v === "object") {
    const out: Rec = {};
    for (const k of Object.keys(v as Rec)) {
      if (k === "xml" || k === "xml_roundtrip" || k === "xml_roundtrip_ok") continue;
      const val = (v as Rec)[k];
      if (typeof val === "function") continue;
      out[k] = safe_render(val);
    }
    return out;
  }
  return v;
}

function install_ditto(): void {
  class FakeSounds {
    static current?: FakeSounds;
    bgm_name: string | null = null;
    is_random = false;
    readonly callbacks_list: Rec[] = [];
    callbacks = {
      add: (cb: unknown) => {
        if (!this.callbacks_list.includes(cb as Rec)) this.callbacks_list.push(cb as Rec);
        push("snd_cbadd");
      },
      del: (cb: unknown) => {
        const k = this.callbacks_list.indexOf(cb as Rec);
        if (k >= 0) this.callbacks_list.splice(k, 1);
        push("snd_cbdel");
      },
    };
    private _prev_url: string | null = null;
    private rand?: Randoming<string>;
    private readonly _lfw: Rec;
    constructor(lfw_: unknown) {
      FakeSounds.current = this;
      this._lfw = lfw_ as Rec;
      this.rand = new Randoming<string>("bgm_randoming", this._lfw["bgms"] as string[], this._lfw["mt"]);
      snd_probe = () => `${this.bgm_name ?? ""}|${this.is_random ? 1 : 0}`;
      push("snd_init");
    }
    private fire(bgm: string | null, prev: string | null): void {
      for (const cb of [...this.callbacks_list]) cb.on_bgm_changed?.(bgm, prev, this);
    }
    bgm(): string | null {
      return this.bgm_name;
    }
    dispose(): void {
      this.callbacks_list.length = 0;
      push("snd_dispose");
    }
    play_bgm(name: string): () => void {
      push("snd_bgm");
      if (this._prev_url === name) return () => undefined;
      const prev = this.bgm_name;
      const real = name === "?" ? (this.rand!.set_src(this._lfw["bgms"] as string[]).get() as string | undefined) : name;
      this.bgm_name = null;
      this._prev_url = null;
      if (!real) return () => undefined;
      this.bgm_name = real;
      this._prev_url = real;
      this.is_random = name === "?";
      this.fire(real, prev);
      return () => undefined;
    }
    stop_bgm(): void {
      if (this.bgm_name === null) return;
      const prev = this.bgm_name;
      this.bgm_name = null;
      this._prev_url = null;
      push("snd_stop");
      this.fire(null, prev);
    }
    play(): void {
      push("snd_play");
    }
    play_with_load(): void {
      push("snd_load");
    }
    play_preset(name: unknown, x: unknown, y: unknown, z: unknown): void {
      push(`snd_preset|${renderValue(name)}|${renderValue(x)}|${renderValue(y)}|${renderValue(z)}`);
    }
  }
  class FakeImageMgr {
    constructor(_lfw: unknown) {
      push("img_init");
    }
    load_img(key: string, path?: unknown, ops?: unknown): unknown {
      // 两参以下的调用是 DatMgr 那条（只打一行）；三参是 `ui_load_img`（带 ops）。
      if (ops === undefined) {
        push("img:load|" + key);
        return "";
      }
      push(`img:load|${key}|${renderValue(path)}|${renderValue(ops)}`);
      if (!image_vals.has(key)) throw "unscripted image";
      return image_vals.get(key);
    }
    pin(key: string): void {
      push("img:pin|" + key);
    }
    measure_text(text: unknown, style: unknown): unknown {
      const norm = style instanceof Style ? (style as unknown as Rec).data : style;
      push(`measure|${renderValue(text)}|${renderValue(norm)}`);
      const t = String(text ?? "");
      return { text: t, w: t.length * 4, h: 10, scale: 1 };
    }
  }
  class FakeKeyboard {
    readonly callbacks: Rec[] = [];
    callback = {
      add: (cb: unknown) => {
        if (!this.callbacks.includes(cb as Rec)) this.callbacks.push(cb as Rec);
        push("kbd_cbadd");
      },
      del: (cb: unknown) => {
        const k = this.callbacks.indexOf(cb as Rec);
        if (k >= 0) this.callbacks.splice(k, 1);
        push("kbd_cbdel");
      },
    };
    constructor(_lfw: unknown) {
      kbd_dispatch = (dir, e) => {
        for (const cb of [...this.callbacks]) {
          if (dir === "down") cb.on_key_down?.(e);
          else cb.on_key_up?.(e);
        }
      };
      push("kbd_init");
    }
    dispose(): void {
      this.callbacks.length = 0;
      push("kbd_dispose");
    }
  }
  class FakePointings {
    readonly callbacks: Rec[] = [];
    callback = {
      add: (cb: unknown) => {
        if (!this.callbacks.includes(cb as Rec)) this.callbacks.push(cb as Rec);
        push("pt_cbadd");
      },
      del: (cb: unknown) => {
        const k = this.callbacks.indexOf(cb as Rec);
        if (k >= 0) this.callbacks.splice(k, 1);
        push("pt_cbdel");
      },
    };
    constructor() {
      pt_dispatch_click = (e) => {
        for (const cb of [...this.callbacks]) cb.on_click?.(e);
      };
      push("pt_init");
    }
    dispose(): void {
      this.callbacks.length = 0;
      push("pt_dispose");
    }
  }
  class FakeUIInputHandle {
    constructor(_lfw: unknown) {}
  }
  class FakeWorldRender {
    constructor(_world: unknown) {
      push("wr_init");
    }
    add_entity(): void {}
    del_entity(): void {}
    render(): void {}
    dispose(): void {}
  }
  const cache = {
    forget(type: string, version: number) {
      push(`cache:forget|${type}|${num(version)}`);
      return Promise.resolve();
    },
    async get(name: string) {
      if (cache_log) push(`cache:get|${name}`);
      if (cache_late.has(name)) {
        const v = cache_late.get(name);
        cache_late.delete(name);
        cache_vals.set(name, v);
        return undefined;
      }
      return cache_vals.get(name);
    },
    put(entry: Rec) {
      if (cache_log) push(`cache:put|${String(entry?.["name"])}`);
    },
    async del(name: string, version: string) {
      if (cache_log) push(`cache:del|${name}|${version}`);
    },
  };
  const zip = {
    forget_stored(type: string, version: number) {
      push(`zip:forget|${type}|${num(version)}`);
      return Promise.resolve();
    },
    async get_stored(url: string, md5: string) {
      push(`zip:get_stored|${url}|${md5}`);
      return zip_stored.get(`${url}|${md5}`);
    },
    async read_blob(name: string, blob: unknown, md5: unknown) {
      push(`zip:read_blob|${name}|${String(md5)}`);
      return resolve_token(blob, "unscripted blob");
    },
    async read_buf(name: string, data: unknown) {
      push(`zip:read_buf|${name}`);
      return resolve_token(data, "unscripted buf");
    },
    async download(url: string, progress: (p: number, s: number) => void) {
      push(`zip:download|${url}`);
      const s = dl_scripts.get(url);
      if (!s) throw "unscripted download";
      if (s.mode === "fail") throw "dl-fail";
      progress(50, s.size);
      return { stored: s.mode === "stored", blob: s.token, md5: s.md5 };
    },
  };
  const resolve_token = (token: unknown, msg: string): unknown => {
    if (typeof token !== "string") throw msg;
    const zid = blob_zip.get(token);
    if (zid === undefined) throw msg;
    const z = lzips.get(zid);
    if (!z) throw msg;
    return z;
  };
  const importer = {
    async import_as_json(urls: string[]) {
      for (const url of urls) {
        const key = url.split("?")[0]!;
        push(`imp:json|${key}`);
        if (import_soft_fails.has(key)) throw { is_make_import_error: true, message: "soft" };
        const fail = import_fails.get(key);
        if (fail !== undefined) throw fail;
        if (imports.has(key)) return [imports.get(key)!(), url];
      }
      throw "unscripted import";
    },
    async import_as_text(_urls: string[]) {
      throw { is_make_import_error: true, message: "no text" };
    },
  };
  const clock = {
    ms: 0,
    next: 1,
    handles: new Map<number, () => void>(),
    now: () => clock_ms,
    add(h: () => void) {
      const id = clock.next++;
      clock.handles.set(id, h);
      return id;
    },
    del(id: number) {
      clock.handles.delete(id);
    },
    hidden: () => false,
  };
  const render = {
    add(h: () => void) {
      return clock.add(h);
    },
    del(id: number) {
      clock.del(id);
    },
    raf(h: () => void) {
      return clock.add(h);
    },
    caf(id: number) {
      clock.del(id);
    },
  };
  Ditto.setup({
    Clock: clock as never,
    Render: render as never,
    Timeout: {
      next: 1,
      add(_h: () => void, _ms: number) {
        return clock.next++;
      },
      del(_id: number) {},
    } as never,
    Interval: {
      next: 1,
      add(_h: () => void, _ms: number) {
        return clock.next++;
      },
      del(_id: number) {},
    } as never,
    MD5: (s: unknown) => "h:" + String(s),
    JSON5: { parse: (s: string) => JSON.parse(s), stringify: (v: unknown) => JSON.stringify(v) },
    Zip: zip as never,
    Sounds: FakeSounds as never,
    Keyboard: FakeKeyboard as never,
    Pointings: FakePointings as never,
    FullScreen: class {} as never,
    Importer: importer as never,
    Cache: cache as never,
    Vector3: class Vec3 {
      x: number;
      y: number;
      z: number;
      constructor(x = 0, y = 0, z = 0) {
        this.x = x;
        this.y = y;
        this.z = z;
      }
      set(x = 0, y = 0, z = 0): unknown {
        this.x = x;
        this.y = y;
        this.z = z;
        return this;
      }
      equals(o: Vec3): boolean {
        return this.x === o.x && this.y === o.y && this.z === o.z;
      }
      clone(): Vec3 {
        return new Vec3(this.x, this.y, this.z);
      }
      sub(o: Vec3): Vec3 {
        return new Vec3(this.x - o.x, this.y - o.y, this.z - o.z);
      }
    } as never,
    Vector2: class {
      x = 0;
      y = 0;
    } as never,
    WorldRender: FakeWorldRender as never,
    UINodeRenderer: class {
      constructor(private readonly uinode: UINode) {}
      private ref(): string {
        return nref(this.uinode);
      }
      del_self(): void { push(`rend|del_self|${this.ref()}`); }
      on_start(): void { push(`rend|on_start|${this.ref()}`); }
      on_stop(): void { push(`rend|on_stop|${this.ref()}`); }
      on_resume(): void { push(`rend|on_resume|${this.ref()}`); }
      on_pause(): void { push(`rend|on_pause|${this.ref()}`); }
      on_show(): void { push(`rend|on_show|${this.ref()}`); }
      on_hide(): void { push(`rend|on_hide|${this.ref()}`); }
      on_foucs(): void { push(`rend|on_foucs|${this.ref()}`); }
      on_blur(): void { push(`rend|on_blur|${this.ref()}`); }
    } as never,
    ImageMgr: FakeImageMgr as never,
    UIInputHandle: FakeUIInputHandle as never,
    XML: {} as never,
    warn: (...args: unknown[]) => push("warn|" + args.map((a) => render_value(safe_render(a))).join("~")),
    error: (...args: unknown[]) =>
      push("error|" + args.map((a) => render_value(safe_render(a))).join("~")),
    Log: (...args: unknown[]) => push("Log|" + args.map((a) => render_value(safe_render(a))).join("~")),
    // 端口里 `debug` 是空转（同 DESIGN 记录）；`warn`/`Log` 有真通道，`debug` 不收。
    debug: () => undefined,
    DEV: false,
    IsDesktop: false,
    alert: () => undefined,
  });
}

const CB_KEYS = [
  "on_ui_changed",
  "on_loading_start",
  "on_loading_end",
  "on_loading_failed",
  "on_progress",
  "on_bgms_loaded",
  "on_bgms_clear",
  "on_player_infos_changed",
  "on_cheat_changed",
  "on_stage_pass",
  "on_enter_next_stage",
  "on_dispose",
  "on_ui_loaded",
  "on_prel_loaded",
  "on_lang_changed",
  "on_broadcast",
  "on_survival_rank_changed",
  "on_zips_changed",
  "on_component_broadcast",
  "on_extra_zips_changed",
  "controller_detected",
  "keyboard_detected",
];

let lfw: LFW;
// `FakeKeyboard` / `FakePointings` 在 `install_ditto` 里声明， 用模块级钩子接出分发入口。
let kbd_dispatch: (dir: "down" | "up", e: Rec) => void = () => undefined;
let pt_dispatch_click: (e: Rec) => void = () => undefined;
// 假声音的状态探针（`sndbgm` op）：`<bgm>|<is_random>`。
let snd_probe: () => string = () => "|0";

function listen(): void {
  for (const name of CB_KEYS) {
    (lfw.callbacks as unknown as Rec).on(name, (...args: unknown[]) => {
      const parts: string[] = [`cb|${name}`];
      if (name === "on_ui_changed") {
        for (const a of args) parts.push(a == null ? "u" : "?");
        push(parts.join("|"));
        return;
      }
      for (const a of args) {
        if (a === lfw) parts.push("self");
        else if (name === "on_ui_loaded") parts.push(String((a as unknown[]).length));
        else if (name === "on_zips_changed")
          parts.push((a as { name: string }[]).map((v) => v.name).join(","));
        else if (name === "on_broadcast") parts.push("s:" + String(a));
        else if (a === undefined || a === null) parts.push("u");
        else if (a instanceof PlayerInfo) parts.push("pl:" + String(a.id));
        else if (typeof a === "number") parts.push("n:" + num(a));
        else if (typeof a === "boolean") parts.push("b:" + (a ? 1 : 0));
        else if (typeof a === "string") parts.push("s:" + a);
        else parts.push(render_value(a));
      }
      push(parts.join("|"));
    });
  }
}

function team_token(t: string): string | undefined {
  if (t === "-") return undefined;
  return t;
}

// ---- 4AC：加载流程的脚本化假 zip ----
class LzObject {
  constructor(
    private readonly path: string,
    private readonly kind: string,
  ) {}
  get name(): string {
    return this.path;
  }
  async json(): Promise<unknown> {
    push(`lz:json|${this.path}`);
    if (this.kind === "e") throw "boom";
    return kind_value(this.kind);
  }
  async text(): Promise<unknown> {
    push(`lz:text|${this.path}`);
    if (this.kind === "e") throw "boom";
    const v = kind_value(this.kind);
    return typeof v === "string" ? v : v === undefined ? undefined : JSON.stringify(v);
  }
}

class Lz {
  zid = "";
  name = "";
  md5?: string;
  entries: { path: string; kind: string }[] = [];
  file(path: string | RegExp): unknown {
    if (typeof path === "string") {
      push(`lz:file|${this.zid}|${path}`);
      const e = this.entries.find((v) => v.path === path);
      return e ? new LzObject(e.path, e.kind) : null;
    }
    push(`lz:rgx|${this.zid}|${path.source}`);
    return this.entries
      .filter((v) => {
        path.lastIndex = 0;
        return path.test(v.path);
      })
      .map((v) => new LzObject(v.path, v.kind));
  }
}

const lzips = new Map<string, Lz>();

function dump_info(info: Rec): string {
  return [
    info["type"],
    info["url"],
    info["title"],
    info["description"],
    info["author"],
    info["version"],
    info["time"],
    info["md5"],
  ]
    .map((v) => String(v))
    .join("|");
}

function main(): void {
  install_ditto();
  (Date as unknown as Rec).now = () => 12345;
  Randoming.mt = new MersenneTwister(12345);
  lfw = new LFW(false);
  listen();

  void run_ops().catch((e) => {
    process.stderr.write(err_msg(e) + "\n");
    process.exit(2);
  });
}

async function run_ops(): Promise<void> {
  for (const line of readCaseLines(process.argv[2]!)) {
    const t = splitWs(line);
    if (t.length === 0) continue;
    const op = t[0]!;
    const i = [1];
    const next = (): string => t[i[0]!++]!;
    const arg = (): unknown => parseValue(t, i);
    const nextKey = (): string => keyOf(next());

    if (op === "info") {
      const info = LFW.INFO as Rec;
      push(
        `info|${String(info?.["title"])}|${String(info?.["type"])}|${num(Number(info?.["version"]))}|default=${LFW.IS_DEFAULT_INFO ? 1 : 0}|zips=${LFW.ZIPS.length}`,
      );
    } else if (op === "setinfo") {
      const title = nextKey();
      LFW.INFO = {
        type: "FULL",
        version: 1,
        title,
        description: "d",
        author: "a",
        paths: ["prel.zip.json", "data.zip.json", "extra.zip.json"],
      } as never;
    } else if (op === "setzips") {
      LFW.ZIPS = [nextKey(), nextKey()] as never;
    } else if (op === "newid") {
      const n = Number(nextKey());
      for (let k = 0; k < n; ++k) push("newid|" + lfw.new_id);
    } else if (op === "newteam") {
      const n = Number(nextKey());
      for (let k = 0; k < n; ++k) push("newteam|" + lfw.new_team);
    } else if (op === "resetids") {
      lfw.reset_new_id();
    } else if (op === "resetteam") {
      lfw.reset_new_team();
    } else if (op === "player") {
      const id = nextKey();
      const p1 = lfw.player(id);
      const p2 = lfw.player(id);
      push(
        `player|${p1.id}|${p1 === p2 ? 1 : 0}|local=${p1.local ? 1 : 0}|${String(p1.name ?? "u")}|count=${lfw.players.size}`,
      );
    } else if (op === "pkey") {
      const pid = nextKey();
      const name = nextKey();
      const key = nextKey();
      const p = lfw.player(pid);
      let ok = 0;
      try {
        p.set_key(name, key);
        ok = 1;
      } catch (e) {
        void e;
      }
      push(`pkey|${p.id}|${ok}`);
    } else if (op === "pkeys") {
      const pid = nextKey();
      const p = lfw.player(pid);
      push(`pkeys|${p.id}|${render_value(p.keys)}`);
    } else if (op === "kbdown") {
      const key = nextKey();
      const times = Number(nextKey());
      const dev = nextKey();
      const e = {
        key,
        times,
        device_type: dev === "-" ? undefined : dev,
        interrupt() {
          (e as Rec)["_int"] = true;
        },
      };
      kbd_dispatch("down", e as Rec);
      push(`kbdown|${key}|${num(times)}|${dev}|int=${(e as Rec)["_int"] ? 1 : 0}`);
    } else if (op === "kbup") {
      const key = nextKey();
      const e = { key };
      kbd_dispatch("up", e as Rec);
      push(`kbup|${key}`);
    } else if (op === "ptclick") {
      const btn = Number(nextKey());
      const e = {
        x: 0,
        y: 0,
        scene_x: 0,
        scene_y: 0,
        is_pointing_event: true,
        button: btn,
        delta_x: 0,
        delta_y: 0,
      };
      pt_dispatch_click(e as Rec);
      push(`ptclick|${num(btn)}`);
    } else if (op === "cmds") {
      push("cmds|" + lfw.cmds.join(";"));
    } else if (op === "clearcmds") {
      lfw.cmds.length = 0;
    } else if (op === "ischeat") {
      push(`ischeat|${nextKey()}|${lfw.is_cheat(nextKey()) ? 1 : 0}`);
    } else if (op === "setcheat") {
      const name = nextKey();
      const en = t[i[0]!++]!;
      lfw.set_cheat(name, team_to_bool(en));
    } else if (op === "ep") {
      lfw.emit_progress(nextKey(), Number(nextKey()));
    } else if (op === "eps") {
      const c = nextKey();
      const p = Number(nextKey());
      const s = nextKey();
      lfw.emit_progress(c, p, s === "-" ? undefined : Number(s));
    } else if (op === "bcast") {
      lfw.broadcast(nextKey());
    } else if (op === "dataset") {
      (lfw.world.dataset as Rec)[nextKey()] = parseValue(t, i);
    } else if (op === "switchdiff") {
      lfw.switch_difficulty(Number(nextKey()));
    } else if (op === "lang") {
      push(`lang|${lfw.lang}`);
    } else if (op === "setlang") {
      lfw.lang = nextKey();
    } else if (op === "setlangbad") {
      lfw.set_lang(parseValue(t, i) as never);
    } else if (op === "canon") {
      push(`canon|${String(lfw.canonical_lang(nextKey()))}`);
    } else if (op === "i18nadd") {
      const lang = nextKey();
      const key = nextKey();
      const val = nextKey();
      (lfw as Rec)["_i18n"].add({ [lang]: { [key]: val } });
      push(`i18nadd|${lang}|${key}`);
    } else if (op === "str") {
      const n = nextKey();
      push(`str|${n}|${render_value(lfw.string(n))}`);
    } else if (op === "srank") {
      lfw.survival_rank_mode = true;
      lfw.survival_rank_available = true;
      lfw.survival_rank_2p = true;
      lfw.survival_rank_period = "week";
      lfw.set_survival_rank_data({ period: "week", list: [], mine: null });
      push(
        `srank|${lfw.survival_rank_mode ? 1 : 0}${lfw.survival_rank_available ? 1 : 0}${lfw.survival_rank_2p ? 1 : 0}|${lfw.survival_rank_period}|cheated=${lfw.survival_rank_cheated ? 1 : 0}|modded=${lfw.survival_rank_modded ? 1 : 0}|invalid=${lfw.survival_rank_invalid ? 1 : 0}`,
      );
    } else if (op === "randinfo") {
      const e = make_entity();
      lfw.random_entity_info(e as never);
      push(
        `randinfo|${(e as Rec)["id"]}|${fmt_num((e as Rec)["facing"])}|${fmt_num((e as Rec)["position"].x)}|${fmt_num((e as Rec)["position"].y)}|${fmt_num((e as Rec)["position"].z)}|L${fmt_num(lfw.world.left)},${fmt_num(lfw.world.right)},${fmt_num(lfw.world.near)},${fmt_num(lfw.world.far)}`,
      );
    } else if (op === "mtrange") {
      push(
        `mtrange|${fmt_num((lfw as Rec)["_mt"].range(Number(nextKey()), Number(nextKey())))}`,
      );
    } else if (op === "entadd") {
      const id = nextKey();
      const n = Number(nextKey());
      const ret = lfw.entities.add({ type: 8, id }, n);
      push(`entadd|${ret.length}`);
    } else if (op === "getter") {
      const w = nextKey();
      push(`getter|${w}|${get_val_getter_from_stage(w) ? 1 : 0}`);
    } else if (op === "endtest") {
      const words: string[] = [];
      while (i[0]! < t.length) words.push(keyOf(t[i[0]!++]!));
      const items = words.map((w) => new Expression(w, get_val_getter_from_stage));
      push(`endtest|${words.join(",")}|${items.length}`);
    } else if (op === "keys2") {
      const a = (lfw as unknown as Rec)["keys"];
      const b = (lfw as unknown as Rec)["keys"];
      push(`keys2|${a === b ? 1 : 0}`);
    } else if (op === "keysgo") {
      const k = (lfw as unknown as Rec)["create_keys"]();
      const before = lfw.mounted_keys.length;
      (lfw as unknown as Rec)["regist_keys"](k);
      (lfw as unknown as Rec)["recycle_keys"](k);
      (lfw as unknown as Rec)["regist_keys"](k);
      push(`keysgo|${before}|${lfw.mounted_keys.length}`);
    } else if (op === "instcount") {
      push(`instcount|${LFW.instances.length}`);
    } else if (op === "dispose") {
      lfw.dispose();
      push(`dispose|${LFW.instances.length}`);
    } else if (op === "lznew") {
      const z = new Lz();
      z.zid = nextKey();
      z.name = nextKey();
      if (i[0] < t.length) z.md5 = nextKey();
      lzips.set(z.zid, z);
    } else if (op === "lzadd") {
      const zid = nextKey();
      const path = nextKey();
      const kind = t[i[0]!++]!;
      lzips.get(zid)!.entries.push({ path, kind });
    } else if (op === "imp") {
      const key = next();
      const kind = next();
      imports.set(key, () => kind_value(kind));
    } else if (op === "impfail") {
      const key = next();
      import_fails.set(key, i[0] < t.length ? next() : "boom");
    } else if (op === "impsoft") {
      import_soft_fails.add(next());
    } else if (op === "uimg") {
      const vid = next();
      const img = parseValue(t, i);
      try {
        const info = await ui_load_img(lfw, img as never);
        push(`uimg|${vid}|ok|${renderValue(safe_render(info))}`);
      } catch (e) {
        push(`uimg|${vid}|err|${esc(err_msg(e))}`);
      }
    } else if (op === "imgset") {
      const key = next();
      image_vals.set(key, parseValue(t, i));
    } else if (op === "newp") {
      const id = next();
      const x = Number(next());
      const y = Number(next());
      const z = Number(next());
      const btn = Number(next());
      pevs.set(id, new LFWPointerEvent({ x, y, z }, btn));
    } else if (op === "newk") {
      const id = next();
      const player = nextKey();
      const pressed = next() === "1";
      const gk = nextKey();
      const key = nextKey();
      kevs.set(id, new LFWKeyEvent(player, pressed, gk as never, key));
    } else if (op === "stp" || op === "sti") {
      const id = next();
      const e = pevs.get(id) ?? kevs.get(id);
      if (!e) fail(`unknown event '${id}'`);
      if (op === "stp") e.stop_propagation();
      else e.stop_immediate_propagation();
    } else if (op === "rdp") {
      const id = next();
      const e = pevs.get(id)!;
      push(`rdp|${id}|${num(e.point.x)}|${num(e.point.y)}|${num(e.point.z)}|${num(e.button)}|${e.stopped}`);
    } else if (op === "rdk") {
      const id = next();
      const e = kevs.get(id)!;
      push(`rdk|${id}|${esc(e.player)}|${esc(e.game_key)}|${esc(e.key)}|${e.pressed ? "1" : "0"}|${e.stopped}`);
    } else if (op === "imnode") {
      const lid = next();
      const nid = next();
      let node: Rec | null = null;
      if (nid !== "-") {
        node = img_nodes.get(nid) ?? null;
        if (!node) {
          node = make_img_node(lfw);
          img_nodes.set(nid, node);
        }
      }
      const captured = node;
      loaders.set(lid, new UIImgLoader(() => captured as never));
    } else if (op === "imjid") {
      const j = (loaders.get(next()) as unknown as Rec)["_jid"] as Rec;
      push(`imjid|${num(j["value"] as number)}|${num(j["min"] as number)}|${num(j["max"] as number)}`);
    } else if (op === "imignore") {
      loaders.get(next())!.ignore_out_of_date();
    } else if (op === "imload" || op === "imset") {
      const lid = next();
      const loader = loaders.get(lid)!;
      try {
        const imgs =
          op === "imload"
            ? await loader.load(parseValue(t, i) as never)
            : await loader.set_img(nextKey());
        push(`imload|ok|${renderValue(imgs)}`);
      } catch (e) {
        const rec = e as Rec;
        push(
          `imload|err|${esc(err_msg(e))}` +
            (rec && rec["__is_out_of_date_error"] ? "|ood|" + renderValue(rec["texture"]) : ""),
        );
      }
    } else if (op === "nod" || op === "noc") {
      const nid = next();
      const parent = op === "noc" ? tnodes.get(next()) : undefined;
      const data = parseValue(t, i);
      tnodes.set(nid, new UINode(lfw, data as never, parent as never));
    } else if (op === "nadd") {
      const parent = tnodes.get(next())!;
      const child = tnodes.get(next())!;
      parent.add_child(child);
    } else if (op === "nfn") {
      const n = tnodes.get(next())!;
      const tok = next();
      n.focused_node = tok === "-" ? undefined : tnodes.get(tok)!;
    } else if (op === "nset") {
      const nid = next();
      const what = next();
      const n = tnodes.get(nid)!;
      const nnum = (): number => Number(next());
      if (what === "x") n.set_x(nnum());
      else if (what === "y") n.set_y(nnum());
      else if (what === "z") n.set_z(nnum());
      else if (what === "w") n.set_w(nnum());
      else if (what === "h") n.set_h(nnum());
      else if (what === "cx") n.set_cx(nnum());
      else if (what === "cy") n.set_cy(nnum());
      else if (what === "cz") n.set_cz(nnum());
      else if (what === "sx") n.set_sx(nnum());
      else if (what === "sy") n.set_sy(nnum());
      else if (what === "sz") n.set_sz(nnum());
      else if (what === "visible") n.set_visible(next() === "1");
      else if (what === "disabled") n.set_disabled(next() === "1");
      else if (what === "opacity") n.set_opacity(nnum());
      else if (what === "clip") n.clip_children = next() === "1";
      else if (what === "focused") n.focused = next() === "1";
      else if (what === "background" || what === "foreground") {
        const tok = next();
        const v = tok === "-" ? null : keyOf(tok);
        if (what === "background") n.background = v;
        else n.foreground = v;
      } else if (what === "backgroundAlpha" || what === "foregroundAlpha") {
        const tok = next();
        const v = tok === "-" ? null : Number(tok);
        if (what === "backgroundAlpha") n.backgroundAlpha = v;
        else n.foregroundAlpha = v;
      } else if (what === "outlineColor" || what === "outlineWidth" || what === "outlineAlpha") {
        const v = parseValue(t, i);
        if (what === "outlineColor") n.outlineColor = v as never;
        else if (what === "outlineWidth") n.outlineWidth = v as never;
        else n.outlineAlpha = v as never;
      } else if (what === "global_pos") {
        n.global_pos = { x: nnum(), y: nnum(), z: nnum() };
      } else if (what === "resize3") {
        n.resize(nnum(), nnum(), nnum());
      } else if (what === "move3") {
        n.move_to(nnum(), nnum(), nnum());
      } else if (what === "center3") {
        n.set_center(nnum(), nnum(), nnum());
      } else if (what === "scale3") {
        n.set_scale(nnum(), nnum(), nnum());
      } else if (what === "update") {
        n.update(nnum());
      } else if (what === "text") {
        n.set_text(keyOf(next()));
      } else if (what === "text2") {
        const s = keyOf(next());
        n.set_text(s, parseValue(t, i) as never);
      } else if (what === "texti") {
        n.text = parseValue(t, i) as never;
      } else if (what === "image") {
        n.image = parseValue(t, i) as never;
      } else if (what === "style_assign") {
        n.style.assign(parseValue(t, i) as never);
      } else if (what === "style_touch") {
        n.style.touch();
      } else fail(`unknown nset '${what}'`);
    } else if (op === "nrd") {
      const nid = next();
      const what = next();
      const n = tnodes.get(nid)!;
      if (what === "pos" || what === "scale" || what === "size" || what === "center") {
        const v = what === "pos" ? n.pos : what === "scale" ? n.scale : what === "size" ? n.size : n.center;
        push(`nrd|${nid}|${what}|${num(v.x)}|${num(v.y)}|${num(v.z)}`);
      } else if (what === "clip") {
        push(`nrd|${nid}|clip|${n.clip_children ? "1" : "0"}`);
      } else if (what === "cross") {
        const c = n.cross;
        push(`nrd|${nid}|cross|${num(c.left)}|${num(c.top)}|${num(c.right)}|${num(c.bottom)}|${num(c.mid_x)}|${num(c.mid_y)}`);
      } else if (what === "rect") {
        const r = n.rect;
        push(`nrd|${nid}|rect|${num(r.left)}|${num(r.top)}|${num(r.right)}|${num(r.bottom)}`);
      } else if (what === "geo") {
        const g = n.geo;
        push(`nrd|${nid}|geo|${num(g.pos.x)}|${num(g.pos.y)}|${num(g.left)}|${num(g.top)}|${num(g.right)}|${num(g.bottom)}`);
      } else if (what === "gp") {
        const g = n.global_pos;
        push(`nrd|${nid}|gp|${num(g.x)}|${num(g.y)}|${num(g.z)}`);
      } else if (what === "flags") {
        push(`nrd|${nid}|flags|${n.visible ? 1 : 0}${n.self_visible ? 1 : 0}${n.disabled ? 1 : 0}${n.self_disabled ? 1 : 0}`);
      } else if (what === "op") {
        push(`nrd|${nid}|op|${num(n.opacity)}|${num(n.global_opacity)}`);
      } else if (what === "bg") {
        push(`nrd|${nid}|bg|${esc(n.background)}|${num(n.backgroundAlpha)}`);
      } else if (what === "fg") {
        push(`nrd|${nid}|fg|${esc(n.foreground)}|${num(n.foregroundAlpha)}`);
      } else if (what === "outline") {
        push(`nrd|${nid}|outline|${renderValue(n.outlineColor)}|${renderValue(n.outlineWidth)}|${renderValue(n.outlineAlpha)}`);
      } else if (what === "id") {
        push(`nrd|${nid}|id|${renderValue(n.id)}|${renderValue(n.name)}|${num(n.depth)}`);
      } else if (what === "depth") {
        push(`nrd|${nid}|depth|${num(n.depth)}`);
      } else if (what === "lifetime") {
        push(`nrd|${nid}|lifetime|${num(n.lifetime)}`);
      } else if (what === "ptr") {
        push(`nrd|${nid}|ptr|${n.pointer_over}|${n.pointer_down}|${n.click_flag}`);
      } else if (what === "foc") {
        push(`nrd|${nid}|foc|${n.focused ? 1 : 0}|${nref(n.focused_node)}`);
      } else if (what === "state") {
        push(`nrd|${nid}|state|${renderValue(n.state)}`);
      } else if (what === "value") {
        push(`nrd|${nid}|value|${renderValue(n.get_value(keyOf(next())))}`);
      } else if (what === "fc" || what === "sn" || what === "ln" || what === "fp") {
        const id = keyOf(next());
        const hit =
          what === "fc" ? n.find_child(id) : what === "sn" ? n.search_node(id) : what === "ln" ? n.lookup_node(id) : n.find_parent((p) => p.id === id);
        push(`nrd|${nid}|${what}|${nref(hit)}`);
      } else if (what === "fcn") {
        push(`nrd|${nid}|fcn|${nref(n.find_child_by_name(keyOf(next())))}`);
      } else if (what === "hit") {
        const x = Number(next());
        const y = Number(next());
        push(`nrd|${nid}|hit|${n.hit(x, y) ? 1 : 0}`);
      } else if (what === "text") {
        const tv = n.text as Rec | null | undefined;
        if (tv === null || tv === undefined) push(`nrd|${nid}|text|null`);
        else {
          const st = (tv as Rec).style;
          const ver = st instanceof Style;
          const sv = ver ? "v" + String((st as unknown as Rec).version) : "-";
          const sj = ver ? "-" : esc(JSON.stringify(st ?? {}));
          push(`nrd|${nid}|text|${renderValue((tv as Rec).text)}|${renderValue((tv as Rec).w)}|${renderValue((tv as Rec).h)}|${renderValue((tv as Rec).scale)}|${sv}|${sj}`);
        }
      } else if (what === "style") {
        push(`nrd|${nid}|style|${renderValue(n.style.data)}`);
      } else if (what === "image") {
        push(`nrd|${nid}|image|${renderValue(n.image)}`);
      } else if (what === "color") {
        push(`nrd|${nid}|color|${renderValue(n.color)}`);
      } else if (what === "opacity") {
        push(`nrd|${nid}|opacity|${num(n.opacity)}`);
      } else if (what === "kids") {
        let line = `nrd|${nid}|kids|${n.children.length}`;
        for (const c of n.children) line += `|${nref(c as UINode)}`;
        push(line);
      } else fail(`unknown nrd '${what}'`);
    } else if (op === "npd" || op === "npm" || op === "npu" || op === "npc") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const e = new LFWPointerEvent({ x: 0, y: 0, z: 0 }, 0);
      if (op === "npd") n.on_pointer_down(e);
      else if (op === "npm") n.on_pointer_move(e);
      else if (op === "npu") n.on_pointer_up(e);
      else n.on_pointer_cancel(e);
      push(`np|${nid}|${op}|stop=${e.stopped}`);
    } else if (op === "npl" || op === "npe") {
      const nid = next();
      const n = tnodes.get(nid)!;
      if (op === "npl") n.on_pointer_leave();
      else n.on_pointer_enter();
      push(`np|${nid}|${op}`);
    } else if (op === "ncb") {
      const nid = next();
      const ev = next();
      const n = tnodes.get(nid)!;
      if (ev === "show") n.callbacks.on("on_show", (node) => push(`cb|${nid}|show|${nref(node as never as UINode)}`));
      else if (ev === "hide") n.callbacks.on("on_hide", (node) => push(`cb|${nid}|hide|${nref(node as never as UINode)}`));
      else if (ev === "foucs_changed") n.callbacks.on("on_foucs_changed", (node) => push(`cb|${nid}|foucs_changed|${nref(node as never as UINode)}`));
      else if (ev === "foucs_item_changed") n.callbacks.on("on_foucs_item_changed", (f, b) => push(`cb|${nid}|foucs_item_changed|${nref(f as never)}|${nref(b as never)}`));
      else if (ev === "pdown" || ev === "pmove" || ev === "pup" || ev === "pcancel") {
        const key = ev === "pdown" ? "on_pointer_down" : ev === "pmove" ? "on_pointer_move" : ev === "pup" ? "on_pointer_up" : "on_pointer_cancel";
        n.callbacks.on(key, (e) => push(`cb|${nid}|${ev}|${num((e as LFWPointerEvent).button)}`));
      } else if (ev === "pleave") n.callbacks.on("on_pointer_leave", (node) => push(`cb|${nid}|pleave|${nref(node as never as UINode)}`));
      else if (ev === "penter") n.callbacks.on("on_pointer_enter", (node) => push(`cb|${nid}|penter|${nref(node as never as UINode)}`));
      else if (ev === "comp_add") n.callbacks.on("on_component_add", (c, node) => push(`cb|${nid}|comp_add|${(c as Rec).f_name}#${(c as Rec).id}|${nref(node as never as UINode)}`));
      else if (ev === "comp_del") n.callbacks.on("on_component_del", (c, node) => push(`cb|${nid}|comp_del|${(c as Rec).f_name}#${(c as Rec).id}|${nref(node as never as UINode)}`));
      else fail(`unknown ncb '${ev}'`);
    } else if (op === "nmk") {
      const nid = next();
      const ptok = next();
      const parent = ptok === "-" ? undefined : tnodes.get(ptok)!;
      tnodes.set(nid, UINode.create(lfw, parseValue(t, i) as never, parent, undefined));
    } else if (op === "nlife") {
      const n = tnodes.get(next())!;
      const what = next();
      if (what === "start") n.on_start();
      else if (what === "stop") n.on_stop();
      else if (what === "resume") n.on_resume();
      else if (what === "pause") n.on_pause();
      else if (what === "show") n.on_show();
      else if (what === "hide") n.on_hide();
      else if (what === "blur") n.on_blur();
      else if (what === "foucs") n.on_foucs();
      else fail(`unknown nlife '${what}'`);
      await flush_microtasks();
    } else if (op === "nclick") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const e = new LFWPointerEvent({ x: 0, y: 0, z: 0 }, Number(next()));
      n.on_click(e);
      push(`click|${nid}|stop=${e.stopped}`);
    } else if (op === "nkey") {
      const nid = next();
      const dir = next();
      const gk = nextKey();
      const key = nextKey();
      const pre = t[i[0]] === "1";
      if (pre) ++i[0];
      const n = tnodes.get(nid)!;
      const e = new LFWKeyEvent("", dir === "down", gk as never, key);
      if (pre) e.stop_immediate_propagation();
      if (dir === "down") n.on_key_down(e);
      else n.on_key_up(e);
      push(`nkey|${nid}|${dir}|stop=${e.stopped}`);
    } else if (op === "nml") {
      const nid = next();
      const lt = next();
      const layer = lt !== "-" ? real_layers.v?.at(Number(lt)) : undefined;
      tnodes.set(nid, UINode.create(lfw, parseValue(t, i) as never, undefined, layer));
    } else if (op === "cadd" || op === "cdel" || op === "lcomp" || op === "cupd" || op === "cfind" || op === "cset") {
      const nid = next();
      const n = tnodes.get(nid)!;
      if (op === "cadd") {
        const cid = next();
        const props = parseValue(t, i) as Rec;
        const info = { id: cid, name: cid, cls: "FakeComp", args: [], enabled: true, properties: props, props: {}, weight: 0 };
        const c = new FakeComponent(n, cid, info as never);
        n.add_components(c);
        push(`cadd|${nid}|${cid}|${n.components.length}`);
      } else if (op === "cdel") {
        const cid = next();
        const c = n.components.find((v: Rec) => v.id === cid) as FakeComponent | undefined;
        if (c) n.del_components(c);
        push(`cdel|${nid}|${cid}|${n.components.length}`);
      } else if (op === "lcomp") {
        push(`lcomp|${nid}|${n.components.length}`);
        let k = 0;
        for (const c of n.components as FakeComponent[]) {
          push(`fc|list|${k}|${(c.constructor as Rec).TAG}|${c.f_name}|${c.id}|${c.name}|${c.enabled ? "b1" : "b0"}`);
          k += 1;
        }
      } else if (op === "cupd") {
        n.update(Number(next()));
        push(`cupd|${nid}`);
      } else if (op === "cset") {
        const cid = next();
        const on = next() === "1";
        const c = n.components.find((v: Rec) => v.id === cid) as FakeComponent | undefined;
        c?.set_enabled(on);
        push(`cset|${nid}|${cid}|${c?.enabled ? "b1" : "b0"}`);
      } else {
        const cid = next();
        const which = keyOf(next());
        const c = n.components.find((v: Rec) => v.id === cid) as FakeComponent | undefined;
        push(`cfind|${nid}|${cid}|${which}|${c ? nref(c.find_node(which) as UINode) : "u"}`);
      }
    } else if (op === "wpause") {
      const v = Number(next());
      lfw.world.set_paused(v as never);
      push(`wpause|${num(v)}|${lfw.world.paused ? 1 : 0}`);
    } else if (op === "gfa" || op === "gfu" || op === "gfuse" || op === "gfr") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const cid = next();
      const c = (n.components as Rec[]).find((v) => v.id === cid) as Rec | undefined;
      if (op === "gfa") {
        c!.fsm = new FSM<ModeState, never>().add(
          new GameModeFSMState_Running(c as never),
          new GameModeFSMState_BeforeEnd(c as never),
          new GameModeFSMState_End(c as never),
        ) as never;
        push(`gfa|${nid}|${cid}`);
      } else if (op === "gfu") {
        const dt = Number(next());
        (c!.fsm as Rec).update(dt);
        push(`gfu|${nid}|${cid}|${num(dt)}`);
      } else if (op === "gfuse") {
        const key = nextKey();
        (c!.fsm as Rec).use(key as never);
        push(`gfuse|${nid}|${cid}|${key}`);
      } else {
        const f = c!.fsm as Rec;
        const s = f.state as Rec | undefined;
        const p = f.prev_state as Rec | undefined;
        push(
          `gfr|${nid}|${cid}|state=${renderValue(s?.key)}|prev=${renderValue(p?.key)}|st=${num(f.state_time as number)}|t=${num(f.time as number)}`,
        );
      }
    } else if (op === "cplay") {
      const nid = next();
      const cls = next();
      const act = next();
      const rev = next() === "1";
      const n = tnodes.get(nid)!;
      const comp = (n.components as Rec[]).find(
        (c) => (((c.constructor as Rec).TAGS as string[] | undefined) ?? []).includes(cls),
      );
      if (comp) {
        if (cls === "OpacityAnimation") {
          if (act === "start") (comp as Rec).start(rev);
          else if (act === "stop") (comp as Rec).stop(rev);
        } else if (cls === "OpacityFlash") {
          if (act === "start") (comp as Rec).start();
          else if (act === "stop") (comp as Rec).stop();
          else if (act === "replay") (comp as Rec).replay();
        } else if (cls === "FadeOutOpacity" || cls === "ScaleAnimation" || cls === "PositionAnimation") {
          if (act === "start") (comp as Rec).start(rev);
        }
      }
      push(`cplay|${nid}|${cls}|${act}|${comp ? "ok" : "none"}|${comp && (comp as Rec).enabled ? "b1" : "b0"}`);
    } else if (op === "calign") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = (n.components as Rec[]).find(
        (v) => (((v.constructor as Rec).TAGS as string[] | undefined) ?? []).includes("FlexItem"),
      );
      if (!c) {
        push(`calign|${nid}|none`);
      } else {
        let out = "err";
        let errs = 0;
        try {
          out = String((c as Rec).align);
          errs = (((c as Rec).props_holder as Rec).errors as unknown[]).length;
        } catch (e) {
          errs = (((e as Rec).errors as unknown[] | undefined) ?? []).length;
        }
        push(`calign|${nid}|${out}|${num(errs)}`);
      }
    } else if (op === "pic") {
      const nid = next();
      const act = next();
      const n = tnodes.get(nid)!;
      const comps = n.components as Rec[];
      const pic = comps.find((v) => (((v.constructor as Rec).TAGS as string[] | undefined) ?? []).includes("Picture"));
      const loop = comps.find((v) => (((v.constructor as Rec).TAGS as string[] | undefined) ?? []).includes("ImgLoop"));
      if (act === "src" && pic) {
        (pic as Rec).set_src(next());
        await flush_microtasks();
        push(`pic|${nid}|src|${String((pic as Rec).src)}`);
      } else if (act === "rd" && pic) {
        push(`pic|${nid}|rd|${num((pic as Rec).width)}|${num((pic as Rec).height)}|${String((pic as Rec).src)}`);
      } else if (act === "setw" && pic) {
        (pic as Rec).width = Number(next());
        push(`pic|${nid}|setw|${num((pic as Rec).width)}`);
      } else if (act === "seth" && pic) {
        (pic as Rec).height = Number(next());
        push(`pic|${nid}|seth|${num((pic as Rec).height)}`);
      } else if (act === "setimg" && (pic || loop)) {
        (n as Rec).image = new ImageInfo(parseValue(t, i) as Rec);
        push(`pic|${nid}|setimg|1`);
      } else if (act === "istart" && loop) {
        (loop as Rec).start();
        push(`pic|${nid}|istart|1`);
      } else if (act === "istop" && loop) {
        (loop as Rec).stop();
        push(`pic|${nid}|istop|1`);
      } else {
        push(`pic|${nid}|${act}|none`);
      }
    } else if (op === "snew") {
      const sid = next();
      const s = new SmoothNumber();
      s.handler((self) => push(`scb|${sid}|${num(self.value)}|${self.done ? 1 : 0}`));
      smooths.set(sid, s);
    } else if (op === "srd") {
      const sid = next();
      const s = smooths.get(sid)!;
      push(`srd|${sid}|${num(s.value)}|${num(s.target)}|${s.done ? 1 : 0}`);
    } else if (op === "svalue" || op === "starget" || op === "sspeed" || op === "sfactor" || op === "smdiff") {
      const sid = next();
      const v = Number(next());
      const s = smooths.get(sid)!;
      if (op === "svalue") s.value = v;
      else if (op === "starget") s.target = v;
      else if (op === "sspeed") s.speed(v);
      else if (op === "sfactor") s.factor(v);
      else s.min_diff(v);
    } else if (op === "smode") {
      const sid = next();
      smooths.get(sid)!.mode(next() as never);
    } else if (op === "supd") {
      smooths.get(next())!.update();
    } else if (op === "shandle") {
      smooths.get(next())!.handle();
    } else if (op === "reach") {
      const nid = next();
      const act = next();
      const n = tnodes.get(nid)!;
      const comps = n.components as Rec[];
      const tags = (v: Rec): string[] => (((v.constructor as Rec).TAGS as string[] | undefined) ?? []);
      if (act === "rd") {
        const r = comps.find((v) => tags(v).includes("Reachable"));
        if (!r) push(`reach|${nid}|none`);
        else push(`reach|${nid}|g|${String((r as Rec).group_name)}|${(r as Rec).group ? nref((r as Rec).group.node as UINode) : "u"}`);
      } else {
        const rg = comps.find((v) => tags(v).includes("ReachableGroup"));
        if (!rg) push(`reach|${nid}|none`);
        else {
          const parts = [
            `reach|${nid}|rg|${String((rg as Rec).group)}|${String((rg as Rec).direction)}|${nref((rg as Rec).binded_layout as UINode)}|${((rg as Rec).reachables as Rec[]).length}`,
            ...((rg as Rec).reachables as Rec[]).map((v) => nref(v.node as UINode)),
          ];
          push(parts.join("|"));
        }
      }
    } else if (op === "wfn") {
      lfw.world.fn_locked = Number(next()) !== 0;
    } else if (op === "stime") {
      lfw.world.stage.time = Number(next());
    } else if (op === "wcb") {
      lfw.world.callbacks.call(keyOf(next()));
    } else if (op === "lcb") {
      lfw.callbacks.call(keyOf(next()), lfw);
    } else if (op === "wstage") {
      lfw.world.stage = new Stage(lfw.world, parseValue(t, i) as never);
    } else if (op === "sphase") {
      lfw.world.stage.enter_phase(Number(next()));
    } else if (op === "sfield") {
      const key = keyOf(next());
      (lfw.world.stage.data as Rec)[key] = parseValue(t, i);
    } else if (op === "dialpush") {
      lfw.world.stage.push_dialogs(parseValue(t, i) as never);
    } else if (op === "dialnext") {
      lfw.world.stage.next_dialog();
    } else if (op === "dialclear") {
      lfw.world.stage.clear_dialogs();
    } else if (op === "stevt") {
      lfw.world.stage.callbacks.call(keyOf(next()), lfw.world.stage);
    } else if (op === "pkeye") {
      const pid = nextKey();
      const name = nextKey();
      const key = nextKey();
      const p = lfw.player(pid);
      let ok = 0;
      try {
        p.set_key(name as never, key, true);
        ok = 1;
      } catch {
        ok = 0;
      }
      push(`pkeye|${p.id}|${ok}`);
    } else if (op === "ctext") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const act = next();
      const ti = n.components.find((c) => c instanceof TextInput) as TextInput | undefined;
      if (!ti) push(`ctext|${nid}|none`);
      else {
        if (act === "set") ti.text = keyOf(next());
        push(`ctext|${nid}|${render_value(ti.text)}|${render_value(ti.maxLength)}|${render_value(ti.defaultValue)}`);
      }
    } else if (op === "pname") {
      const pid = nextKey();
      const name = nextKey();
      lfw.player(pid).set_name(name, true);
    } else if (op === "ccall") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const cls_name = next();
      const method = next();
      const cls = cls_name === "PlayerName" ? PlayerName : cls_name === "PlayerTeamName" ? PlayerTeamName : FighterName;
      const c = n.components.find((v) => v instanceof cls) as PlayerName | undefined;
      if (!c) push(`ccall|${nid}|${cls_name}|none`);
      else if (method === "quit") c.quit();
      else {
        const text = keyOf(next());
        const com = next() === "1";
        const decided = next() === "1";
        c.join(text, com, decided);
      }
    } else if (op === "ird") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const p = n.components.find((c) => c instanceof IntegerPicker) as IntegerPicker | undefined;
      if (!p) push(`ird|${nid}|none`);
      else push(`ird|${nid}|${num(p.min)}|${num(p.max)}|${num(p.val)}`);
    } else if (op === "ibus") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const p = n.components.find((c) => c instanceof IntegerPicker) as IntegerPicker | undefined;
      if (!p) push(`ibus|${nid}|none`);
      else {
        p.callbacks.add({
          on_min_changed: (curr: number, prev: number, self: IntegerPicker) =>
            push(`ibus|min|${num(curr)}|${num(prev)}|${self === p ? 1 : 0}`),
          on_max_changed: (curr: number, prev: number, self: IntegerPicker) =>
            push(`ibus|max|${num(curr)}|${num(prev)}|${self === p ? 1 : 0}`),
          on_val_changed: (curr: number, prev: number, self: IntegerPicker) =>
            push(`ibus|val|${num(curr)}|${num(prev)}|${self === p ? 1 : 0}`),
        });
      }
    } else if (op === "iput") {
      const nid = next();
      const what = next();
      const v = Number(next());
      const n = tnodes.get(nid)!;
      const p = n.components.find((c) => c instanceof IntegerPicker) as IntegerPicker | undefined;
      if (!p) push(`iput|${nid}|none`);
      else if (what === "min") p.min = v;
      else if (what === "max") p.max = v;
      else p.val = v;
    } else if (op === "algn") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const p = n.components.find((c) => c instanceof Alignment) as Alignment | undefined;
      if (!p) push(`algn|${nid}|none`);
      else {
        const ref = (v: Rec | null | undefined) => (v ? renderValue(v.id) : "u");
        const al = (v: string | null | undefined) => renderValue(v ?? undefined);
        const once = (p as unknown as Rec).once as boolean;
        push(
          `algn|${nid}|f=${ref(p.follower)}|l=${ref(p.followed.l)}|r=${ref(p.followed.r)}|t=${ref(p.followed.t)}|b=${ref(p.followed.b)}|al=${al(p.align.l)}|ar=${al(p.align.r)}|at=${al(p.align.t)}|ab=${al(p.align.b)}|ol=${num(p.offset.l)}|or=${num(p.offset.r)}|ot=${num(p.offset.t)}|ob=${num(p.offset.b)}|once=${once ? 1 : 0}|has=${n.components.includes(p) ? 1 : 0}`,
        );
      }
    } else if (op === "apl") {
      const nid = next();
      const what = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof Players) as Players | undefined;
      if (!c) push(`apl|${nid}|none`);
      else if (what === "read") push(`apl|${nid}|read|${num(c.players.size)}`);
      else {
        const pid = nextKey();
        const p = lfw.player(pid);
        if (!p) push(`apl|${nid}|${pid}|nop`);
        else if (what === "join")
          push(`apl|${nid}|${pid}|${c.player_join(p) ? 1 : 0}|${num(c.players.size)}`);
        else {
          c.player_leave(p);
          push(`apl|${nid}|${pid}|left|${num(c.players.size)}`);
        }
      }
    } else if (op === "scall") {
      const nid = next();
      const what = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof Sounds) as Sounds | undefined;
      if (!c) push(`scall|${nid}|none`);
      else {
        if (what === "start") c.start();
        else if (what === "stop") c.stop();
        else if (what !== "read") c.replay();
        const rec = c as unknown as Rec;
        push(
          `scall|${nid}|${what}|en=${c.enabled ? 1 : 0}|idx=${num(rec.idx)}|tm=${num(rec.time)}|lc=${num(c.loop.count)}|lt=${num(c.loop.times)}`,
        );
      }
    } else if (op === "tsn") {
      const map = (lfw.callbacks as unknown as Rec)._map as Map<string, Rec> | undefined;
      const pack = map?.get("on_enter_next_stage");
      const n = pack ? (pack._set as Set<unknown>).size : 0;
      push(`tsn|n=${n}`);
    } else if (op === "lbk") {
      const nid = next();
      const what = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof LabelButton) as LabelButton | undefined;
      if (!c) push(`lbk|${nid}|none`);
      else {
        if (what === "set") c.text = keyOf(next());
        else if (what === "checked") {
          const v = next();
          c.checked = v === "null" ? null : v === "1";
        }
        push(
          `lbk|${nid}|checked=${renderValue(c.checked)}|resp=${nref(c.responser)}|nc=${esc(c.normal_color)}|hc=${esc(c.hover_color)}|text=${esc(c.text)}`,
        );
      }
    } else if (op === "pct" || op === "pset") {
      const pid = nextKey();
      const p = lfw.player(pid);
      if (op === "pset") {
        const v = Number(next());
        const emit = next() === "1";
        p.set_ctrl(v as never, emit);
      }
      push(`pct|${pid}|ctrl=${renderValue(p.ctrl)}`);
    } else if (op === "wr") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof WeaponRain) as WeaponRain | undefined;
      if (!c) push(`wr|${nid}|none`);
      else {
        const rec = c as unknown as Rec;
        const timer = rec.timer as Rec;
        push(
          `wr|${nid}|tv=${num(timer.value)}|tmin=${num(timer.min)}|tmax=${num(timer.max)}|ls=${renderValue(rec.last_section)}|n=${num((rec._oid_datas as unknown[]).length)}`,
        );
      }
    } else if (op === "fh") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof FighterHead) as FighterHead | undefined;
      if (!c) push(`fh|${nid}|none`);
      else {
        (c as unknown as Rec).handle_changed();
        await flush_microtasks();
        push(
          `fh|${nid}|pid=${esc(c.player_id)}|head=${esc(c.head)}|src=${esc(c.src)}|w=${num(c.width)}|h=${num(c.height)}`,
        );
      }
    } else if (op === "ft") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof FighterThumb) as FighterThumb | undefined;
      if (!c) push(`ft|${nid}|none`);
      else {
        (c as unknown as Rec).handle_changed();
        await flush_microtasks();
        push(
          `ft|${nid}|url=${esc(c.thumb_url)}|src=${esc(c.src)}|w=${num(c.width)}|h=${num(c.height)}`,
        );
      }
    } else if (op === "dcnt") {
      const st = lfw.datas.stages.map((v) => String((v as Rec)["id"])).join(",");
      const bg = lfw.datas.backgrounds.map((v) => String((v as Rec)["id"])).join(",");
      const fg = (lfw.datas as Rec & { fighters: Rec[] }).fighters
        .map((v) => String(v["id"]))
        .join(",");
      push(`dcnt|stages=${st}|bgs=${bg}|fighters=${fg}`);
    } else if (op === "cplayer") {
      const nid = next();
      const act = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof ComponentsPlayer) as
        | ComponentsPlayer
        | undefined;
      if (!c) push(`cplayer|${nid}|none`);
      else {
        if (act === "start") c.start();
        else if (act === "stop") c.stop();
        else if (act === "replay") c.replay();
        const list = (c as unknown as Rec)["components"] as (Rec | null)[];
        const ids = list
          .map((v) => (v ? `${String(v["id"])}@${nref((v as Rec)["node"] as UINode)}` : "u"))
          .join(",");
        push(
          `cplayer|${nid}|${act}|enabled=${c.enabled ? 1 : 0}|playable=${c["__is_playable__"] ? 1 : 0}|ids=${ids}`,
        );
      }
    } else if (op === "sswi") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof StageSwitcher) as StageSwitcher | undefined;
      if (!c) push(`sswi|${nid}|none`);
      else {
        const list = c.stages as Rec[];
        const st = (c as unknown as Rec)["_stage"] as Rec;
        push(
          `sswi|${nid}|n=${num(list.length)}|all=${c.show_all ? 1 : 0}|id=${String(st?.["id"])}|text=${esc(c.stage_text)}`,
        );
      }
    } else if (op === "bswi") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof BackgroundSwitcher) as
        | BackgroundSwitcher
        | undefined;
      if (!c) push(`bswi|${nid}|none`);
      else {
        const list = c.backgrounds as Rec[];
        const bg = (c as unknown as Rec)["_background"] as Rec;
        push(`bswi|${nid}|n=${num(list.length)}|id=${String(bg?.["id"])}`);
      }
    } else if (op === "dlg") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof Dialogs) as Dialogs | undefined;
      if (!c) push(`dlg|${nid}|none`);
      else {
        const tr = (c as unknown as Rec)["_transform"] as Rec;
        push(
          `dlg|${nid}|hiding=${(c as unknown as Rec)["_hiding"] ? 1 : 0}|` +
            `vis=${n.visible ? 1 : 0}|tx=${num(tr["scale_x"] as number)},` +
            `${num(tr["scale_y"] as number)},${num(tr["scale_z"] as number)}|` +
            `wt=${num((lfw.world as Rec)["transform"]["x"] as number)},` +
            `${num((lfw.world as Rec)["transform"]["y"] as number)}|` +
            `wtt=${num((lfw.world as Rec)["transform"]["d"]["x"] as number)},` +
            `${num((lfw.world as Rec)["transform"]["d"]["y"] as number)}`,
        );
      }
    } else if (op === "fx") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof Flex) as Flex | undefined;
      if (!c) push(`fx|${nid}|none`);
      else {
        const pad = c.padding;
        push(
          `fx|${nid}|${c.direction}|${num(c.gap)}|${num(c.row_gap)}|${num(c.col_gap)}|` +
            `${c.align}|${c.fit ? 1 : 0}|${c.fit_w ? 1 : 0}|${c.fit_h ? 1 : 0}|` +
            `${num(pad[0])}|${num(pad[1])}|${num(pad[2])}|${num(pad[3])}`,
        );
      }
    } else if (op === "jl") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof Jalousie) as Jalousie | undefined;
      if (!c) push(`jl|${nid}|none`);
      else {
        const anim = c.anim as Rec & { value: number; done: boolean; reverse: boolean };
        push(
          `jl|${nid}|${c.open ? 1 : 0}|${c.click_test ? 1 : 0}|${num(c.w)}|${num(c.h)}|` +
            `v=${num(anim.value)}|done=${anim.done ? 1 : 0}|rev=${anim.reverse ? 1 : 0}|` +
            `dur=${num(anim.duration as number)}|n=${num((anim.anims as Rec[]).length)}`,
        );
      }
    } else if (op === "jlopen") {
      const nid = next();
      const v = next() === "1";
      const n = tnodes.get(nid)!;
      const c = n.components.find((v2) => v2 instanceof Jalousie) as Jalousie | undefined;
      if (c) c.open = v;
    } else if (op === "jsub") {
      const nid = next();
      const n = tnodes.get(nid)!;
      const c = n.components.find((v) => v instanceof Jalousie) as Jalousie | undefined;
      if (c) {
        c.callbacks.add({
          on_anim_end: (v: Jalousie) => push(`jcb|on_anim_end|${v === c ? 1 : 0}`),
        });
      }
    } else if (op === "mtstate") {
      const mt = (lfw as Rec)["_mt"] as Rec;
      push(`mtstate|${num(mt["index"] as number)}|${num(mt["times"] as number)}`);
    } else if (op === "gwcnt") {
      const section = Number(next());
      const count = Number(next());
      ((lfw.world as Rec).ground_weapon_counts as Map<number, number>).set(section, count);
      push(`gwcnt|${num(section)}|${num(count)}`);
    } else if (op === "fhset" || op === "ftset") {
      const nid = nextKey();
      const url = nextKey();
      const n = tnodes.get(nid)!;
      let e: Rec | undefined;
      if (url !== "-") {
        const key = op === "fhset" ? "head" : "small";
        e = { id: "fx", data: { type: 8, id: "fx", base: { [key]: url === "z" ? undefined : url } } };
        fakes.push(e);
      }
      if (op === "fhset") {
        const c = n.components.find((v) => v instanceof FighterHead) as FighterHead | undefined;
        const pid = c?.player_id ?? "";
        lfw.player(pid).fighter = e as never;
        push(`fhset|${pid}|${e ? 1 : 0}`);
      } else {
        const c = n.components.find((v) => v instanceof FighterThumb) as FighterThumb | undefined;
        if (c) (c as unknown as Rec).fighter = e;
        push(`ftset|${nid}|${e ? 1 : 0}`);
      }
    } else if (op === "nact") {
      const n = tnodes.get(next())!;
      actor.act(n, parseValue(t, i) as never);
    } else if (op === "rlnew") {
      real_layers.v = new UILayers(lfw);
    } else if (op === "flset" || op === "flpush") {
      const id = keyOf(next());
      if (op === "flset") lfw.layers.set_page({ id }, 0);
      else lfw.layers.push_page({ id }, 0);
    } else if (op === "rlpush") {
      real_layers.v!.push();
    } else if (op === "rlset" || op === "rlpushp") {
      const idx = Number(next());
      const id = keyOf(next());
      if (op === "rlset") real_layers.v!.set_page({ id }, idx);
      else real_layers.v!.push_page({ id }, idx);
    } else if (op === "rlpop") {
      const idx = Number(next());
      const min_pages = Number(next());
      const inclusive = next() === "1";
      const until = next();
      const l = real_layers.v!.at(idx) as UILayer | undefined;
      const opts: IPopPageOpts = { min_pages, inclusive };
      if (until !== "-") opts.until = (u) => u.id === until;
      l?.pop(opts);
    } else if (op === "rldisp") {
      real_layers.v!.dispose();
    } else if (op === "rlinfo") {
      let line = `rlinfo|${num(real_layers.v!.length)}`;
      for (const l of real_layers.v!.all) {
        if (!l) {
          line += "|-";
          continue;
        }
        line += `|${num(l.index)}:${num(l.pages.length)}:${l.ui?.id ?? "u"}`;
      }
      push(line);
    } else if (op === "rlui") {
      const idx = Number(next());
      const u = real_layers.v!.at(idx)?.ui;
      push(`rlui|${num(idx)}|${nref(u)}|${u ? num(u.z) : "u"}`);
    } else if (op === "rlz") {
      const idx = Number(next());
      const u = real_layers.v!.at(idx)?.ui;
      push(`rlz|${num(idx)}|${u ? num(u.z) : "u"}`);
    } else if (op === "rlfind") {
      const idx = Number(next());
      const nid = keyOf(next());
      const u = real_layers.v!.at(idx)?.ui;
      const hit = u ? u.search_node(nid) : undefined;
      push(`rlfind|${num(idx)}|${nref(hit)}|${hit ? num(hit.depth) : "u"}`);
    } else if (op === "rlfocus") {
      const idx = Number(next());
      const nid = keyOf(next());
      const v = next() === "1";
      const hit = real_layers.v!.at(idx)?.ui?.search_node(nid);
      if (hit) hit.focused = v;
    } else if (op === "rlfn") {
      const idx = Number(next());
      const u = real_layers.v!.at(idx)?.ui;
      push(`rlfn|${num(idx)}|${nref(u ? u.root.focused_node : undefined)}`);
    } else if (op === "rltree") {
      const idx = Number(next());
      const u = real_layers.v!.at(idx)?.ui;
      push(`rltree|${num(idx)}|${u ? tree_str(u) : "u"}`);
    } else if (op === "unpatch") {
      // 4AQ：两侧都不再有 uis/layers 补丁（真实现）；占位兼容旧用例。
    } else if (op === "uipg") {
      const id = keyOf(next());
      const v = parseValue(t, i) as Rec;
      v["id"] = id;
      (lfw.uis as unknown as Rec)["add"](v);
    } else if (op === "rlact") {
      const idx = Number(next());
      const ntok = next();
      const action = parseValue(t, i) as never;
      const l = real_layers.v!.at(idx) as UILayer | undefined;
      let u: UINode | undefined = l?.ui;
      if (u && ntok !== "-") u = u.search_node(keyOf(ntok));
      if (u) actor.act(u, action);
    } else if (op === "uinew") {
      ui_vals.set(next(), parseValue(t, i) as Rec);
    } else if (op === "uinest") {
      const cid = next();
      const pid = next();
      (ui_vals.get(cid) as Rec)["parent"] = ui_vals.get(pid);
    } else if (op === "uifind") {
      const id = next();
      const name = nextKey();
      const out = await find_ui_template(lfw, ui_vals.get(id) as never, name);
      push(`uifind|${renderValue(out)}`);
    } else if (op === "uimerge") {
      const pid = next();
      const parent = pid === "-" ? undefined : (ui_vals.get(pid) as never);
      const raw = parseValue(t, i) as never;
      push(`uimerge|${renderValue(await merge_ui_template(lfw, raw, parent))}`);
    } else if (op === "ucook") {
      const pid = next();
      const parent = pid === "-" ? undefined : (ui_vals.get(pid) as never);
      const info = parseValue(t, i);
      try {
        const cooked = await cook_ui_info(lfw, info as never, parent);
        push(`ucook|${render_cycle_safe(cooked)}`);
      } catch (e) {
        const rec = e as Rec;
        const msg = rec && rec["error"] ? (rec["error"] as Error).message : err_msg(e);
        push(`ucook|err|${esc(msg)}`);
      }
    } else if (op === "devon") {
      lfw.dev = true;
    } else if (op === "devoff") {
      lfw.dev = false;
    } else if (op === "zips") {
      const zs = (lfw as unknown as Rec)["zips"].zips as { name: string }[];
      const infos = (lfw as unknown as Rec)["zips"].data_infos as Rec[];
      push(`zips|${zs.map((v) => v.name).join(",")}|${infos.map((v) => String(v.md5)).join(",")}`);
    } else if (op === "collect") {
      const infos = (await (LFW as unknown as Rec).collect_data_infos()) as Rec[];
      const body = infos
        .map((v) => `${String(v.type)}:${String(v.url)}:${String(v.title)}:${String(v.md5)}`)
        .join(";");
      push(`collect|${infos.length}|${body}`);
    } else if (op === "lstate") {
      const r = lfw as unknown as Rec;
      push(
        `lstate|loading=${r["_loading"] ? 1 : 0}|playable=${r["_playable"] ? 1 : 0}|ui=${r["_ui_loaded"] ? 1 : 0}|disposed=${r["_disposed"] ? 1 : 0}`,
      );
    } else if (op === "bgms") {
      push(`bgms|${((lfw as unknown as Rec)["bgms"] as string[]).join(",")}`);
    } else if (op === "setbgms") {
      const list = parseValue(t, i) as string[];
      const cur = (lfw as unknown as Rec)["bgms"] as string[];
      cur.length = 0;
      for (const b of list) cur.push(b);
      push(`setbgms|${cur.join(",")}`);
    } else if (op === "sndbgm") {
      push(`sndbgm|${snd_probe()}`);
    } else if (op === "loadobj") {
      const zid = nextKey();
      const z = lzips.get(zid);
      try {
        const loaded = await (lfw as unknown as Rec)["_load_zip_from_object"](z);
        push(`loadobj|ok|${dump_info(loaded.info)}`);
      } catch (e) {
        push(`loadobj|fail|${err_msg(e)}`);
      }
    } else if (op === "loaddata") {
      const zid = nextKey();
      const z = lzips.get(zid);
      try {
        const loaded = await (lfw as unknown as Rec)["_load_zip_from_object"](z);
        await (lfw as unknown as Rec)["load_data"](loaded);
        push("loaddata|ok");
      } catch (e) {
        push(`loaddata|fail|${err_msg(e)}`);
      }
    } else if (op === "load") {
      const items: unknown[] = [];
      while (i[0] < t.length) {
        const tok = t[i[0]!++]!;
        if (tok.startsWith("z:")) items.push(lzips.get(tok.slice(2)));
        else items.push(keyOf(tok.startsWith("s:") ? tok.slice(2) : tok));
      }
      try {
        await lfw.load(...(items as never[]));
        push("load|ok");
      } catch (e) {
        push(`load|fail|${err_msg(e)}`);
      }
    } else if (op === "stored") {
      const url = next();
      const md5 = next();
      zip_stored.set(`${url}|${md5}`, next());
    } else if (op === "blob" || op === "buf") {
      blob_zip.set(next(), next());
    } else if (op === "dl") {
      const url = next();
      const mode = next();
      const token = next();
      const md5 = next();
      const size = Number(next());
      dl_scripts.set(url, { mode, token, md5: md5 === "-" ? undefined : md5, size });
    } else if (op === "cacheblob" || op === "cachedata") {
      const key = next();
      const name = next();
      const token = next();
      cache_vals.set(key, op === "cacheblob" ? { name, blob: token } : { name, data: token });
    } else if (op === "cachelate") {
      const key = next();
      const name = next();
      const token = next();
      cache_late.set(key, { name, blob: token });
    } else if (op === "cachelog") {
      cache_log = true;
    } else if (op === "impinfo") {
      const key = next();
      const url = next();
      const md5 = next();
      imports.set(key, () => {
        const v: Rec = { type: "FULL", title: "T" };
        if (url !== "-") v["url"] = url;
        if (md5 !== "-") v["md5"] = md5;
        return v;
      });
    } else if (op === "url") {
      const info_url = nextKey();
      try {
        const loaded = await (lfw as unknown as Rec)["_load_zip_from_url"](info_url);
        push(`url|ok|${loaded.zip.name}|${dump_info(loaded.info)}`);
      } catch (e) {
        push(`url|fail|${err_msg(e)}`);
      }
    } else {
      fail(`unknown op '${op}'`);
    }
  }

  process.stdout.write(log.join("\n") + "\n");
}

let ent_seq = 0;
const fakes: Rec[] = [];
function make_entity(): Rec {
  const e: Rec = {
    id: `e${++ent_seq}`,
    facing: 1,
    team: undefined,
    ctrl: undefined,
    position: {
      x: 0,
      y: 0,
      z: 0,
      set(x: number, y: number, z: number) {
        this.x = x;
        this.y = y;
        this.z = z;
      },
    },
    attach() {},
  };
  return e;
}

function fmt_num(n: unknown): string {
  return num(Number(n));
}

function team_to_bool(t: string): boolean | undefined {
  if (t === "-") return undefined;
  return t === "1";
}

Factory.entity_creators.set(8 as never, ((_world: unknown, data: Rec) => {
  push("entadd:create|" + String(data?.["id"]));
  const e = make_entity();
  e["data"] = data;
  return e;
}) as never);

main();
