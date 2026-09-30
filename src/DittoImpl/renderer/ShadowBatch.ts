import * as T from "../_t";
import type { EntityRenderer } from "./EntityRenderer";
import type { WorldRenderer } from "./WorldRenderer";

const MAX_SHADOWS = 256;

export class ShadowBatch {
  readonly world_renderer: WorldRenderer;
  protected readonly _mesh: T.InstancedMesh<T.BufferGeometry, T.MeshBasicMaterial>;
  protected readonly _opacity: T.InstancedBufferAttribute;
  protected readonly _material: T.MeshBasicMaterial;
  protected _source_geo: T.BufferGeometry | null = null;
  protected _clone_geo: T.BufferGeometry | null = null;
  protected _map: T.Texture | null = null;
  protected _count = 0;

  get count() { return this._count }

  constructor(world_renderer: WorldRenderer) {
    this.world_renderer = world_renderer;
    this._opacity = new T.InstancedBufferAttribute(new Float32Array(MAX_SHADOWS), 1);
    this._opacity.setUsage(T.DynamicDrawUsage);
    this._material = new T.MeshBasicMaterial({ transparent: true });
    this._material.onBeforeCompile = (shader) => {
      shader.vertexShader = shader.vertexShader
        .replace('#include <common>', '#include <common>\nattribute float iOpacity;\nvarying float vInstOpacity;')
        .replace('#include <begin_vertex>', '#include <begin_vertex>\nvInstOpacity = iOpacity;');
      shader.fragmentShader = shader.fragmentShader
        .replace('#include <common>', '#include <common>\nvarying float vInstOpacity;')
        .replace('vec4 diffuseColor = vec4( diffuse, opacity );', 'vec4 diffuseColor = vec4( diffuse, opacity * vInstOpacity );');
    };
    this._material.customProgramCacheKey = () => 'lf2_shadow_batch';
    this._mesh = new T.InstancedMesh(new T.BufferGeometry(), this._material, MAX_SHADOWS);
    this._mesh.instanceMatrix.setUsage(T.DynamicDrawUsage);
    this._mesh.frustumCulled = false;
    this._mesh.visible = false;
    this._mesh.name = 'ShadowBatch';
    world_renderer.world_node.add(this._mesh);
  }

  protected _use_source_geo(src: T.BufferGeometry): void {
    const clone = src.clone();
    clone.setAttribute('iOpacity', this._opacity);
    this._clone_geo?.dispose();
    this._clone_geo = clone;
    this._mesh.geometry = clone;
  }

  /** 收集本帧全部可见影子；返回是否有可绘制实例（影子个体 mesh 会被隐藏，由批量网格接手） */
  collect(groups: readonly EntityRenderer[][]): boolean {
    this._count = 0;
    let sample: T.Mesh<T.BufferGeometry, T.MeshBasicMaterial> | undefined;
    for (const g of groups) {
      for (const er of g) {
        const mesh = er.shad.mesh;
        if (mesh.visible && mesh.material.map) { sample = mesh; break; }
      }
      if (sample) break;
    }
    if (!sample) {
      this._mesh.visible = false;
      return false;
    }
    if (this._source_geo !== sample.geometry) {
      this._source_geo = sample.geometry;
      this._use_source_geo(sample.geometry);
    }
    if (this._map !== sample.material.map) {
      this._map = sample.material.map;
      this._material.map = this._map;
      this._material.needsUpdate = true;
    }
    const geo = this._source_geo;
    const map = this._map;
    const arr = this._mesh.instanceMatrix.array as Float32Array;
    const opac = this._opacity.array as Float32Array;
    for (const g of groups) {
      for (const er of g) {
        const mesh = er.shad.mesh;
        if (!mesh.visible) continue;
        if (mesh.geometry !== geo || mesh.material.map !== map) continue;
        if (this._count >= MAX_SHADOWS) continue;
        const i = this._count++;
        const o = i * 16;
        const { x, y, z } = mesh.position;
        arr[o] = mesh.scale.x; arr[o + 1] = 0; arr[o + 2] = 0; arr[o + 3] = 0;
        arr[o + 4] = 0; arr[o + 5] = mesh.scale.y; arr[o + 6] = 0; arr[o + 7] = 0;
        arr[o + 8] = 0; arr[o + 9] = 0; arr[o + 10] = mesh.scale.z; arr[o + 11] = 0;
        arr[o + 12] = x; arr[o + 13] = y; arr[o + 14] = z; arr[o + 15] = 1;
        opac[i] = mesh.material.opacity;
        mesh.visible = false;
      }
    }
    this._mesh.count = this._count;
    this._mesh.visible = this._count > 0;
    if (!this._count) return false;
    this._mesh.instanceMatrix.needsUpdate = true;
    this._opacity.needsUpdate = true;
    return true;
  }

  end(): void {
    this._mesh.visible = false;
  }

  dispose(): void {
    this._mesh.removeFromParent();
    this._clone_geo?.dispose();
    this._clone_geo = null;
    this._material.dispose();
    this._mesh.dispose();
  }
}
