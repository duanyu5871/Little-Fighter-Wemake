import img_btn_0_4 from "@/assets/btn_0_4.png";
import img_btn_1_4 from "@/assets/btn_1_4.png";
import img_btn_2_4 from "@/assets/btn_2_4.png";
import img_btn_3_3 from "@/assets/btn_3_3.png";
import { ToggleImgButton } from "@/Component/Buttons/ToggleImgButton";
import { useLFW } from "@/hooks/useLFW";
import * as Paths from "@/Paths";
import { useEffect, useState, type ComponentType } from "react";
import { useLocation, useNavigate } from "react-router";
import { BackgroundPreviewer } from "./BackgroundPreviewer";
import { PreviewerContext, type TCanvasFit } from "./ctx";
import { EntityPreviewer } from "./EntityPreviewer";
import { ImagePreviewer } from "./ImagePreviewer";
import csses from "./styles.module.scss";

interface ITab {
  id: string;
  label: string;
  Component: ComponentType;
}

/** 新增预览类型时，写一个 `xxxPreviewer` 组件再往这里加一项即可 */
const TABS: readonly ITab[] = [
  { id: "bg", label: "背景", Component: BackgroundPreviewer },
  { id: "entity", label: "物体", Component: EntityPreviewer },
  { id: "image", label: "图片", Component: ImagePreviewer },
];

export default function PreviewerPage() {
  const nav = useNavigate();
  const l = useLocation();
  const [progress, set_progress] = useState("");
  const { lfw, ready, error } = useLFW({
    muted: true,
    hide_ui: true,
    on_progress: (content, value) => set_progress(`${content} ${Math.round(value)}%`),
  });
  const [tab_id, set_tab_id] = useState(TABS[0].id);
  /** 画布显示模式（object-fit），背景 / 物体两个 tab 共用 */
  const [fit, set_fit] = useState<TCanvasFit>("contain");
  const tab = TABS.find((v) => v.id === tab_id) ?? TABS[0];

  // 桌面客户端（无边框窗口）：窗口控制按钮自己画，样式与游戏页右上角那套一致
  const is_desktop = !!window.runtime?.WindowMinimise;
  const [is_maximised, set_is_maximised] = useState(false);
  useEffect(() => {
    const q = window.runtime?.WindowIsMaximised?.();
    if (q) void q.then((v) => set_is_maximised(!!v));
  }, [is_desktop]);
  const toggle_maximise = async () => {
    const q = window.runtime?.WindowIsMaximised?.();
    const m = q ? await q : false;
    set_is_maximised(!m);
    window.runtime?.WindowToggleMaximise?.();
  };

  return (
    <PreviewerContext.Provider value={{ lfw, ready, fit, set_fit }}>
      <div className={csses.page}>
        {/* 顶部一律靠左：标签页 │ 刷新 ┃ 返回游戏 ┃ 加载进度；右侧只放窗口三件套（桌面端） */}
        <div className={csses.header}>
          <div className={csses.tabs}>
            {TABS.map((v) => (
              <button
                key={v.id}
                className={`${csses.tab}${v.id === tab.id ? " " + csses.tab_active : ""}`}
                onClick={() => set_tab_id(v.id)}
              >
                {v.label}
              </button>
            ))}
          </div>
          <div className={csses.divider} />
          <button className={csses.btn} onClick={() => window.location.reload()}>刷新</button>
          {/* 查询串原样带回去：游戏页靠 URL 里的 DANMU_WS 建弹幕桥连接（见 src/danmu_bridge.ts） */}
          <button
            className={csses.btn}
            onClick={() => nav({ pathname: Paths.Paths.game, search: l.search }, { replace: true })}
          >
            返回游戏
          </button>
          <div className={csses.muted}>{progress}</div>
          <div className={csses.spacer} />
          {is_desktop && (
            <div className={csses.window_btns}>
              <ToggleImgButton
                onClick={() => window.runtime?.WindowMinimise?.()}
                src={[img_btn_0_4]} />
              <ToggleImgButton
                checked={is_maximised}
                onClick={() => void toggle_maximise()}
                src={[img_btn_1_4, img_btn_2_4]} />
              <ToggleImgButton
                onClick={() => window.runtime?.Quit?.()}
                src={[img_btn_3_3]} />
            </div>
          )}
        </div>
        <div className={csses.body}>
          {error ? (
            <div className={csses.body_msg}>{error}</div>
          ) : ready ? (
            <tab.Component />
          ) : (
            <div className={csses.body_msg}>数据包加载中…（首次约 30MB）</div>
          )}
        </div>
      </div>
    </PreviewerContext.Provider>
  );
}
