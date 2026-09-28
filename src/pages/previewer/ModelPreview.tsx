import { useEffect, useRef, useState, type KeyboardEvent as ReactKeyboardEvent } from "react";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import * as T from "@/DittoImpl/_t";
import { ZipGLTFLoader } from "@/DittoImpl/renderer/GLTFZipLoader";
import { ModelCache } from "@/DittoImpl/renderer/ModelCache";
import type { LFW } from "@/LFW";
import csses from "./styles.module.scss";

/** 透视相机的视场角（正交/透视换算共用） */
const FOV = 45;

/** 标准视图：前/后/左/右/上/下，user = 回到默认的三维视角 */
export type TModelView = "front" | "back" | "left" | "right" | "top" | "bottom" | "user";

/** 各标准视图的机位方向与相机 up（上/下视图得换个 up，否则 lookAt 会退化） */
const MODEL_VIEWS: Record<Exclude<TModelView, "user">, { dir: readonly [number, number, number]; up: readonly [number, number, number] }> = {
  front: { dir: [0, 0, 1], up: [0, 1, 0] },
  back: { dir: [0, 0, -1], up: [0, 1, 0] },
  right: { dir: [1, 0, 0], up: [0, 1, 0] },
  left: { dir: [-1, 0, 0], up: [0, 1, 0] },
  top: { dir: [0, 1, 0], up: [0, 0, -1] },
  bottom: { dir: [0, -1, 0], up: [0, 0, 1] },
};

/** 默认的三维视角方向（和初次摆相机一致） */
const USER_DIR = new T.Vector3(0.6, 0.45, 1).normalize();
const Y_UP = new T.Vector3(0, 1, 0);

/** 快捷键（参考 blender 小键盘；Ctrl 取反向）：1 前 / 3 右 / 7 上 / 0 复位 */
const KEY_VIEWS: Record<string, { view: TModelView; ctrl?: TModelView }> = {
  Digit1: { view: "front", ctrl: "back" },
  Numpad1: { view: "front", ctrl: "back" },
  Digit3: { view: "right", ctrl: "left" },
  Numpad3: { view: "right", ctrl: "left" },
  Digit7: { view: "top", ctrl: "bottom" },
  Numpad7: { view: "top", ctrl: "bottom" },
  Digit0: { view: "user" },
  Numpad0: { view: "user" },
};

/** 一次视图切换请求（n 用自增序号，同一个视图连点也能重新摆位） */
export interface IModelViewRequest {
  name: TModelView;
  n: number;
}

/** 三根轴的方向与颜色（X 红 / Y 绿 / Z 蓝） */
const AXIS_DEFS: readonly [number, number, number, number][] = [
  [1, 0, 0, 0xff0000],
  [0, 1, 0, 0x00ff00],
  [0, 0, 1, 0x0000ff],
];

/**
 * 世界坐标轴：单位长度、正负方向各一根（负方向淡一些），
 * 长度靠整体缩放每帧重算，所以看上去是无限延伸的。
 */
function build_axes(): T.Group {
  const group = new T.Group();
  group.name = "world_axes";
  for (const [x, y, z, color] of AXIS_DEFS) {
    for (const sign of [1, -1]) {
      const geo = new T.BufferGeometry();
      geo.setAttribute("position", new T.BufferAttribute(
        new Float32Array([0, 0, 0, x * sign, y * sign, z * sign]),
        3,
      ));
      group.add(new T.LineSegments(geo, new T.LineBasicMaterial({
        color,
        transparent: sign < 0,
        opacity: sign < 0 ? 0.28 : 1,
      })));
    }
  }
  return group;
}

/** 场景里跨 effect 共用的东西：切投影时相机与控制器整体换掉，别的都留着 */
interface IView {
  renderer: T.WebGLRenderer;
  scene: T.Scene;
  camera: T.Camera;
  controls?: OrbitControls;
  /** 当前相机是不是正交（`Camera` 类型上没判别字段，自己记一笔） */
  is_ortho: boolean;
  /** 模型包围盒的中心 / 半径 */
  center: T.Vector3;
  radius: number;
  /** 正交相机 zoom=1 时的半高 */
  half_h: number;
  /** 初始机位到中心的距离：正交(zoom)与透视(距离)互相换算的基准 */
  init_dist: number;
  axes: T.Group;
  /** 当前相机的 up（上/下视图会换掉） */
  up: T.Vector3;
  /** 已经摆过的视图请求序号，避免别的重渲染把它又摆一次 */
  applied_req: number;
}

interface IPlay {
  mixer?: T.AnimationMixer;
  actions?: Map<string, T.AnimationAction>;
  action?: T.AnimationAction;
  playing: boolean;
  loop: boolean;
}

/**
 * glTF / GLB 的三维预览：自动摆好相机与光照，拖动旋转、滚轮缩放。
 *
 * - 默认正交投影（可切透视）；世界原点画坐标轴，长度随模型大小；
 * - 外部贴图 / .bin 由 `ZipGLTFLoader` 从数据包里解析（不只认内嵌资源）；
 * - 不复用 `ModelCache`：那是挂在游戏世界里的共享对象，预览器改它的变换会影响游戏。
 *
 * 换文件时由调用方给 `key`（重新挂载，省得在 effect 里同步清状态）。
 */
export function ModelPreview({ lfw, path, ortho, axes, view, on_view }: {
  lfw: LFW;
  path: string;
  /** true = 正交投影 */
  ortho: boolean;
  /** 显示世界坐标轴 */
  axes: boolean;
  /** 视图切换请求 */
  view?: IModelViewRequest;
  /** 快捷键把请求交回调用方（由它自增序号） */
  on_view?: (name: TModelView) => void;
}) {
  const ref_canvas = useRef<HTMLCanvasElement>(null);
  const ref_view = useRef<IView | undefined>(undefined);
  const ref_play = useRef<IPlay>({ playing: true, loop: true });
  const [ready, set_ready] = useState(false);
  const [error, set_error] = useState<string>();
  const [clips, set_clips] = useState<readonly string[]>([]);
  const [clip_name, set_clip_name] = useState("");
  const [playing, set_playing] = useState(true);
  const [loop, set_loop] = useState(true);
  const [speed, set_speed] = useState(1);
  const [time, set_time] = useState(0);
  const [duration, set_duration] = useState(0);

  // 渲染器 / 场景 / 光照 / 坐标轴 / 模型：只在换文件时重建
  useEffect(() => {
    const canvas = ref_canvas.current;
    if (!canvas) return;
    let disposed = false;
    let raf = 0;

    const renderer = new T.WebGLRenderer({ canvas, antialias: true, alpha: true });
    renderer.setPixelRatio(Math.min(2, window.devicePixelRatio || 1));
    const scene = new T.Scene();
    scene.add(new T.HemisphereLight(0xffffff, 0x445566, 2.2));
    const key_light = new T.DirectionalLight(0xffffff, 1.6);
    key_light.position.set(1, 2, 3);
    scene.add(key_light);

    // 世界坐标轴（X 红 / Y 绿 / Z 蓝）：长度每帧按视口重算，看着是无限延伸的
    const axes_group = build_axes();
    scene.add(axes_group);

    const view: IView = {
      renderer,
      scene,
      axes: axes_group,
      // 占位相机：真正的相机由下面的 effect 按投影类型创建，这里先保证循环有东西可用
      camera: new T.PerspectiveCamera(FOV, 1, 0.01, 10000),
      is_ortho: false,
      center: new T.Vector3(),
      radius: 1,
      half_h: 1,
      init_dist: 3,
      up: Y_UP.clone(),
      applied_req: 0,
    };
    ref_view.current = view;

    const resize = () => {
      const w = Math.max(1, canvas.clientWidth);
      const h = Math.max(1, canvas.clientHeight);
      renderer.setSize(w, h, false);
      const aspect = w / h;
      if (view.is_ortho) {
        // 正交：视锥是按画面比例算的，宽高都得跟着窗口走
        const cam = view.camera as T.OrthographicCamera;
        cam.left = -view.half_h * aspect;
        cam.right = view.half_h * aspect;
        cam.top = view.half_h;
        cam.bottom = -view.half_h;
        cam.updateProjectionMatrix();
      } else {
        const cam = view.camera as T.PerspectiveCamera;
        cam.aspect = aspect;
        cam.updateProjectionMatrix();
      }
    };
    const observer = new ResizeObserver(resize);
    observer.observe(canvas);
    resize();

    /** 把坐标轴拉到比视口还大 -> 看上去就是无限长（相机拉近/拉远都跟得上） */
    const fit_axes = () => {
      const zoom = (view.camera as T.OrthographicCamera | T.PerspectiveCamera).zoom || 1;
      // target 处的可视半高：正交看 half_h，透视看距离×tan(fov/2)
      const half_v = view.is_ortho
        ? view.half_h / zoom
        : view.camera.position.distanceTo(view.controls?.target ?? view.center)
          * Math.tan((FOV * Math.PI) / 360) / zoom;
      axes_group.scale.setScalar(Math.max(view.radius * 3, half_v * 4));
    };

    let last_ms = performance.now();
    let last_shown = -1;
    const loop = () => {
      if (disposed) return;
      const now = performance.now();
      const dt = Math.min(0.1, (now - last_ms) / 1000);
      last_ms = now;
      const play = ref_play.current;
      const action = play.action;
      if (play.mixer && action && play.playing) {
        play.mixer.update(dt);
        const dur = action.getClip().duration;
        const shown = play.loop ? action.time % dur : Math.min(action.time, dur);
        if (Math.abs(shown - last_shown) > 0.04) {
          last_shown = shown;
          set_time(shown);
        }
      }
      view.controls?.update();
      fit_axes();
      view.renderer.render(scene, view.camera);
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
        view.center = box.getCenter(new T.Vector3());
        view.radius = Math.max(size.x, size.y, size.z) / 2 || 1;
        view.init_dist = view.radius / Math.tan((FOV * Math.PI) / 360) * 1.7;
        view.half_h = view.init_dist * Math.tan((FOV * Math.PI) / 360);
        const mixer = new T.AnimationMixer(gltf.scene);
        const actions = new Map<string, T.AnimationAction>();
        for (const clip of gltf.animations) actions.set(clip.name, mixer.clipAction(clip));
        ref_play.current.mixer = mixer;
        ref_play.current.actions = actions;
        ref_play.current.action = void 0;
        set_clips(gltf.animations.map((v) => v.name));
        set_clip_name(gltf.animations[0]?.name ?? "");
        set_duration(gltf.animations[0]?.duration ?? 0);
        set_time(0);
        set_ready(true);
      })
      .catch((e: unknown) => {
        if (!disposed) set_error(`模型加载失败：${e instanceof Error ? e.message : e}`);
      });

    return () => {
      disposed = true;
      cancelAnimationFrame(raf);
      observer.disconnect();
      view.controls?.dispose();
      ref_view.current = void 0;
      ref_play.current.mixer?.stopAllAction();
      ref_play.current.mixer = void 0;
      ref_play.current.actions = void 0;
      ref_play.current.action = void 0;
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

  // 相机 + 控制器：切投影/切视图时重建，机位朝向沿用，并且「看起来一样大」
  useEffect(() => {
    const v = ref_view.current;
    const canvas = ref_canvas.current;
    if (!ready || !v || !canvas) return;

    const { center, init_dist } = v;
    const aspect = Math.max(1, canvas.clientWidth) / Math.max(1, canvas.clientHeight);
    const prev = v.controls;
    const target = prev ? prev.target.clone() : center.clone();
    const dir = new T.Vector3();
    let dist = init_dist;
    let zoom = 1;
    if (prev) {
      const offset = new T.Vector3().subVectors(v.camera.position, target);
      const len = offset.length();
      if (len > 1e-6) dir.copy(offset).divideScalar(len);
      // 透视靠距离决定大小、正交靠 zoom 决定大小，换算成等效距离再按新投影还原
      const prev_zoom = (v.camera as T.OrthographicCamera | T.PerspectiveCamera).zoom || 1;
      const vis_dist = v.is_ortho ? init_dist / prev_zoom : len;
      if (ortho) zoom = init_dist / Math.max(vis_dist, 1e-6);
      else dist = vis_dist;
    } else {
      dir.copy(USER_DIR);
    }

    // 标准视图：只换朝向与 up，距离/缩放沿用当前手感
    if (view && view.n !== v.applied_req) {
      v.applied_req = view.n;
      if (view.name === "user") {
        dir.copy(USER_DIR);
        v.up.copy(Y_UP);
        target.copy(center);
        dist = init_dist;
        zoom = 1;
      } else {
        const it = MODEL_VIEWS[view.name];
        dir.set(it.dir[0], it.dir[1], it.dir[2]);
        v.up.set(it.up[0], it.up[1], it.up[2]);
      }
    }

    const camera = ortho
      ? new T.OrthographicCamera(-v.half_h * aspect, v.half_h * aspect, v.half_h, -v.half_h, 0.01, init_dist * 100)
      : new T.PerspectiveCamera(FOV, aspect, init_dist * 0.001, init_dist * 100);
    camera.up.copy(v.up);
    camera.position.copy(target).addScaledVector(dir, dist);
    camera.zoom = zoom;
    camera.updateProjectionMatrix();
    camera.lookAt(target);

    // OrbitControls 在构造时就记下了相机 up，所以换 up 必须重建它
    const controls = new OrbitControls(camera, canvas);
    controls.enableDamping = true;
    controls.target.copy(target);
    controls.update();

    v.camera = camera;
    v.controls = controls;
    v.is_ortho = ortho;

    return () => {
      controls.dispose();
      if (v.controls === controls) v.controls = void 0;
    };
  }, [ready, ortho, view]);

  // 坐标轴开关：只改 helper 的可见性，不动场景
  useEffect(() => {
    const v = ref_view.current;
    if (v) v.axes.visible = axes;
  }, [axes, ready]);

  useEffect(() => {
    const play = ref_play.current;
    play.playing = playing;
    play.loop = loop;
    const action = play.action;
    if (!action) return;
    action.setLoop(loop ? T.LoopRepeat : T.LoopOnce, loop ? Infinity : 1);
    action.clampWhenFinished = !loop;
    action.timeScale = speed;
  }, [playing, loop, speed, clips]);

  useEffect(() => {
    const play = ref_play.current;
    const action = play.actions?.get(clip_name);
    if (!action) return;
    if (play.actions) for (const other of play.actions.values()) other.stop();
    play.action = action;
    action.reset();
    action.setLoop(loop ? T.LoopRepeat : T.LoopOnce, loop ? Infinity : 1);
    action.clampWhenFinished = !loop;
    action.timeScale = speed;
    action.play();
    set_duration(action.getClip().duration);
    set_time(0);
  }, [clip_name, clips]);

  const seek = (v: number) => {
    const play = ref_play.current;
    const action = play.action;
    if (!action) return;
    action.time = v;
    play.mixer?.update(0);
    set_time(v);
  };

  const restart = () => {
    ref_play.current.action?.reset().play();
    set_time(0);
  };

  const resume_if_finished = () => {
    const action = ref_play.current.action;
    if (!action?.paused) return;
    if (action.time >= action.getClip().duration) action.reset().play();
    else action.paused = false;
  };

  const toggle_playing = () => {
    if (!playing) resume_if_finished();
    set_playing(!playing);
  };

  const toggle_loop = () => {
    if (!loop) resume_if_finished();
    set_loop(!loop);
  };

  // 快捷键：点一下画面（canvas 拿到焦点）后，数字键切视图
  const on_key_down = (e: ReactKeyboardEvent<HTMLCanvasElement>) => {
    const it = KEY_VIEWS[e.code];
    if (!it) return;
    e.preventDefault();
    on_view?.((e.ctrlKey && it.ctrl) ? it.ctrl : it.view);
  };

  return (
    <>
      <canvas
        ref={ref_canvas}
        className={csses.model_canvas}
        tabIndex={0}
        title="点一下画面后可用数字键切视图：1 前 / 3 右 / 7 上，按住 Ctrl 取反（后/左/下），0 复位"
        onKeyDown={on_key_down}
      />
      {error && <div className={csses.center_text}>{error}</div>}
      {!ready && !error && <div className={csses.center_text}>模型解析中…</div>}
      {ready && clips.length > 0 && (
        <div className={csses.model_anim}>
          <button onClick={toggle_playing}>{playing ? "暂停" : "播放"}</button>
          <button onClick={restart}>重播</button>
          <button onClick={toggle_loop} title="循环播放 / 播完停在最后一帧">
            {loop ? "循环播放" : "停留最后一帧"}
          </button>
          <select value={clip_name} onChange={(e) => set_clip_name(e.target.value)}>
            {clips.map((name) => <option key={name} value={name}>{name}</option>)}
          </select>
          <input
            type="range"
            min={0}
            max={Math.max(duration, 0.001)}
            step={0.001}
            value={Math.min(time, duration)}
            onChange={(e) => seek(Number(e.target.value))}
          />
          <span className={csses.model_time}>{time.toFixed(2)} / {duration.toFixed(2)}s</span>
          <span className={csses.model_time}>
            倍速
            <input
              type="number"
              step={0.1}
              min={0.1}
              max={4}
              value={speed}
              onChange={(e) => set_speed(Math.max(0.1, Number(e.target.value) || 1))}
            />
          </span>
          <span className={csses.muted}>{clips.length} 个片段</span>
        </div>
      )}
    </>
  );
}
