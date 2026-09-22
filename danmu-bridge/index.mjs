import { createServer } from "node:http";
import { existsSync, readFileSync } from "node:fs";
import { dirname, isAbsolute, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import JSON5 from "json5";
import { WebSocketServer } from "ws";
import { BilibiliDanmuClient } from "./bilibili.mjs";
import { OpenDanmuClient } from "./open.mjs";
import { make_scoreboard } from "./scores.mjs";

const HERE = dirname(fileURLToPath(import.meta.url));

function parse_args(argv) {
  const ret = {};
  for (let i = 0; i < argv.length; ++i) {
    const a = argv[i];
    if (!a.startsWith("--")) continue;
    const key = a.slice(2);
    const next = argv[i + 1];
    ret[key] = next && !next.startsWith("--") ? argv[++i] : true;
  }
  return ret;
}

function print_usage() {
  console.log(`用法: node index.mjs --mode <web|open> [选项]

选项可用 命令行 / 环境变量 / 配置文件 三种方式提供，优先级：命令行 > 环境变量 > 配置文件 > 默认值。

配置文件:
  --config <path>           指定配置文件；默认自动读取本目录下的 config.json5 或 config.json（字段见 config.example.json5）
  DANMU_BRIDGE_CONFIG       等价环境变量

模式 web（默认，直接连直播间弹幕流，无需审核）:
  --room <id>               直播间号（必填）
  --sessdata <value>        登录 cookie 的 SESSDATA（可选，登录态更稳）
  --uid <id>                登录账号的 DedeUserID（配 --sessdata 用，可选）

模式 open（官方直播开放平台，需入驻审核 + 申请消息类型）:
  --app-id <id>             应用 ID（即项目的「项目ID」）
  --access-key <key>        access_key
  --access-key-secret <sk>  access_key_secret
  --code <code>             主播身份码（主播启动你开发中的互动玩法后产生，联调可用官方测试入口）
  --open-host <url>         开放平台地址（默认 https://live-open.biliapi.com）

弹幕指令（关键词均为「包含」匹配）:
  --join <kw1,kw2>          触发入队的关键词，留空/不填 = 任意弹幕都入队
  --pick <kw=角色,...>      指定角色入队/切换（如 戴维斯=Davis；仅常规角色；关键词忽略大小写）
  --cheer <kw1,kw2>         触发应援的关键词（默认 加油,666,应援）
  --leave <kw1,kw2>         触发退出的关键词（离队/场上退场），默认不启用
  进入直播间:               合作闯关开局/每关生成 4 个 Template，进入即认领一个（最多 32 人）；其他模式场上未满时以 Template 入场
                            （认领/入场后发角色关键词即可切换）

其他:
  --port <port>             游戏页面连接端口（默认 8066）
  --host <host>             监听地址（默认 127.0.0.1）
  --join-cooldown <ms>      同一观众两次入队尝试的最小间隔（默认 5000）
  --scores <path>           战绩存档文件（默认本目录 scores.json）
  --debug                   打印所有收到的弹幕事件
  --dry                     仅打印解析后的配置（密钥脱敏）后退出，用于校验配置

对应环境变量: BILI_APP_ID / BILI_ACCESS_KEY / BILI_ACCESS_SECRET / BILI_CODE / BILI_ROOM_ID / BILI_SESSDATA / BILI_UID / DANMU_BRIDGE_MODE / DANMU_BRIDGE_CONFIG / DANMU_JOIN_KEYWORDS / DANMU_PICK_KEYWORDS / DANMU_CHEER_KEYWORDS / DANMU_LEAVE_KEYWORDS / DANMU_SCORES_FILE
`);
}

function load_config_file() {
  const arg = args.config;
  if (arg === true) {
    console.error("--config 需要指定文件路径");
    process.exit(1);
  }
  const explicit = typeof arg === "string" ? arg : process.env.DANMU_BRIDGE_CONFIG ?? "";
  const candidates = explicit
    ? [explicit]
    : [join(HERE, "config.json5"), join(HERE, "config.json")];
  for (const candidate of candidates) {
    const path = isAbsolute(candidate) ? candidate : resolve(process.cwd(), candidate);
    if (!existsSync(path)) {
      if (explicit) {
        console.error(`找不到配置文件: ${path}`);
        process.exit(1);
      }
      continue;
    }
    try {
      return { path, data: JSON5.parse(readFileSync(path, "utf8")) ?? {} };
    } catch (e) {
      console.error(`配置文件解析失败: ${path}\n${e.message}`);
      process.exit(1);
    }
  }
  return null;
}

function build_cookie(sessdata, uid) {
  if (!sessdata) return "";
  return `SESSDATA=${sessdata}${uid ? `; DedeUserID=${uid}` : ""}`;
}

function split_list(s) {
  if (Array.isArray(s)) return s.map((v) => String(v).trim()).filter(Boolean);
  return String(s)
    .split(/[,，\s]+/)
    .map((v) => v.trim())
    .filter(Boolean);
}

function to_pairs(input) {
  if (Array.isArray(input))
    return input
      .map((v) => String(v).split("="))
      .filter((v) => v.length === 2 && v[0].trim() && v[1].trim())
      .map((v) => [v[0].trim(), v[1].trim()]);
  if (input && typeof input === "object")
    return Object.entries(input)
      .map(([k, v]) => [String(k).trim(), String(v).trim()])
      .filter((v) => v[0] && v[1]);
  const ret = [];
  for (const piece of split_list(input)) {
    const i = piece.indexOf("=");
    if (i <= 0 || i >= piece.length - 1) continue;
    ret.push([piece.slice(0, i).trim(), piece.slice(i + 1).trim()]);
  }
  return ret;
}

function mask(v) {
  return v ? `${String(v).slice(0, 4)}****` : "";
}

const args = parse_args(process.argv.slice(2));
const config_file = load_config_file();
const file = config_file?.data ?? {};
const pick = (cli, env, conf, def) => cli ?? env ?? conf ?? def;

const app_id = String(pick(args["app-id"], process.env.BILI_APP_ID, file.app_id, ""));
const access_key = String(pick(args["access-key"], process.env.BILI_ACCESS_KEY, file.access_key, ""));
const access_secret = String(pick(args["access-key-secret"], process.env.BILI_ACCESS_SECRET, file.access_key_secret, ""));
const code = String(pick(args.code, process.env.BILI_CODE, file.code, ""));
const room = String(pick(args.room, process.env.BILI_ROOM_ID, file.room, ""));
const mode = String(pick(args.mode, process.env.DANMU_BRIDGE_MODE, file.mode, app_id && access_key && access_secret && code ? "open" : "web"));
const open_ready = !!(app_id && access_key && access_secret && code);
const sessdata = String(pick(args.sessdata, process.env.BILI_SESSDATA, file.sessdata, ""));
const uid = Number(pick(args.uid, process.env.BILI_UID, file.uid, 0));
const invalid = (mode === "web" && !room) || (mode === "open" && !open_ready);

if (args.help || invalid) {
  print_usage();
  process.exit(args.help || !invalid ? 0 : 1);
}

const config = {
  mode,
  room,
  cookie: build_cookie(sessdata, uid),
  uid,
  port: Number(pick(args.port, process.env.DANMU_BRIDGE_PORT, file.port, 8066)),
  host: String(pick(args.host, process.env.DANMU_BRIDGE_HOST, file.host, "127.0.0.1")),
  join_keywords: split_list(pick(args.join, process.env.DANMU_JOIN_KEYWORDS, file.join_keywords, "")),
  leave_keywords: split_list(pick(args.leave, process.env.DANMU_LEAVE_KEYWORDS, file.leave_keywords, "")),
  cheer_keywords: split_list(pick(args.cheer, process.env.DANMU_CHEER_KEYWORDS, file.cheer_keywords, "加油,666,应援")),
  character_keywords: to_pairs(pick(args.pick, process.env.DANMU_PICK_KEYWORDS, file.character_keywords, {})),
  join_cooldown: Number(pick(args["join-cooldown"], process.env.DANMU_JOIN_COOLDOWN, file.join_cooldown, 5000)),
  debug: args.debug === true || file.debug === true,
  dry: args.dry === true,
  config_path: config_file?.path ?? "",
  scores_file: String(pick(args.scores, process.env.DANMU_SCORES_FILE, file.scores_file, "")),
  score_weights: { kills: 10, spawns: 1, cheers: 1, deads: 0, damages: 0, ...(file.score_weights ?? {}) },
  open: {
    host: String(pick(args["open-host"], process.env.BILI_OPEN_HOST, file.open_host, "https://live-open.biliapi.com")),
    app_id,
    access_key,
    secret: access_secret,
    code,
  },
};

if (config.dry) {
  console.log(JSON.stringify({
    mode: config.mode,
    room: config.room,
    sessdata: config.cookie ? "已设置" : "",
    uid: config.uid,
    port: config.port,
    host: config.host,
    join_keywords: config.join_keywords,
    leave_keywords: config.leave_keywords,
    cheer_keywords: config.cheer_keywords,
    character_keywords: config.character_keywords,
    join_cooldown: config.join_cooldown,
    debug: config.debug,
    open: {
      host: config.open.host,
      app_id: config.open.app_id,
      access_key: mask(config.open.access_key),
      access_key_secret: mask(config.open.secret),
      code: mask(config.open.code),
    },
  }, null, 2));
  process.exit(0);
}

const board = make_scoreboard({
  file: config.scores_file || join(HERE, "scores.json"),
  weights: config.score_weights,
});
const SCORES_SAVE_MS = Number(process.env.DANMU_SCORES_SAVE_MS ?? 30_000);

const stamp = () => new Date().toLocaleTimeString("zh-CN", { hour12: false });
function log(msg) {
  console.log(`[${stamp()}] ${msg}`);
}

const last_join = new Map();
const last_enter = new Map();
const last_cheer = new Map();
const last_touch = new Map();
const TOUCH_INTERVAL = 30_000;
const CHEER_INTERVAL = 2000;
const ENTER_INTERVAL = 10_000;

function send(action) {
  const raw = JSON.stringify(action);
  let sent = 0;
  for (const client of wss.clients) {
    if (client.readyState !== 1) continue;
    client.send(raw);
    ++sent;
  }
  if (config.debug || action.type !== "touch")
    log(`→ ${action.type} ${action.name ?? ""}${action.uid ? ` (${action.uid})` : ""}${sent ? "" : "（无游戏页面连接）"}`);
}

function try_join(ev, oid) {
  const now = Date.now();
  if (now - (last_join.get(ev.uid) ?? 0) < config.join_cooldown) return;
  last_join.set(ev.uid, now);
  send({ type: "join", uid: ev.uid, name: ev.name || `用户${ev.uid}`, oid: oid || void 0 });
}

function try_enter(ev) {
  const now = Date.now();
  if (now - (last_enter.get(ev.uid) ?? 0) < ENTER_INTERVAL) return;
  last_enter.set(ev.uid, now);
  send({ type: "enter", uid: ev.uid, name: ev.name || `用户${ev.uid}` });
}

function try_cheer(ev) {
  const now = Date.now();
  if (now - (last_cheer.get(ev.uid) ?? 0) < CHEER_INTERVAL) return;
  last_cheer.set(ev.uid, now);
  send({ type: "cheer", uid: ev.uid, name: ev.name });
}

function touch(uid) {
  const now = Date.now();
  if (now - (last_touch.get(uid) ?? 0) < TOUCH_INTERVAL) return;
  last_touch.set(uid, now);
  send({ type: "touch", uid });
}

function match_keywords(text, keywords) {
  const lower = text.toLowerCase();
  return keywords.some((kw) => lower.includes(kw.toLowerCase()));
}

function match_join(text) {
  if (!config.join_keywords.length) return true;
  return match_keywords(text, config.join_keywords);
}

function match_cheer(text) {
  return match_keywords(text, config.cheer_keywords);
}

function match_pick(text) {
  const lower = text.toLowerCase();
  return config.character_keywords.find((v) => lower.includes(v[0].toLowerCase()));
}

function build_hints() {
  const hints = [];
  if (config.join_keywords.length) hints.push(`发「${config.join_keywords.slice(0, 2).join("」「")}」加入战斗`);
  else hints.push("发任意弹幕加入战斗");
  const picks = [];
  const seen = new Set();
  for (const [kw, oid] of config.character_keywords) {
    if (seen.has(oid)) continue;
    seen.add(oid);
    picks.push(kw);
  }
  if (picks.length) hints.push("发角色名换人：{picks}");
  if (config.cheer_keywords.length) hints.push(`发「${config.cheer_keywords.slice(0, 2).join("」「")}」应援回血`);
  if (config.leave_keywords.length) hints.push(`发「${config.leave_keywords[0]}」退场`);
  hints.push("进入直播间自动上场");
  hints.push("战死要重新发弹幕才能再上");
  return { hints, picks };
}

function on_event(ev) {
  if (config.debug) log(`event ${ev.kind} ${ev.name ?? ""} ${ev.text ?? ""}`.trimEnd());
  switch (ev.kind) {
    case "danmu": {
      if (!ev.uid) return;
      touch(ev.uid);
      if (config.leave_keywords.length && match_keywords(ev.text, config.leave_keywords)) {
        send({ type: "leave", uid: ev.uid, name: ev.name });
        return;
      }
      const pick = match_pick(ev.text);
      if (pick) try_join(ev, pick[1]);
      else if (match_join(ev.text)) try_join(ev);
      if (match_cheer(ev.text)) try_cheer(ev);
      break;
    }
    case "enter": {
      if (!ev.uid) return;
      touch(ev.uid);
      try_enter(ev);
      break;
    }
    case "follow":
    case "share":
    case "like":
    case "interact": {
      if (ev.uid) touch(ev.uid);
      break;
    }
    case "gift":
    case "guard":
    case "superchat": {
      if (!ev.uid) return;
      touch(ev.uid);
      try_cheer(ev);
      break;
    }
    case "live":
      log("主播开播");
      break;
    case "preparing":
      log("直播间未开播（等待开播）");
      break;
  }
}

const LEADERBOARD_HTML = `<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>弹幕互动积分榜</title>
<style>
  body { margin: 0; padding: 18px 22px; background: #0b1020; color: #e8ecff;
    font: 15px/1.6 Arial, "Microsoft YaHei", sans-serif; }
  h1 { margin: 0 0 4px; font-size: 20px; color: #ffd75e; }
  .sub { margin-bottom: 12px; color: #a8b6d8; font-size: 13px; }
  table { border-collapse: collapse; width: 100%; max-width: 640px; }
  th, td { padding: 5px 10px; text-align: left; border-bottom: 1px solid #ffffff1f; }
  th { color: #9fc2ff; font-size: 13px; font-weight: normal; }
  .rank { width: 44px; color: #ffd75e; }
  .score { color: #ffe08a; }
  .nums { color: #a8b6d8; font-size: 13px; }
</style>
</head>
<body>
<h1>弹幕互动积分榜</h1>
<div class="sub" id="sub">加载中...</div>
<table>
  <thead><tr><th>名次</th><th>观众</th><th>积分</th><th>击败</th><th>出场</th><th>应援</th><th>场次</th></tr></thead>
  <tbody id="rows"></tbody>
</table>
<script>
async function tick() {
  try {
    const resp = await fetch('/scores.json', { cache: 'no-store' });
    const data = await resp.json();
    const players = data.players || [];
    document.getElementById('rows').innerHTML = players.map(function (p, i) {
      return '<tr><td class="rank">' + (i + 1) + '</td><td>' + esc(p.name) + '</td>' +
        '<td class="score">' + p.score + '</td><td class="nums">' + p.kills + '</td>' +
        '<td class="nums">' + p.spawns + '</td><td class="nums">' + p.cheers + '</td>' +
        '<td class="nums">' + p.games + '</td></tr>';
    }).join('');
    document.getElementById('sub').textContent = '共 ' + players.length + ' 人 · 更新于 ' +
      (data.updated_at ? new Date(data.updated_at).toLocaleTimeString('zh-CN', { hour12: false }) : '--');
  } catch (e) {
    document.getElementById('sub').textContent = '数据读取失败，稍后重试';
  }
}
function esc(s) {
  return String(s == null ? '' : s).replace(/[&<>"']/g, function (c) {
    return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c];
  });
}
setInterval(tick, 10000);
tick();
</script>
</body>
</html>`;

const server = createServer((req, res) => {
  const path = (req.url ?? "/").split("?")[0];
  if (path === "/scores.json") {
    res.writeHead(200, { "content-type": "application/json; charset=utf-8", "access-control-allow-origin": "*" });
    res.end(JSON.stringify({ updated_at: board.updated_at, players: board.top(100) }));
    return;
  }
  if (path === "/" || path === "/index.html") {
    res.writeHead(200, { "content-type": "text/html; charset=utf-8" });
    res.end(LEADERBOARD_HTML);
    return;
  }
  res.writeHead(404, { "content-type": "text/plain; charset=utf-8" });
  res.end("not found");
});

const wss = new WebSocketServer({ server });
wss.on("connection", (ws, req) => {
  log(`游戏页面已连接 (${req.socket.remoteAddress})`);
  const { hints, picks } = build_hints();
  ws.send(JSON.stringify({ type: "hint", texts: hints, picks }));
  ws.on("message", (raw) => {
    try {
      const msg = JSON.parse(raw.toString());
      if (msg.type === "state")
        log(`[游戏] ${msg.mode ?? ""} 排队 ${msg.queue ?? 0} / 场上 ${msg.on_stage ?? 0}${msg.stage ? ` / 关卡 ${msg.stage}` : ""}`);
      else if (msg.type === "stats") {
        const changed = board.merge(msg.stats);
        if (config.debug) log(`[战绩] 同步 ${changed} 人，累计 ${board.size()} 人`);
      }
    } catch {
      return;
    }
  });
  ws.on("close", () => log("游戏页面已断开"));
});

server.listen(config.port, config.host, () => {
  log(`服务已启动: http://${config.host}:${config.port}（榜单页 / 与游戏 ws 同端口）`);
  log(`战绩存档: ${config.scores_file || join(HERE, "scores.json")}（已加载 ${board.size()} 人）`);
});

const on_status = (level, msg) => {
  if (level === "debug" && !config.debug) return;
  log(`[弹幕] ${msg}`);
};

const client = config.mode === "open"
  ? new OpenDanmuClient({ ...config.open, debug: config.debug, on_event, on_status })
  : new BilibiliDanmuClient({ room: config.room, cookie: config.cookie, uid: config.uid, debug: config.debug, on_event, on_status });

log(`配置: ${config.config_path || "未使用配置文件（可复制 config.example.json5 为 config.json5）"}`);
log(`来源: ${config.mode === "open" ? `官方开放平台 app_id=${config.open.app_id}` : `直播间弹幕协议（房间 ${config.room}${config.cookie ? "，带登录态" : "，游客"}）`}`);
log(`入队规则: ${config.join_keywords.length ? `包含关键词 ${config.join_keywords.join("/")}` : "任意弹幕"}${config.character_keywords.length ? `；指定角色 ${config.character_keywords.map((v) => v[0]).join("/")}（仅常规角色）` : ""}`);
log(`应援规则: 包含关键词 ${config.cheer_keywords.join("/")}，礼物/上舰/SC 也会应援`);
log(`退出规则: ${config.leave_keywords.length ? `包含关键词 ${config.leave_keywords.join("/")}` : "未启用（--leave 可配置）"}`);
log(`无离场事件：排队时超过 5 分钟无任何互动会被引擎自动移出队列（DanmuGameLogic.QUEUE_IDLE_TIMEOUT）`);
client.start();

export async function stop_bridge() {
  try {
    await client.stop();
  } catch {
    void 0;
  }
  board.save();
}

process.on("SIGINT", async () => {
  log("正在退出...");
  await stop_bridge();
  process.exit(0);
});

setInterval(() => board.save(), SCORES_SAVE_MS);

setInterval(() => {
  const now = Date.now();
  for (const map of [last_join, last_enter, last_cheer, last_touch])
    for (const [uid, t] of map)
      if (now - t > 3600_000) map.delete(uid);
}, 600_000).unref();
