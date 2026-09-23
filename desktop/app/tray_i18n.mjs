import { readdirSync, readFileSync } from "node:fs";
import { extname, join } from "node:path";
import JSON5 from "json5";

const DICT = {
  "": {
    server_on: "Multiplayer server: on (%1)",
    server_off: "Start multiplayer server",
    allow_lan: "Allow LAN connections",
    copy_server_addr: "Copy server address",
    allow_game_lan: "Allow LAN access to the game page",
    copy_game_addr: "Copy game page address",
    open_tool: "Open data tool (console)",
    copy_tool_cmd: "Copy data tool command",
    open_data_dir: "Open data folder",
    show_window: "Show game window",
    quit: "Quit",
  },
  "zh-hans": {
    server_on: "联机服务器：已开启（%1）",
    server_off: "开启联机服务器",
    allow_lan: "允许局域网连接",
    copy_server_addr: "复制联机地址",
    allow_game_lan: "允许局域网访问游戏页面",
    copy_game_addr: "复制游戏页面地址",
    open_tool: "打开数据工具（命令行）",
    copy_tool_cmd: "复制数据工具命令",
    open_data_dir: "打开数据目录",
    show_window: "显示游戏窗口",
    quit: "退出",
  },
  "zh-hant": {
    server_on: "連線伺服器：已開啟（%1）",
    server_off: "開啟連線伺服器",
    allow_lan: "允許區域網路連線",
    copy_server_addr: "複製連線位址",
    allow_game_lan: "允許區域網路存取遊戲頁面",
    copy_game_addr: "複製遊戲頁面位址",
    open_tool: "開啟資料工具（命令列）",
    copy_tool_cmd: "複製資料工具命令",
    open_data_dir: "開啟資料目錄",
    show_window: "顯示遊戲視窗",
    quit: "關閉",
  },
};

const ZH_HANT = new Set(["zh-hant", "zh-tw", "zh-hk", "zh-mo"]);

export function normalize_lang(lang) {
  const v = String(lang ?? "").trim().toLowerCase();
  if (!v) return "";
  if (v.startsWith("zh")) return ZH_HANT.has(v) ? "zh-hant" : "zh-hans";
  if (v === "en" || v.startsWith("en-")) return "";
  const dash = v.indexOf("-");
  return dash < 0 ? v : v.slice(0, dash);
}

/**
 * 解析界面语言：命令行 > auto（跟随游戏，兜底系统语言）
 * 返回 { lang, fixed }：fixed 为 true 时忽略游戏上报的语言
 */
export function resolve_lang(cli, locale) {
  const v = String(cli ?? "").trim().toLowerCase();
  if (!v || v === "auto") return { lang: normalize_lang(locale), fixed: false };
  return { lang: normalize_lang(v), fixed: true };
}

/**
 * 加载目录下的自定义文案：<语言码>.json5 / .json，每个文件是一个 { 键: 文案 } 对象
 * 覆盖或补充内置字典；%1/%2… 插值由 tray_text 处理
 */
export function load_tray_texts(dir) {
  const ret = { files: 0, keys: 0, errors: [] };
  let names;
  try {
    names = readdirSync(dir).sort();
  } catch {
    return ret;
  }
  for (const name of names) {
    const ext = extname(name).toLowerCase();
    if (ext !== ".json5" && ext !== ".json") continue;
    let obj;
    try {
      obj = JSON5.parse(readFileSync(join(dir, name), "utf8").replace(/^\uFEFF/, ""));
    } catch (e) {
      ret.errors.push(`${name}: ${e?.message ?? e}`);
      continue;
    }
    if (!obj || typeof obj !== "object" || Array.isArray(obj)) {
      ret.errors.push(`${name}: 内容应为一个对象`);
      continue;
    }
    const code = normalize_lang(name.slice(0, -ext.length));
    const dst = DICT[code] ?? (DICT[code] = {});
    ret.files++;
    for (const key in obj) {
      const v = obj[key];
      if (typeof v !== "string") continue;
      dst[key] = v;
      ret.keys++;
    }
  }
  return ret;
}

export function tray_text(lang, key, ...args) {
  const table = DICT[normalize_lang(lang)] ?? DICT[""];
  let v = table[key] ?? DICT[""][key] ?? key;
  if (typeof v !== "string") return key;
  for (let i = 0; i < args.length; ++i) v = v.split(`%${i + 1}`).join(String(args[i]));
  return v;
}
