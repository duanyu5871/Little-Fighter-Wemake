import { usePreviewer, type TCanvasFit } from "./ctx";
import csses from "./styles.module.scss";

const MODES: readonly { id: TCanvasFit; label: string; hint: string }[] = [
  { id: "contain", label: "适应", hint: "保持比例完整显示，可能留黑边" },
  { id: "cover", label: "填充", hint: "保持比例铺满，超出部分裁掉" },
  { id: "fill", label: "拉伸", hint: "铺满，不保持比例（会变形）" },
  { id: "none", label: "原始", hint: "按画布自身像素显示，不缩放" },
];

/** 画布显示模式（css `object-fit`）：背景 / 物体两个 tab 的工具栏共用，状态在页面级的 PreviewerContext 里 */
export function FitButtons() {
  const { fit, set_fit } = usePreviewer();
  return (
    <div className={csses.fit_group}>
      {MODES.map((v) => (
        <button
          key={v.id}
          className={`${csses.fit_btn}${v.id === fit ? " " + csses.fit_btn_active : ""}`}
          title={`${v.label}：${v.hint}`}
          onClick={() => set_fit(v.id)}
        >
          {v.label}
        </button>
      ))}
    </div>
  );
}
