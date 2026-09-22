import { createHash, createHmac, randomBytes } from "node:crypto";
import WebSocket from "ws";
import { decode_packets, encode_packet } from "./bilibili.mjs";

const OP_HEARTBEAT = 2;
const HEARTBEAT_INTERVAL = 20_000;
const DELAY_DEFAULT = 3000;
const DELAY_MAX = 30_000;

const delay = (ms) => new Promise((r) => setTimeout(r, ms));

class AuthError extends Error { }

export function sign_headers(access_key, secret, body) {
  const headers = {
    "x-bili-timestamp": String(Math.floor(Date.now() / 1000)),
    "x-bili-signature-method": "HMAC-SHA256",
    "x-bili-signature-nonce": randomBytes(12).toString("hex"),
    "x-bili-accesskeyid": access_key,
    "x-bili-signature-version": "1.0",
    "x-bili-content-md5": createHash("md5").update(body).digest("hex"),
  };
  const content = Object.keys(headers)
    .sort()
    .map((k) => `${k}:${headers[k]}`)
    .join("\n");
  return {
    ...headers,
    Authorization: createHmac("sha256", secret).update(content).digest("hex"),
    "Content-Type": "application/json",
    Accept: "application/json",
  };
}

export function parse_open_event(msg) {
  const cmd = String(msg.cmd ?? "");
  const d = msg.data ?? {};
  switch (cmd) {
    case "LIVE_OPEN_PLATFORM_DM":
      return { kind: "danmu", uid: String(d.uid ?? d.open_id ?? 0), name: String(d.uname ?? ""), text: String(d.msg ?? "") };
    case "LIVE_OPEN_PLATFORM_SEND_GIFT":
      return { kind: "gift", uid: String(d.uid ?? 0), name: String(d.uname ?? ""), text: `${d.gift_name ?? "礼物"}x${d.gift_num ?? 1}` };
    case "LIVE_OPEN_PLATFORM_LIKE":
      return { kind: "like", uid: String(d.uid ?? 0), name: String(d.uname ?? "") };
    case "LIVE_OPEN_PLATFORM_SUPER_CHAT":
      return { kind: "superchat", uid: String(d.uid ?? 0), name: String(d.uname ?? ""), text: String(d.message ?? "") };
    case "LIVE_OPEN_PLATFORM_GUARD":
      return { kind: "guard", uid: String(d.uid ?? 0), name: String(d.uname ?? ""), text: String(d.guard_level ?? "") };
    case "LIVE_OPEN_PLATFORM_LIVE_START":
      return { kind: "live" };
    case "LIVE_OPEN_PLATFORM_LIVE_END":
      return { kind: "live_end" };
    default:
      return null;
  }
}

export class OpenDanmuClient {
  constructor({ host = "https://live-open.biliapi.com", code, app_id, access_key, secret, on_event, on_status, debug = false }) {
    this.host = host;
    this.code = code;
    this.app_id = Number(app_id);
    this.access_key = access_key;
    this.secret = secret;
    this.on_event = on_event;
    this.on_status = on_status ?? (() => { });
    this.debug = debug;
    this.stopped = false;
    this.retry_ms = DELAY_DEFAULT;
    this.session_info = null;
    this.game_id = "";
  }

  async start() {
    while (!this.stopped) {
      try {
        this.session_info = this.session_info ?? await this.app_start();
        await this.session();
        this.retry_ms = DELAY_DEFAULT;
      } catch (e) {
        this.on_status("error", e.message ?? String(e));
        if (e instanceof AuthError) this.session_info = null;
      }
      if (this.stopped) break;
      this.on_status("info", `${this.retry_ms / 1000} 秒后重试...`);
      await delay(this.retry_ms);
      this.retry_ms = Math.min(this.retry_ms * 2, DELAY_MAX);
    }
  }

  async stop() {
    this.stopped = true;
    this.ws?.close();
    if (this.game_id) await this.post_json("/v2/app/end", JSON.stringify({ game_id: this.game_id, app_id: this.app_id })).catch(() => { });
  }

  async post_json(path, body) {
    const res = await fetch(`${this.host}${path}`, {
      method: "POST",
      headers: sign_headers(this.access_key, this.secret, body),
      body,
    });
    if (!res.ok) throw new Error(`HTTP ${res.status}: ${path}`);
    return res.json();
  }

  async app_start() {
    const ret = await this.post_json("/v2/app/start", JSON.stringify({ code: this.code, app_id: this.app_id }));
    if (ret.code !== 0) throw new Error(`app/start 失败: ${ret.message ?? ret.code}（code=${this.code}）`);
    const info = ret.data?.websocket_info;
    if (!info?.wss_link?.length) throw new Error("app/start 未返回 websocket_info");
    this.game_id = String(ret.data?.game_info?.game_id ?? "");
    this.on_status("ok", `应用已启动 game_id=${this.game_id}，长连 ${info.wss_link[0]}`);
    return { auth_body: String(info.auth_body), wss_link: info.wss_link };
  }

  async session() {
    const { auth_body, wss_link } = this.session_info;
    await new Promise((resolve, reject) => {
      const ws = this.ws = new WebSocket(wss_link[0]);
      let heartbeat_timer = null;
      let app_heartbeat_timer = null;
      let auth_error = null;
      const clear_timers = () => {
        clearInterval(heartbeat_timer);
        clearInterval(app_heartbeat_timer);
      };
      const send_app_heartbeat = async () => {
        try {
          const body = JSON.stringify({ game_id: this.game_id });
          const ret = await this.post_json("/v2/app/heartbeat", body);
          if (ret.code !== 0) this.on_status("error", `app/heartbeat 返回 ${ret.message ?? ret.code}`);
        } catch (e) {
          this.on_status("error", `app/heartbeat 失败: ${e.message}`);
        }
      };
      ws.on("open", () => {
        ws.send(encode_packet(7, auth_body));
        heartbeat_timer = setInterval(() => ws.send(encode_packet(OP_HEARTBEAT)), HEARTBEAT_INTERVAL);
        app_heartbeat_timer = setInterval(send_app_heartbeat, HEARTBEAT_INTERVAL);
      });
      ws.on("message", (data) => {
        for (const pkt of decode_packets(data)) {
          if (pkt.operation !== 5 && pkt.operation !== 8) continue;
          let json;
          try {
            json = JSON.parse(pkt.body.toString("utf8"));
          } catch {
            continue;
          }
          if (typeof json.code === "number" && json.cmd == null) {
            if (json.code === 0) this.on_status("ok", "长连鉴权成功");
            else {
              auth_error = new AuthError(`长连鉴权失败 code=${json.code} ${json.message ?? ""}`.trim());
              ws.close();
            }
            continue;
          }
          const ev = parse_open_event(json);
          if (ev) this.on_event(ev);
          else if (this.debug) this.on_status("debug", `忽略消息 ${json.cmd}`);
        }
      });
      ws.on("close", () => {
        clear_timers();
        this.on_status("info", "长连已断开");
        if (auth_error) reject(auth_error);
        else resolve();
      });
      ws.on("error", (e) => this.on_status("error", `长连错误: ${e.message}`));
    });
  }
}
