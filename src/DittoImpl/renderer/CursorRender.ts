import { get_pointer_cursors, type IPointerCursor } from "@/LFW/cmds/CMD_POINTER_EVENTS";
import { current_connection } from "@/pages/network_test/current_connection";
import { CanvasTexture, Object3D, Sprite, SpriteMaterial } from "../_t";
import type { WorldRenderer } from "./WorldRenderer";

const COLORS = [0xff5b5b, 0x5b9dff, 0x5bd75b, 0xffd75b];
const TIMEOUT = 2000;
const EASE_TAU = 40;
const PIXEL_RATIO = 4;
const FONT_SIZE = 12;
const TIP_X = 12;
const TIP_Y = 12;
const TEXT_X = 26;
const TEXT_Y = 28;
const TEXT_OUTLINE: number[][] = [
  [-1, -1], [0, -1], [1, -1],
  [-1, 0], [1, 0],
  [-1, 1], [0, 1], [1, 1],
];

function cursor_canvas(name: string): { canvas: HTMLCanvasElement; w: number; h: number } {
  const canvas = document.createElement("canvas");
  const ctx = canvas.getContext("2d")!;
  ctx.font = `${FONT_SIZE}px sans-serif`;
  const text_w = name ? ctx.measureText(name).width : 0;
  const w = Math.ceil(Math.max(TIP_X + 10.4, name ? TEXT_X + text_w : 0)) + 2;
  const h = Math.ceil(Math.max(TIP_Y + 14.6, name ? TEXT_Y + FONT_SIZE + 4 : 0)) + 2;
  canvas.width = w * PIXEL_RATIO;
  canvas.height = h * PIXEL_RATIO;
  ctx.setTransform(PIXEL_RATIO, 0, 0, PIXEL_RATIO, 0, 0);
  ctx.font = `${FONT_SIZE}px sans-serif`;
  ctx.textBaseline = "top";
  ctx.beginPath();
  ctx.moveTo(TIP_X, TIP_Y);
  ctx.lineTo(TIP_X, TIP_Y + 12.7);
  ctx.lineTo(TIP_X + 3.7, TIP_Y + 9.4);
  ctx.lineTo(TIP_X + 5.9, TIP_Y + 14.6);
  ctx.lineTo(TIP_X + 8.6, TIP_Y + 13.4);
  ctx.lineTo(TIP_X + 6.4, TIP_Y + 8.3);
  ctx.lineTo(TIP_X + 10.4, TIP_Y + 8.3);
  ctx.closePath();
  ctx.fillStyle = "#ffffff";
  ctx.fill();
  ctx.lineJoin = "round";
  ctx.lineWidth = 1.5;
  ctx.strokeStyle = "rgba(0, 0, 0, 0.65)";
  ctx.stroke();
  if (name) {
    ctx.fillStyle = "rgba(0, 0, 0, 0.75)";
    for (let i = 0; i < TEXT_OUTLINE.length; i++) {
      const outline = TEXT_OUTLINE[i];
      ctx.fillText(name, TEXT_X + outline[0], TEXT_Y + outline[1]);
    }
    ctx.fillStyle = "#ffffff";
    ctx.fillText(name, TEXT_X, TEXT_Y);
  }
  return { canvas, w, h };
}

interface ICursorView {
  sprite: Sprite;
  name: string;
  rx: number;
  ry: number;
  shown: boolean;
}

export class CursorRender {
  readonly container = new Object3D();
  protected _views = new Map<string, ICursorView>();
  protected _colors = new Map<string, number>();
  constructor(protected renderer: WorldRenderer) {
    renderer.ui_fg_container.add(this.container);
  }
  protected color_of(from: string): number {
    if (!from) return 0xffffff;
    let color = this._colors.get(from);
    if (color == void 0) {
      color = COLORS[this._colors.size % COLORS.length];
      this._colors.set(from, color);
    }
    return color;
  }
  protected is_mine(from: string): boolean {
    return this.renderer.lfw.players.get(`${from}#1`)?.mine === true;
  }
  protected name_of(from: string): string {
    if (!from) {
      const me = current_connection.conn?.client;
      return me?.name || me?.id || "";
    }
    const clients = current_connection.conn?.room?.clients;
    if (clients) {
      for (let i = 0; i < clients.length; i++) {
        const client = clients[i];
        if (client.id === from) return client.name || from;
      }
    }
    return from;
  }
  protected get_view(from: string): ICursorView {
    let view = this._views.get(from);
    if (view) return view;
    const sprite = new Sprite(new SpriteMaterial({
      color: this.color_of(from),
      transparent: true,
      depthTest: false,
      depthWrite: false,
    }));
    sprite.renderOrder = 9999;
    sprite.visible = false;
    this.container.add(sprite);
    const name = this.name_of(from);
    view = { sprite, name, rx: 0, ry: 0, shown: false };
    this.apply_name(view, name);
    this._views.set(from, view);
    return view;
  }
  protected apply_name(view: ICursorView, name: string): void {
    const { canvas, w, h } = cursor_canvas(name);
    const material = view.sprite.material as SpriteMaterial;
    material.map?.dispose();
    material.map = new CanvasTexture(canvas);
    view.sprite.scale.set(w, h, 1);
    view.sprite.center.set(TIP_X / w, (h - TIP_Y) / h);
  }
  protected show_cursor(from: string, cursor: IPointerCursor, now: number): boolean {
    if (cursor.hidden) return false;
    if (!from) return !!this.renderer.lfw.ui && this.renderer.lfw.pointings.enabled;
    return now - cursor.t <= TIMEOUT && !this.is_mine(from);
  }
  update(dt: number): void {
    const cursors = get_pointer_cursors(this.renderer.world);
    const now = performance.now();
    for (const [from, view] of this._views) {
      const cursor = cursors?.get(from);
      const visible = !!cursor && this.show_cursor(from, cursor, now);
      view.sprite.visible = visible;
      if (!visible) view.shown = false;
    }
    if (!cursors) return;
    const ease = 1 - Math.exp(-dt / EASE_TAU);
    for (const [from, cursor] of cursors) {
      if (!this.show_cursor(from, cursor, now)) continue;
      const view = this.get_view(from);
      const name = this.name_of(from);
      if (view.name !== name) {
        view.name = name;
        this.apply_name(view, name);
      }
      const tx = cursor.x;
      const ty = -cursor.y;
      if (view.shown) {
        view.rx += (tx - view.rx) * ease;
        view.ry += (ty - view.ry) * ease;
      } else {
        view.rx = tx;
        view.ry = ty;
      }
      view.shown = true;
      view.sprite.visible = true;
      view.sprite.position.set(view.rx, view.ry, 0);
    }
  }
}
