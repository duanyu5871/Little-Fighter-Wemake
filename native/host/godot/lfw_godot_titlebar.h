// 自绘标题栏（本文件**不碰任何引擎类型**，DESIGN §G5）：Electron 端那份顶栏在引擎侧的等价几何。
//
// 对位的是 `src/App.tsx:788-811` 的三个按钮（minimize / toggle-maximize / quit）+ `src/App.module.scss`
// 的 `-webkit-app-region: drag` 拖拽层，以及 `desktop/app/main.mjs:915` 的 `frame: false`。那边按钮是
// 32×32 的图缩到 0.6（≈19 逻辑像素）贴右；这里取整成 20。
//
// 坐标：**逻辑像素**（左上原点、794×450 那套 —— 与 `_draw()` / UI 命中测试同一套）。引擎把 OS 的
// 窗口像素按 `Window::get_final_transform()` 反算回逻辑像素后才交给宿主（`lfw_godot_node.cpp` 的
// 「窗口」一节），所以这一层永远按逻辑像素判；只有「边缘缩放带」是窗口尺度的东西（`titlebar_edge_at`，
// 坐标是窗口像素）。
#pragma once

namespace lfw::host {

// 顶栏上的三个动作（与 Electron 的三个按钮一一对应）。
enum class TitleAction { none, minimize, maximize, close };
// 窗口边缘（`DisplayServer::window_start_resize` 要的那 8 个方向）。
enum class TitleEdge {
  none,
  top_left,
  top,
  top_right,
  left,
  right,
  bottom_left,
  bottom,
  bottom_right
};

struct TitleBarRect {
  double x = 0;
  double y = 0;
  double w = 0;
  double h = 0;
};

struct TitleBarButton {
  TitleAction action = TitleAction::none;
  TitleBarRect rect;
};

struct TitleBarMetrics {
  // 条高（逻辑像素）。与 Electron 同量级：按钮图 32×32 缩到 0.6 ⇒ 约 19。
  static constexpr double height = 20.0;
  // 每个按钮的宽（逻辑像素）；三个加起来就是右侧按钮簇的宽度。
  static constexpr double button = 20.0;
  // 边缘缩放带的宽度（**窗口像素**）。`borderless` 在 Windows 上是 `WS_POPUP` ⇒ 没有原生可拖的边框
  // （`platform/windows/display_server_windows.cpp` 的 `_get_window_style`），所以自己留一条。
  static constexpr double resize_band = 6.0;
  static constexpr int count = 3;
};

// 三个按钮的矩形：右对齐、紧挨着 —— close 在最右，往左依次 maximize / minimize（Electron 的次序）。
inline void titlebar_layout(double screen_w, TitleBarButton out[TitleBarMetrics::count]) {
  const double w = TitleBarMetrics::button;
  const double h = TitleBarMetrics::height;
  const double base = screen_w - TitleBarMetrics::count * w;
  out[0] = TitleBarButton{TitleAction::minimize, TitleBarRect{base, 0.0, w, h}};
  out[1] = TitleBarButton{TitleAction::maximize, TitleBarRect{base + w, 0.0, w, h}};
  out[2] = TitleBarButton{TitleAction::close, TitleBarRect{base + 2.0 * w, 0.0, w, h}};
}

inline bool titlebar_rect_contains(const TitleBarRect& r, double x, double y) {
  return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

// 整条（不只是按钮那一簇）：这一带里的点击**一律**吞掉、不落到游戏/UI。比 Electron 严格一点 ——
// 那边只有 `-webkit-app-region: drag` 那一层吞点击，按钮底下的缝会漏到画布上。
inline bool titlebar_in_strip(double x, double y, double screen_w) {
  return x >= 0.0 && x < screen_w && y >= 0.0 && y < TitleBarMetrics::height;
}

// 点在哪个按钮上（`none` = 条上的空白处 ⇒ 拖拽区）。
inline TitleAction titlebar_action_at(double x, double y, double screen_w) {
  TitleBarButton buttons[TitleBarMetrics::count];
  titlebar_layout(screen_w, buttons);
  for (const TitleBarButton& b : buttons) {
    if (titlebar_rect_contains(b.rect, x, y)) return b.action;
  }
  return TitleAction::none;
}

// 边缘缩放带：坐标是**窗口像素**（调用方用 `Window::get_final_transform()` 把逻辑点换过去 —— 两者的
// 比值就是窗口缩放，letterbox 时还带偏移）。角优先于边（与原生边框同口径）。
inline TitleEdge titlebar_edge_at(double wx, double wy, double win_w, double win_h,
                                 double band = TitleBarMetrics::resize_band) {
  const bool left = wx < band;
  const bool right = wx >= win_w - band;
  const bool top = wy < band;
  const bool bottom = wy >= win_h - band;
  if (left && top) return TitleEdge::top_left;
  if (right && top) return TitleEdge::top_right;
  if (left && bottom) return TitleEdge::bottom_left;
  if (right && bottom) return TitleEdge::bottom_right;
  if (top) return TitleEdge::top;
  if (bottom) return TitleEdge::bottom;
  if (left) return TitleEdge::left;
  if (right) return TitleEdge::right;
  return TitleEdge::none;
}

}  // namespace lfw::host
