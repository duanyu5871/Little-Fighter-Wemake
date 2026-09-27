import i18n from "./i18n";

/**
 * 桌面客户端（Electron）的更新进度提示
 *
 * 主进程那份状态原本只体现在托盘菜单、托盘悬浮提示与任务栏进度条上，
 * 不点开托盘 / 切出全屏游戏就看不见，所以这里把同一份状态画成页面右下角的 DOM 角标。
 *
 * 状态来源：`desktop/app/main.mjs` 的 `apply_update_state()` → preload 的 `window.lfwm_update`。
 * 非桌面客户端（浏览器 / 扩展 / B站 Toy）拿不到 `window.lfwm_update`，整个模块空转。
 */

const BOX_ID = "lfwm_update_box";
const STYLE_ID = "lfwm_update_style";

/** `state.notice` 的代码 → i18n key */
const NOTICE_KEYS: Record<string, string> = {
  up_to_date: "update_up_to_date",
};

/** 进度条：<0 隐藏，-1 不确定态（检查中），>=0 为百分比 */
const BAR_HIDDEN = -1;
const BAR_UNKNOWN = -2;

let state: IDesktopUpdateState = {
  enabled: false,
  phase: "idle",
  version: "",
  percent: 0,
  notice: "",
  error: "",
  current: "",
};
let box: HTMLDivElement | undefined;
let text_el: HTMLSpanElement | undefined;
let action_el: HTMLButtonElement | undefined;
let bar_el: HTMLDivElement | undefined;
let fill_el: HTMLDivElement | undefined;
let detail_el: HTMLDivElement | undefined;

/** 取文案并替换 `%1`/`%2`（与 i18n 里其它条目一致，用 %n 而非 {{}}） */
function t(key: string, ...args: (string | number)[]): string {
  const raw = i18n.t(key);
  let text = typeof raw === "string" && raw ? raw : key;
  args.forEach((value, i) => (text = text.replace(`%${i + 1}`, `${value}`)));
  return text;
}

function ensure_style() {
  if (document.getElementById(STYLE_ID)) return;
  const style = document.createElement("style");
  style.id = STYLE_ID;
  // 内联样式写不了 keyframes，检查阶段的循环进度条单独放这里
  style.textContent = "@keyframes lfwm_update_slide{0%{transform:translateX(-100%)}100%{transform:translateX(400%)}}";
  document.head.append(style);
}

function button(label: string): HTMLButtonElement {
  const el = document.createElement("button");
  el.textContent = label;
  el.style.cssText = [
    "font:inherit",
    "padding:4px 10px",
    "border:0",
    "border-radius:6px",
    "cursor:pointer",
    "color:#16161a",
    "background:#ffd24a",
  ].join(";");
  return el;
}

function ensure_box(): HTMLDivElement {
  if (box) return box;
  ensure_style();

  const root = document.createElement("div");
  root.id = BOX_ID;
  root.style.cssText = [
    "position:fixed",
    "right:16px",
    "bottom:16px",
    "z-index:9999",
    "display:none",
    "flex-direction:column",
    "gap:6px",
    "min-width:190px",
    "max-width:360px",
    "padding:10px 12px",
    "border-radius:10px",
    'background:rgba(18,18,22,.92)',
    "color:#eee",
    'font:13px/1.4 system-ui,-apple-system,"Microsoft YaHei",sans-serif',
    "box-shadow:0 4px 16px rgba(0,0,0,.45)",
    "user-select:none",
    "pointer-events:auto",
  ].join(";");

  const row = document.createElement("div");
  row.style.cssText = "display:flex;align-items:center;gap:10px";

  text_el = document.createElement("span");
  text_el.style.cssText = "flex:1;white-space:pre-wrap";

  action_el = button("");
  action_el.style.display = "none";
  action_el.onclick = () => window.lfwm_update?.install?.();

  row.append(text_el, action_el);

  bar_el = document.createElement("div");
  bar_el.style.cssText = "overflow:hidden;height:4px;border-radius:2px;background:rgba(255,255,255,.16)";
  fill_el = document.createElement("div");
  fill_el.style.cssText = "width:0%;height:100%;border-radius:2px;background:#ffd24a;transition:width 200ms";
  bar_el.append(fill_el);

  detail_el = document.createElement("div");
  detail_el.style.cssText = "display:none;color:#ff9a9a;font-size:12px;overflow-wrap:anywhere";

  root.append(row, bar_el, detail_el);
  document.body.append(root);
  box = root;
  return root;
}

function render() {
  if (!window.lfwm_update) return;
  const { phase, version, percent, notice, error } = state;
  const visible = phase !== "idle" || !!notice || !!error;
  if (!visible) {
    if (box) box.style.display = "none";
    return;
  }

  const root = ensure_box();
  root.style.display = "flex";

  let text = "";
  let progress = BAR_HIDDEN;
  let label = "";
  if (phase === "checking") {
    text = t("update_checking");
    progress = BAR_UNKNOWN;
  } else if (phase === "downloading") {
    text = t("update_downloading", version, percent);
    progress = percent;
  } else if (phase === "ready") {
    text = t("update_ready", version);
    progress = 100;
    label = t("update_restart");
  } else if (notice && NOTICE_KEYS[notice]) {
    text = t(NOTICE_KEYS[notice]);
  } else if (error) {
    text = t("update_failed");
  }

  text_el!.textContent = text;

  action_el!.style.display = label ? "" : "none";
  if (label) action_el!.textContent = label;

  bar_el!.style.display = progress === BAR_HIDDEN ? "none" : "block";
  if (progress === BAR_UNKNOWN) {
    fill_el!.style.width = "25%";
    fill_el!.style.animation = "lfwm_update_slide 1.2s linear infinite";
  } else if (progress >= 0) {
    fill_el!.style.animation = "";
    fill_el!.style.width = `${Math.max(0, Math.min(100, progress))}%`;
  }

  detail_el!.style.display = error ? "block" : "none";
  if (error) detail_el!.textContent = error;
}

function apply_state(next: IDesktopUpdateState | undefined) {
  if (!next || typeof next !== "object") return;
  state = next;
  render();
}

/**
 * 订阅桌面客户端广播的更新状态（非桌面客户端直接返回）。
 *
 * 在主进程广播之前页面可能已经加载完，所以额外主动拉一次当前状态。
 */
export function install_desktop_update() {
  const api = window.lfwm_update;
  if (!api) return;
  api.on_state(apply_state);
  i18n.on("languageChanged", () => render());
  api.state().then(apply_state).catch((e) => console.warn("[desktop_update] 读取更新状态失败", e));
}
