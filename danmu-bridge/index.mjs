import { WebSocketServer } from "ws";
import { BilibiliDanmuClient } from "./bilibili.mjs";
import { OpenDanmuClient } from "./open.mjs";

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

const args = parse_args(process.argv.slice(2));
const app_id = String(args["app-id"] ?? process.env.BILI_APP_ID ?? "");
const access_key = String(args["access-key"] ?? process.env.BILI_ACCESS_KEY ?? "");
const access_secret = String(args["access-key-secret"] ?? process.env.BILI_ACCESS_SECRET ?? "");
const code = String(args.code ?? process.env.BILI_CODE ?? "");
const room = String(args.room ?? process.env.BILI_ROOM_ID ?? "");
const mode = String(args.mode ?? (app_id && access_key && access_secret && code ? "open" : "web"));
const open_ready = !!(app_id && access_key && access_secret && code);

if (args.help || (mode === "web" && !room) || (mode === "open" && !open_ready)) {
  console.log(`用法: node index.mjs --mode <web|open> [选项]

模式 web（默认，直接连直播间弹幕流，无需审核）:
  --room <id>            直播间号（必填）
  --sessdata <value>     登录 cookie 的 SESSDATA（可选，登录态更稳）
  --uid <id>             登录账号的 DedeUserID（配 --sessdata 用，可选）

模式 open（官方直播开放平台，需入驻审核 + 申请消息类型）:
  --app-id <id>             应用 ID
  --access-key <key>        access_key
  --access-key-secret <sk>  access_key_secret
  --code <code>             主播身份码（主播启动你开发中的互动玩法后产生，联调可用官方测试入口）
  --open-host <url>         开放平台地址（默认 https://live-open.biliapi.com）

通用选项:
  --port <port>          游戏页面连接端口（默认 8066）
  --host <host>          监听地址（默认 127.0.0.1）
  --join <kw1,kw2>       触发入队的关键词，留空/不填 = 任意弹幕都入队
  --cheer <kw1,kw2>      触发应援的关键词（默认 加油,666,应援）
  --join-cooldown <ms>   同一观众两次入队尝试的最小间隔（默认 5000）
  --debug                打印所有收到的弹幕事件

对应环境变量: BILI_APP_ID / BILI_ACCESS_KEY / BILI_ACCESS_SECRET / BILI_CODE / BILI_ROOM_ID / BILI_SESSDATA / BILI_UID
`);
  process.exit(0);
}

const config = {
  mode,
  room,
  cookie: build_cookie(args),
  uid: Number(args.uid ?? process.env.BILI_UID ?? 0),
  port: Number(args.port ?? process.env.DANMU_BRIDGE_PORT ?? 8066),
  host: String(args.host ?? process.env.DANMU_BRIDGE_HOST ?? "127.0.0.1"),
  join_keywords: split_list(args.join ?? process.env.DANMU_JOIN_KEYWORDS ?? ""),
  cheer_keywords: split_list(args.cheer ?? process.env.DANMU_CHEER_KEYWORDS ?? "加油,666,应援"),
  join_cooldown: Number(args["join-cooldown"] ?? process.env.DANMU_JOIN_COOLDOWN ?? 5000),
  debug: !!args.debug,
  open: {
    host: String(args["open-host"] ?? process.env.BILI_OPEN_HOST ?? "https://live-open.biliapi.com"),
    app_id,
    access_key,
    secret: access_secret,
    code,
  },
};

function build_cookie(a) {
  const sessdata = a.sessdata ?? process.env.BILI_SESSDATA ?? "";
  if (!sessdata) return "";
  const uid = a.uid ?? process.env.BILI_UID ?? "";
  return `SESSDATA=${sessdata}${uid ? `; DedeUserID=${uid}` : ""}`;
}

function split_list(s) {
  return String(s)
    .split(/[,，\s]+/)
    .map((v) => v.trim())
    .filter(Boolean);
}

const stamp = () => new Date().toLocaleTimeString("zh-CN", { hour12: false });
function log(msg) {
  console.log(`[${stamp()}] ${msg}`);
}

const last_join = new Map();
const last_cheer = new Map();
const last_touch = new Map();
const TOUCH_INTERVAL = 30_000;
const CHEER_INTERVAL = 2000;

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

function try_join(ev) {
  const now = Date.now();
  if (now - (last_join.get(ev.uid) ?? 0) < config.join_cooldown) return;
  last_join.set(ev.uid, now);
  send({ type: "join", uid: ev.uid, name: ev.name || `用户${ev.uid}` });
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

function match_join(text) {
  if (!config.join_keywords.length) return true;
  return config.join_keywords.some((kw) => text.includes(kw));
}

function match_cheer(text) {
  return config.cheer_keywords.some((kw) => text.includes(kw));
}

function on_event(ev) {
  if (config.debug) log(`event ${ev.kind} ${ev.name ?? ""} ${ev.text ?? ""}`.trimEnd());
  switch (ev.kind) {
    case "danmu": {
      if (!ev.uid) return;
      touch(ev.uid);
      if (match_join(ev.text)) try_join(ev);
      if (match_cheer(ev.text)) try_cheer(ev);
      break;
    }
    case "enter":
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

const wss = new WebSocketServer({ host: config.host, port: config.port });
wss.on("listening", () => log(`游戏页面连接服务已启动: ws://${config.host}:${config.port}`));
wss.on("connection", (ws, req) => {
  log(`游戏页面已连接 (${req.socket.remoteAddress})`);
  ws.on("message", (raw) => {
    try {
      const msg = JSON.parse(raw.toString());
      if (msg.type === "state")
        log(`[游戏] ${msg.mode ?? ""} 排队 ${msg.queue ?? 0} / 场上 ${msg.on_stage ?? 0}${msg.stage ? ` / 关卡 ${msg.stage}` : ""}`);
    } catch {
      return;
    }
  });
  ws.on("close", () => log("游戏页面已断开"));
});

const on_status = (level, msg) => {
  if (level === "debug" && !config.debug) return;
  log(`[弹幕] ${msg}`);
};

const client = config.mode === "open"
  ? new OpenDanmuClient({ ...config.open, debug: config.debug, on_event, on_status })
  : new BilibiliDanmuClient({ room: config.room, cookie: config.cookie, uid: config.uid, debug: config.debug, on_event, on_status });

log(`来源: ${config.mode === "open" ? `官方开放平台 app_id=${config.open.app_id}` : `直播间弹幕协议（房间 ${config.room}${config.cookie ? "，带登录态" : "，游客"}）`}`);
log(`入队规则: ${config.join_keywords.length ? `包含关键词 ${config.join_keywords.join("/")}` : "任意弹幕"}`);
log(`应援规则: 包含关键词 ${config.cheer_keywords.join("/")}，礼物/上舰/SC 也会应援`);
log(`无离场事件：排队时超过 5 分钟无任何互动会被引擎自动移出队列（DanmuGameLogic.QUEUE_IDLE_TIMEOUT）`);
client.start();

process.on("SIGINT", async () => {
  log("正在退出...");
  try {
    await client.stop();
  } catch {
    void 0;
  }
  process.exit(0);
});

setInterval(() => {
  const now = Date.now();
  for (const map of [last_join, last_cheer, last_touch])
    for (const [uid, t] of map)
      if (now - t > 3600_000) map.delete(uid);
}, 600_000).unref();
