import type { LFW } from "./LFW";
import type { ILFWCallback } from "./LFW/ILFWCallback";
import { DanmuGameLogic } from "./LFW/ui/component/DanmuGameLogic";
import type { UIComponent } from "./LFW/ui/component/UIComponent";

const LOG_TAG = "[danmu-bridge]";
const DEFAULT_URL = "ws://127.0.0.1:8066";
const RECONNECT_MS = 3000;
const STATE_INTERVAL_MS = 5000;

interface IBridgeAction {
  type?: string;
  uid?: string;
  name?: string;
}

function get_bridge_url(): string | null {
  const m = /[?&#]DANMU_WS=([^&#]+)/i.exec(window.location.href);
  if (!m) return null;
  const v = decodeURIComponent(m[1]).trim();
  if (!v) return null;
  if (/^wss?:\/\//i.test(v)) return v;
  if (v === "1" || v.toLowerCase() === "true") return DEFAULT_URL;
  return `ws://${v}`;
}

class DanmuBridge implements ILFWCallback {
  private logic: DanmuGameLogic | null = null;
  private ws: WebSocket | null = null;
  private reconnect_timer: ReturnType<typeof setTimeout> | null = null;
  private state_timer: ReturnType<typeof setInterval> | null = null;
  private warned = false;

  constructor(readonly lfw: LFW, readonly url: string) {
    lfw.callbacks.add(this);
    this.connect();
    this.state_timer = setInterval(() => this.report_state(), STATE_INTERVAL_MS);
  }
  on_component_broadcast(component: UIComponent, msg: string): void {
    if (msg === DanmuGameLogic.BROADCAST_ON_START) this.logic = component as DanmuGameLogic;
    else if (msg === DanmuGameLogic.BROADCAST_ON_STOP) this.logic = null;
  }
  release(): void {
    this.lfw.callbacks.del(this);
    if (this.reconnect_timer !== null) clearTimeout(this.reconnect_timer);
    if (this.state_timer !== null) clearInterval(this.state_timer);
    this.ws?.close();
    this.ws = null;
  }
  private connect(): void {
    const ws = this.ws = new WebSocket(this.url);
    ws.onopen = () => {
      this.warned = false;
      console.log(LOG_TAG, `已连接到弹幕桥 ${this.url}`);
    };
    ws.onmessage = (e) => this.handle(String(e.data));
    ws.onclose = () => {
      if (this.ws !== ws) return;
      this.ws = null;
      if (!this.warned) {
        this.warned = true;
        console.warn(LOG_TAG, `弹幕桥未连接（${this.url}），${RECONNECT_MS / 1000} 秒后重试`);
      }
      this.reconnect_timer = setTimeout(() => this.connect(), RECONNECT_MS);
    };
  }
  private handle(raw: string): void {
    let action: IBridgeAction;
    try {
      action = JSON.parse(raw) as IBridgeAction;
    } catch {
      return;
    }
    const logic = this.logic;
    if (!logic) return;
    switch (action.type) {
      case "join":
        if (action.uid && action.name) logic.join({ uid: action.uid, name: action.name });
        break;
      case "cheer":
        if (action.uid) logic.cheer(action.uid);
        break;
      case "touch":
        if (action.uid) logic.touch(action.uid);
        break;
      case "leave":
        if (action.uid) logic.leave(action.uid);
        break;
    }
  }
  private report_state(): void {
    const { ws, logic } = this;
    if (!ws || ws.readyState !== WebSocket.OPEN || !logic) return;
    const stats = logic.viewer_stats();
    ws.send(JSON.stringify({
      type: "state",
      mode: logic.mode,
      queue: logic.join_queue.size,
      on_stage: stats.filter((v) => v.alive).length,
      stage: logic.stage_name,
    }));
  }
}

const installed = new WeakSet<LFW>();

export function install_danmu_bridge_if_requested(lfw: LFW): void {
  const url = get_bridge_url();
  if (!url) return;
  if (installed.has(lfw)) return;
  installed.add(lfw);
  new DanmuBridge(lfw, url);
}
