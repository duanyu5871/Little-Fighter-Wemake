const DICT = {
  "": {
    server_on: (addr) => `Multiplayer server: on (${addr})`,
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
    server_on: (addr) => `联机服务器：已开启（${addr}）`,
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
    server_on: (addr) => `連線伺服器：已開啟（${addr}）`,
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
const ZH_HANS = new Set(["zh-hans", "zh-cn", "zh-sg"]);

export function normalize_lang(lang) {
  const v = String(lang ?? "").trim().toLowerCase();
  if (!v) return "";
  if (ZH_HANS.has(v)) return "zh-hans";
  if (ZH_HANT.has(v)) return "zh-hant";
  if (v.startsWith("zh")) return "zh-hans";
  return "";
}

export function tray_text(lang, key, ...args) {
  const table = DICT[normalize_lang(lang)] ?? DICT[""];
  const val = table[key] ?? DICT[""][key];
  return typeof val === "function" ? val(...args) : String(val ?? key);
}
