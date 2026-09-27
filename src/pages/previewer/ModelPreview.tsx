import { useEffect, useRef, useState } from "react";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import * as T from "@/DittoImpl/_t";
import { ZipGLTFLoader } from "@/DittoImpl/renderer/GLTFZipLoader";
import { ModelCache } from "@/DittoImpl/renderer/ModelCache";
import type { LFW } from "@/LFW";
import csses from "./styles.module.scss";

/**
 * glTF / GLB 的三维预览：自动摆好相机与光照，拖动旋转、滚轮缩放。
 *
 * - 外部贴图 / .bin 由 `ZipGLTFLoader` 从数据包里解析（不只认内嵌资源）；
 * - 不复用 `ModelCache`：那是挂在游戏世界里的共享对象，预览器改它的变换会影响游戏。
 *
 * 换文件时由调用方给 `key`（重新挂载，省得在 effect 里同步清状态）。
 */
export function ModelPreview({ lfw, path }: { lfw: LFW; path: string }) {
  const ref_canvas = useRef<HTMLCanvasElement>(null);
  const [ready, set_ready] = useState(false);
  const [error, set_error] = useState<string>();

  useEffect(() => {
    const canvas = ref_canvas.current;
    if (!canvas) return;
    let disposed = false;
    let raf = 0;

    const renderer = new T.WebGLRenderer({ canvas, antialias: true, alpha: true });
    renderer.setPixelRatio(Math.min(2, window.devicePixelRatio || 1));
    const scene = new T.Scene();
    const camera = new T.PerspectiveCamera(45, 1, 0.01, 10000);
    const controls = new OrbitControls(camera, canvas);
    controls.enableDamping = true;
    scene.add(new T.HemisphereLight(0xffffff, 0x445566, 2.2));
    const key_light = new T.DirectionalLight(0xffffff, 1.6);
    key_light.position.set(1, 2, 3);
    scene.add(key_light);

    const resize = () => {
      const w = Math.max(1, canvas.clientWidth);
      const h = Math.max(1, canvas.clientHeight);
      renderer.setSize(w, h, false);
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
    };
    const observer = new ResizeObserver(resize);
    observer.observe(canvas);
    resize();

    const loop = () => {
      if (disposed) return;
      controls.update();
      renderer.render(scene, camera);
      raf = requestAnimationFrame(loop);
    };
    loop();

    void lfw.resources.import_array_buffer(path, true)
      .then(({ data }) => new ZipGLTFLoader(lfw).parse_package(data, ModelCache.glb_dir_of(path)))
      .then((gltf) => {
        if (disposed) return;
        scene.add(gltf.scene);
        // 按包围盒摆相机：拉远到刚好装下
        const box = new T.Box3().setFromObject(gltf.scene);
        const size = box.getSize(new T.Vector3());
        const center = box.getCenter(new T.Vector3());
        const radius = Math.max(size.x, size.y, size.z) / 2 || 1;
        const dist = radius / Math.tan((camera.fov * Math.PI) / 360) * 1.7;
        camera.position.set(center.x + dist * 0.6, center.y + dist * 0.45, center.z + dist);
        camera.lookAt(center);
        controls.target.copy(center);
        controls.update();
        set_ready(true);
      })
      .catch((e: unknown) => {
        if (!disposed) set_error(`模型加载失败：${e instanceof Error ? e.message : e}`);
      });

    return () => {
      disposed = true;
      cancelAnimationFrame(raf);
      observer.disconnect();
      controls.dispose();
      // 释放这次解析出来的 GPU 资源（场景是新建的，不碰共享缓存）
      scene.traverse((obj) => {
        const mesh = obj as T.Mesh;
        mesh.geometry?.dispose?.();
        const materials = Array.isArray(mesh.material) ? mesh.material : mesh.material ? [mesh.material] : [];
        for (const material of materials) {
          for (const value of Object.values(material as unknown as Record<string, unknown>)) {
            const texture = value as { isTexture?: boolean; dispose?(): void };
            if (texture?.isTexture) texture.dispose?.();
          }
          material.dispose();
        }
      });
      renderer.dispose();
    };
  }, [lfw, path]);

  return (
    <>
      <canvas ref={ref_canvas} className={csses.model_canvas} />
      {error && <div className={csses.center_text}>{error}</div>}
      {!ready && !error && <div className={csses.center_text}>模型解析中…</div>}
    </>
  );
}
