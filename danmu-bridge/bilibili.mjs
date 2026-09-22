import { brotliDecompressSync, inflateSync } from "node:zlib";
import WebSocket from "ws";

const HEADER_SIZE = 16;
const OP_HEARTBEAT = 2;
const OP_HEARTBEAT_REPLY = 3;
const OP_MESSAGE = 5;
const OP_AUTH = 7;
const OP_AUTH_REPLY = 8;
const HEARTBEAT_INTERVAL = 30_000;
const RECV_TIMEOUT = 90_000;
const UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36";

const delay = (ms) => new Promise((r) => setTimeout(r, ms));

export function encode_packet(operation, body = "", protover = 1) {
  const body_buf = Buffer.from(body);
  const buf = Buffer.alloc(HEADER_SIZE + body_buf.length);
  buf.writeUInt32BE(buf.length, 0);
  buf.writeUInt16BE(HEADER_SIZE, 4);
  buf.writeUInt16BE(protover, 6);
  buf.writeUInt32BE(operation, 8);
  buf.writeUInt32BE(1, 12);
  body_buf.copy(buf, HEADER_SIZE);
  return buf;
}

export function decode_packets(buf, ret = []) {
  let offset = 0;
  while (offset + HEADER_SIZE <= buf.length) {
    const len = buf.readUInt32BE(offset);
    if (len < HEADER_SIZE || offset + len > buf.length) break;
    const header_len = buf.readUInt16BE(offset + 4);
    const protover = buf.readUInt16BE(offset + 6);
    const operation = buf.readUInt32BE(offset + 8);
    const body = buf.subarray(offset + header_len, offset + len);
    if (protover === 2) decode_packets(inflateSync(body), ret);
    else if (protover === 3) decode_packets(brotliDecompressSync(body), ret);
    else ret.push({ operation, protover, body });
    offset += len;
  }
  return ret;
}

export function parse_event(msg) {
  const cmd = String(msg.cmd ?? "");
  if (cmd.startsWith("DANMU_MSG")) {
    const info = msg.info;
    if (!Array.isArray(info)) return null;
    const user = info[0]?.[15]?.user;
    return {
      kind: "danmu",
      uid: String(info[2]?.[0] ?? user?.uid ?? 0),
      name: String(info[2]?.[1] ?? user?.uname ?? ""),
      text: String(info[1] ?? ""),
    };
  }
  if (cmd.startsWith("INTERACT_WORD")) {
    const d = msg.data;
    if (!d) return null;
    const kinds = { 1: "enter", 2: "follow", 3: "share", 6: "follow", 7: "follow" };
    return { kind: kinds[d.msg_type] ?? "interact", uid: String(d.uid ?? 0), name: String(d.uname ?? "") };
  }
  if (cmd.startsWith("LIKE_INFO_V3_CLICK")) {
    const d = msg.data;
    if (!d) return null;
    return { kind: "like", uid: String(d.uid ?? 0), name: String(d.uname ?? "") };
  }
  if (cmd.startsWith("SEND_GIFT")) {
    const d = msg.data;
    if (!d) return null;
    return { kind: "gift", uid: String(d.uid ?? 0), name: String(d.uname ?? ""), text: `${d.giftName ?? "礼物"}x${d.num ?? 1}` };
  }
  if (cmd.startsWith("SUPER_CHAT_MESSAGE")) {
    const d = msg.data;
    if (!d) return null;
    return { kind: "superchat", uid: String(d.uid ?? 0), name: String(d.user_info?.uname ?? ""), text: String(d.message ?? "") };
  }
  if (cmd.startsWith("GUARD_BUY")) {
    const d = msg.data;
    if (!d) return null;
    return { kind: "guard", uid: String(d.uid ?? 0), name: String(d.username ?? ""), text: String(d.gift_name ?? "") };
  }
  if (cmd.startsWith("ENTRY_EFFECT")) {
    const d = msg.data;
    if (!d) return null;
    return { kind: "enter", uid: String(d.uid ?? 0), name: "" };
  }
  if (cmd === "LIVE") return { kind: "live" };
  if (cmd === "PREPARING") return { kind: "preparing" };
  return null;
}

async function fetch_json(url, headers = {}) {
  const res = await fetch(url, { headers: { "User-Agent": UA, ...headers } });
  if (!res.ok) throw new Error(`HTTP ${res.status}: ${url}`);
  return res.json();
}

export async function resolve_room_id(input, headers = {}) {
  const ret = await fetch_json(`https://api.live.bilibili.com/room/v1/Room/room_init?id=${encodeURIComponent(input)}`, headers);
  if (ret.code !== 0) throw new Error(`room_init 失败: ${ret.message ?? ret.code}`);
  return String(ret.data.room_id);
}

export async function fetch_danmu_info(room_id, headers = {}) {
  try {
    const ret = await fetch_json(`https://api.live.bilibili.com/xlive/web-room/v1/index/getDanmuInfo?id=${room_id}&type=0`, headers);
    if (ret.code === 0) {
      const hosts = ret.data?.host_list ?? [];
      if (hosts.length)
        return { token: ret.data.token ?? "", host: hosts[0].host, port: hosts[0].wss_port ?? 443 };
    }
  } catch {
    void 0;
  }
  const legacy = await fetch_json(`https://api.live.bilibili.com/room/v1/Danmu/getConf?room_id=${room_id}&platform=pc&player=web`, headers);
  if (legacy.code !== 0) throw new Error(`getConf 失败: ${legacy.message ?? legacy.code}`);
  const host = legacy.data?.host_server_list?.[0];
  if (!host) throw new Error("getConf 未返回 host_server_list");
  return { token: legacy.data?.token ?? "", host: host.host, port: host.wss_port ?? 443 };
}

export async function ensure_buvid(cookie) {
  if (/buvid3=/.test(cookie)) return cookie;
  try {
    const ret = await fetch_json("https://api.bilibili.com/x/frontend/finger/spi");
    const b3 = ret.data?.b_3;
    const b4 = ret.data?.b_4;
    if (b3) return `${cookie ? `${cookie}; ` : ""}buvid3=${b3}${b4 ? `; buvid4=${b4}` : ""}`;
  } catch {
    void 0;
  }
  return cookie;
}

export class BilibiliDanmuClient {
  constructor({ room, cookie = "", uid = 0, on_event, on_status, debug = false }) {
    this.room = room;
    this.cookie = cookie;
    this.uid = uid;
    this.on_event = on_event;
    this.on_status = on_status ?? (() => { });
    this.debug = debug;
    this.stopped = false;
    this.retry_ms = 3000;
  }

  async start() {
    while (!this.stopped) {
      try {
        await this.session();
        this.retry_ms = 3000;
      } catch (e) {
        this.on_status("error", e.message ?? String(e));
      }
      if (this.stopped) break;
      this.on_status("info", `${this.retry_ms / 1000} 秒后重连...`);
      await delay(this.retry_ms);
      this.retry_ms = Math.min(this.retry_ms * 2, 30_000);
    }
  }

  stop() {
    this.stopped = true;
    this.ws?.close();
  }

  async session() {
    this.resolved_cookie = this.resolved_cookie ?? await ensure_buvid(this.cookie);
    const headers = { referer: "https://live.bilibili.com/" };
    if (this.resolved_cookie) headers.cookie = this.resolved_cookie;
    const room_id = await resolve_room_id(this.room, headers);
    const { token, host, port } = await fetch_danmu_info(room_id, headers);
    this.on_status("info", `房间 ${this.room} -> ${room_id}，连接 wss://${host}:${port}/sub`);
    await new Promise((resolve) => {
      const ws = this.ws = new WebSocket(`wss://${host}:${port}/sub`);
      let heartbeat_timer = null;
      let watchdog_timer = null;
      let last_recv = Date.now();
      const clear_timers = () => {
        clearInterval(heartbeat_timer);
        clearInterval(watchdog_timer);
      };
      ws.on("open", () => {
        ws.send(encode_packet(OP_AUTH, JSON.stringify({
          uid: this.uid,
          roomid: Number(room_id),
          protover: 3,
          platform: "web",
          type: 2,
          key: token,
        })));
        heartbeat_timer = setInterval(() => ws.send(encode_packet(OP_HEARTBEAT)), HEARTBEAT_INTERVAL);
        watchdog_timer = setInterval(() => {
          if (Date.now() - last_recv > RECV_TIMEOUT) ws.terminate();
        }, 15_000);
      });
      ws.on("message", (data) => {
        last_recv = Date.now();
        for (const pkt of decode_packets(data)) {
          if (pkt.operation === OP_AUTH_REPLY) {
            const reply = JSON.parse(pkt.body.toString("utf8"));
            if (reply.code === 0) this.on_status("ok", "弹幕连接已认证");
            else {
              this.on_status("error", `认证失败 code=${reply.code}`);
              ws.close();
            }
          } else if (pkt.operation === OP_HEARTBEAT_REPLY) {
            if (this.debug) this.on_status("debug", `人气值 ${pkt.body.readUInt32BE(0)}`);
          } else if (pkt.operation === OP_MESSAGE) {
            let json;
            try {
              json = JSON.parse(pkt.body.toString("utf8"));
            } catch {
              continue;
            }
            const ev = parse_event(json);
            if (ev) this.on_event(ev);
            else if (this.debug) this.on_status("debug", `忽略消息 ${json.cmd}`);
          }
        }
      });
      ws.on("close", () => {
        clear_timers();
        this.on_status("info", "弹幕连接已断开");
        resolve();
      });
      ws.on("error", (e) => this.on_status("error", `弹幕连接错误: ${e.message}`));
    });
  }
}
