
import { clamp, Ditto, LFW, min, World, type Entity } from "@/LFW";
import { BufferGeometry, Mesh, MeshBasicMaterial, Texture, Vector3 } from "../_t";
import type { EntityRenderer } from "./EntityRenderer";
import { get_static_plane_geometry } from "./GeometryKeeper";
import type { WorldRenderer } from "./WorldRenderer";
import { MaterialFactory, MaterialKind } from "./factory/MaterialFactory";

export class EntityShadowRender {
  readonly owner: EntityRenderer;
  readonly mesh: Mesh<BufferGeometry, MeshBasicMaterial>;
  readonly world: World;
  readonly lfw: LFW;
  readonly entity: Entity;
  readonly world_renderer: WorldRenderer;
  protected _w: number | undefined;
  protected _h: number | undefined;
  protected _img: string | undefined;
  private _mat: MeshBasicMaterial | null = null;
  private _tex: Texture | null = null;
  private _load_attempted: string | null = null;
  private _p0 = new Vector3()
  private _p1 = new Vector3()
  private _s0 = new Vector3(1, 1, 1)
  private _s1 = new Vector3(1, 1, 1)
  private _o0: number = 0
  private _o1: number = 0

  get bg() { return this.world.bg }
  get visible() { return this.mesh.visible; }
  set visible(v) { this.mesh.visible = v; }
  constructor(owner: EntityRenderer) {
    this.lfw = owner.lfw;
    this.world = owner.world;
    this.owner = owner;
    this.entity = owner.entity;
    this.world_renderer = owner.owner;
    const { world } = owner.entity;
    const { base } = world.bg.data
    const { shadow, shadow_w, shadow_h } = base;
    this._w = shadow_w;
    this._h = shadow_h;
    this._img = shadow;
    this.mesh = new Mesh(
      get_static_plane_geometry(
        shadow_w || 0,
        shadow_h || 0
      ),
      this.material(),
    );
    this.mesh.visible = false;
    this.mesh.name = EntityShadowRender.name;
    this.mesh.renderOrder = 0;
  }
  on_mount() {
    this.owner.body.add(this.mesh);
    this.update_position(true);
    this.update_scale_opacity(true);
  }
  on_unmount() {
    this.mesh.removeFromParent();
  }
  update_position(immidiate: boolean = false): void {
    const { bg, entity } = this
    const { position: { x, z }, ground_y } = entity;
    this._p0.copy(this._p1);
    this._p1.x = x
    this._p1.y = ground_y - z / 2
    this._p1.z = min(bg.far - 1, z)
    if (immidiate) {
      this._p0.copy(this._p1);
      this.mesh.position.copy(this._p1);
    }
  }
  update_scale_opacity(immidiate: boolean = false): void {
    const { position: { y }, ground_y } = this.entity;
    this._s0.copy(this._s1)
    this._s1.x = this._s1.y = 0.5 + 0.5 * clamp(250 - (y - ground_y), 0, 250) / 250
    this._o0 = this._o1;
    this._o1 = 0.3 + 0.7 * clamp(250 - (y - ground_y), 0, 250) / 250
    if (immidiate) {
      this._s0.copy(this._s1)
      this._o0 = this._o1;
      this.mesh.scale.copy(this._s1)
      this.mesh.material.opacity = this._o1;
    }
  }
  material(): MeshBasicMaterial {
    return (this._mat ??= MaterialFactory.get(MaterialKind.Basic, MeshBasicMaterial));
  }
  resolve_texture(): void {
    const { shadow } = this.bg.data.base;
    if (!shadow) return;
    const info = this.lfw.images.find(shadow);
    const tex = info?.pic?.texture ?? null;
    if (tex) {
      this._tex = tex;
      return;
    }
    if (this._tex || this._load_attempted === shadow) return;
    this._load_attempted = shadow;
    this.lfw.images.load_img(shadow, shadow).then((loaded) => {
      if (loaded?.pic?.texture) this._tex = loaded.pic.texture;
    }).catch((e) => {
      Ditto.warn('[EntityShadowRender::resolve_texture]', e);
    });
  }
  sync_material(): void {
    const mat = this.material();
    if (mat.map === this._tex) return;
    const has_map = !!mat.map;
    mat.map = this._tex;
    if (has_map !== !!this._tex) mat.needsUpdate = true;
  }
  render() {
    const { entity } = this;
    const dirty = this.owner.owner.dirty;
    if (dirty) {
      const { bg } = this;
      const { shadow_w, shadow_h, shadow } = bg.data.base;
      if (shadow_w !== this._w || shadow_h !== this._h) {
        this.mesh.geometry = get_static_plane_geometry(
          (this._w = shadow_w) || 0,
          (this._h = shadow_h) || 0
        );
      }
      if (shadow !== this._img) {
        this._img = shadow;
        this._tex = null;
        this._load_attempted = null;
      }
      this.update_position();
      this.update_scale_opacity();
    }
    if (dirty || !this._tex) this.resolve_texture();
    this.sync_material();
    this.mesh.visible = !!this._tex && !this.owner.invisible && !entity.frame.no_shadow;

    const f = this.world_renderer.dfactor;
    this.mesh.position.lerpVectors(this._p0, this._p1, f);
    this.mesh.scale.lerpVectors(this._s0, this._s1, f);
    this.mesh.material.opacity = this._o0 + (this._o1 - this._o0) * f;
  }
}
