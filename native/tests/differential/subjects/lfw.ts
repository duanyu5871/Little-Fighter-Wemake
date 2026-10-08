// `LFW`（门面 4AB）的 TS 侧台面，op 与 `subjects/lfw.cpp` 一一对应。
//
// `Ditto` 的宿主包全脚本化（Sounds/ImageMgr/Keyboard/Pointings/UIInputHandle/WorldRender/
// Cache/Zip/Clock/Render/Timeout/…）；`Date.now` 固定成 12345（端口侧 `host.now()`）。
import { Ditto } from "../../../../src/LFW/ditto/Instance";
import { Expression } from "../../../../src/LFW/base/Expression";
import { Factory } from "../../../../src/LFW/Factory";
import { LFW } from "../../../../src/LFW/LFW";
import { get_val_getter_from_stage } from "../../../../src/LFW/loader/get_val_getter_from_stage";
import { PlayerInfo } from "../../../../src/LFW/PlayerInfo";

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

const render_value = (v: unknown): string => renderValue(v);
const num = (n: number): string => renderValue(n);

let clock_ms = 0;

function install_ditto(): void {
  class FakeSounds {
    constructor(_lfw: unknown) {
      push("snd_init");
    }
    dispose(): void {
      push("snd_dispose");
    }
    play_bgm(): () => void {
      push("snd_bgm");
      return () => undefined;
    }
    stop_bgm(): void {
      push("snd_stop");
    }
    play(): void {
      push("snd_play");
    }
    play_with_load(): void {
      push("snd_load");
    }
  }
  class FakeImageMgr {
    constructor(_lfw: unknown) {
      push("img_init");
    }
    measure_text(): string {
      push("measure");
      return "";
    }
  }
  class FakeKeyboard {
    callback = { add: () => push("kbd_cbadd") };
    constructor(_lfw: unknown) {
      push("kbd_init");
    }
    dispose(): void {
      push("kbd_dispose");
    }
  }
  class FakePointings {
    callback = { add: () => push("pt_cbadd") };
    constructor() {
      push("pt_init");
    }
    dispose(): void {
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
    get() {
      return Promise.resolve(undefined);
    },
    put() {},
    del() {
      return Promise.resolve();
    },
  };
  const zip = {
    forget_stored(type: string, version: number) {
      push(`zip:forget|${type}|${num(version)}`);
      return Promise.resolve();
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
    MD5: () => "",
    JSON5: { parse: (s: string) => JSON.parse(s), stringify: (v: unknown) => JSON.stringify(v) },
    Zip: zip as never,
    Sounds: FakeSounds as never,
    Keyboard: FakeKeyboard as never,
    Pointings: FakePointings as never,
    FullScreen: class {} as never,
    Importer: {} as never,
    Cache: cache as never,
    Vector3: class {
      x = 0;
      y = 0;
      z = 0;
    } as never,
    Vector2: class {
      x = 0;
      y = 0;
    } as never,
    WorldRender: FakeWorldRender as never,
    UINodeRenderer: class {} as never,
    ImageMgr: FakeImageMgr as never,
    UIInputHandle: FakeUIInputHandle as never,
    XML: {} as never,
    warn: (...args: unknown[]) => push("warn|" + args.map((a) => render_value(a)).join("~")),
    error: (...args: unknown[]) => push("error|" + args.map((a) => render_value(a)).join("~")),
    Log: (...args: unknown[]) => push("Log|" + args.map((a) => render_value(a)).join("~")),
    debug: (...args: unknown[]) => push("debug|" + args.map((a) => render_value(a)).join("~")),
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

function listen(): void {
  for (const name of CB_KEYS) {
    (lfw.callbacks as unknown as Rec).on(name, (...args: unknown[]) => {
      const parts: string[] = [`cb|${name}`];
      for (const a of args) {
        if (a === lfw) parts.push("self");
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

function main(): void {
  install_ditto();
  (Date as unknown as Rec).now = () => 12345;
  lfw = new LFW(false);
  listen();

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
      lfw.on_key_down(e as never);
      push(`kbdown|${key}|${num(times)}|${dev}|int=${(e as Rec)["_int"] ? 1 : 0}`);
    } else if (op === "kbup") {
      const key = nextKey();
      const e = { key };
      lfw.on_key_up(e as never);
      push(`kbup|${key}`);
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
    } else {
      fail(`unknown op '${op}'`);
    }
  }

  process.stdout.write(log.join("\n") + "\n");
}

let ent_seq = 0;
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
