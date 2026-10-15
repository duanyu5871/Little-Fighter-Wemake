// 场景入口（§G1 骨架里「跑起来」的那一半 + §G2 第二半「画出来」）：一个 `Node2D`。
//
//   `_ready()`   → 读命令行（`--` 之后的用户参数）→ 装包 → `boot_scene`（`host/common/lfw_boot.h`
//                  的**同一份**共享装配，不另写一条 boot 路径）
//   `_process()` → 按 §G3 的固定步长累加器推帧；每步之后按 `lfw_thin` 的口径补一次
//                  `World::render_once`（本宿主不装 `IRenderScheduler` ⇒ 世界自己不会渲染）
//                  → `queue_redraw()`；推够 `--frames`（或到了 `--shot-frame`）就打摘要并 `quit()`
//   `_input()`   → 平台事件（鼠标按下/抬起/移动/滚轮）→ `LfwPointingEvent` → `POINTER_*` 命令
//                  （`host/common/ui_input_handle.cpp`）⇒ UI 真的点得动
//   `_draw()`    → 把世界渲染器组好的 `GodotDrawItem` 变成 `draw_*`（**唯一**碰引擎绘制的地方）：
//                  按 `pass` 三趟扫（-1 UI 背景 → 0 世界 → +1 UI 前景 = D1(a) 的五遍次序），
//                  纯色块 / 图片 blit / 文字（`draw_string`，字体是引擎的默认字体）
//
// **两种跑法**（`--play` 是岔路口）：
//   * 开放模式（`--play`；`npm run godot` 用的就是它）：**不** `boot_scene`（由数据自己的入口页
//     `init` 驱动，点进去才算走进游戏）、**不**限帧数（关窗口才结束）、缺省不打帧摘要，节拍用
//     Godot 给的真 `delta`（真时间）。截图仍可用（`--screenshot` + `--shot-frame`）。
//   * 验收模式（`--frames N`；`npm run godot:headless` 用的）：与 `lfw_run` 同形 —— 强制
//     `boot_scene` 造场景、固定步长 + `--delta` 节拍、每 `--every` 帧打一行摘要。**三条摘要靠这一支。**
//
// 引擎侧只有这一个类 + 一个注册函数（`register_types.cpp`）：所有 Godot 类型都止步于此
//（§G5「只在边界转换」）。命令行参数与 `lfw_run` 同名（`--root` / `--zip` / `--fighter` /
// `--stage` / `--players` / `--frames` / `--every` / `--time` / `--vs`），另加：
//   * `--play`：开放模式（= `--frames 0` + 不 boot_scene；见上）；
//   * `--delta`：**只覆盖节拍**（`_process` 的 `delta` 从哪来），从不改变固定步长 —— 默认仍是 Godot
//     给的真 `delta`，headless 验收用它把「15 秒虚拟时间」压成几百个 Godot 帧；
//   * `--click <x>,<y>[@<frame>]` + `--click-frame N`：脚本化点击（坐标是 UI 逻辑像素），与
//     `lfw_run --click` 走同一条注入路径；
//   * `--screenshot <png>` + `--shot-frame N`：画到第 N 帧后存一张视口 PNG（⚠ 别加 `--headless`）。
//   * `--ui-page <id>` + `--ui-dump`：装载后把某一页设到第 0 层（拍 UI 用）/ 逐 UI 节点打一行事实。
#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node2d.hpp>

#include "host/fs_zip.h"
#include "host/godot/lfw_godot_host.h"
#include "host/godot/lfw_godot_renderer.h"
#include "host/godot/lfw_godot_textures.h"
#include "lfw/base/clock.h"
#include "lfw/lfw.h"

namespace lfw::host {

using godot::Node2D;

// ⚠ 是 `Node2D`（不是 `Node`）：世界画在 `_draw()` 里（§G2 第二半），引擎侧只有这一个类
// 会把 `GodotDrawItem` 变成 `draw_*` 调用（§G5「只在边界转换」）。
class LfwGodot : public Node2D {
  GDCLASS(LfwGodot, Node2D)  // NOLINT

 public:
  LfwGodot() = default;
  // 端口那两个全局槽（`set_clock` / `set_timeout`）指的都是本节点持有的宿主对象
  // ⇒ 节点被回收时先摘掉，别留悬空指针（编辑器里重载场景 / 反复进出都会走到）。
  // 定义写在头里（inline）：MSVC 会在 `register_types.cpp` 那一侧也落一个实例。
  ~LfwGodot() override {
    lfw::set_clock(nullptr);
    lfw::set_timeout(nullptr);
  }

  void _ready() override;
  void _process(double p_delta) override;
  // 平台指针/键盘事件进来的唯一入口（GDExtension 要先 `set_process_input(true)`）：翻成
  // `LfwPointingEvent` 交给 `HeadlessHost::emit_pointer_*`（与 `lfw_run --click` 同一条注入路径）。
  void _input(const godot::Ref<godot::InputEvent>& p_event) override;
  // 窗口关掉时打一行收尾（开放模式没有帧数上限，正常运行只会这样结束）。
  // ⚠ 不能写 `override`：godot-cpp 的 `Wrapped::_notification` 不是虚函数，靠成员指针比较
  // （`wrapped.hpp:264-275`）认出来 —— 名字隐藏就行。
  void _notification(int p_what);
  void _draw() override;

 protected:
  static void _bind_methods();

 private:
  // `lfw_run` 的同名参数 + `delta`（节拍覆盖）+ 截图那两条 + 开放模式/脚本化点击。
  struct Args {
    std::vector<std::filesystem::path> roots;
    std::vector<std::filesystem::path> zips;
    std::vector<std::u16string> fighters;
    std::u16string stage;
    int frames = 600;  // 0 = 不限（开放模式）；> 0 = 推这么多帧就收工
    int players = 2;
    // `--every N`：每 N 帧打一行摘要。缺省 0 = 按跑法自动（开放模式静默、验收模式 60）。
    int every = 0;
    double time = 0.0;
    double delta = 0.0;  // 秒；0 = 用 Godot 给的 `delta`
    bool vs = false;
    // `--play`：开放模式（= `--frames 0` + 不 `boot_scene`）。`npm run godot` 就是它。
    bool play = false;
    std::filesystem::path screenshot;  // 空 = 不截图
    int shot_frame = -1;               // < 0 = 缺省（有截图时取 120）
    bool ui_dump = false;              // `--ui-dump`：最后一帧逐 UI 节点打一行事实
    std::u16string ui_page;             // `--ui-page <id>`：装载后把这一页设到第 0 层（等价点进去）
    // `--click <x>,<y>[@<frame>]`：在第 `frame` 帧注入一次完整点击（按下 + 抬起）。坐标是
    // UI 逻辑像素（左上原点，与截图 1:1）。`--click-frame N` 给没写 `@frame` 的定帧（缺省 60）。
    struct ClickAt {
      int frame = 0;
      double x = 0;
      double y = 0;
    };
    std::vector<ClickAt> clicks;
    std::vector<std::string> click_specs;
    int click_frame = 60;
  };

  static const char* usage();
  static std::vector<std::string> user_args();
  bool parse_args(const std::vector<std::string>& argv, Args& out) const;
  bool setup();  // 装包 + `LFW` + `boot_scene`；失败时已经把错误打出去了
  bool unlimited() const { return _args.frames <= 0; }  // 开放模式：推到窗口关掉为止
  // 文字度量（D4：「字体度量在宿主」）：引擎的默认字体算 `w/h`，装进 `LfwGodotHost`。
  void install_text_metrics();
  void report_frame(int frame) const;
  void render_world_frame(long long renders_before);  // 每步补一次 `render_once` + `take_frame`
  // ---- 指针（`_input` 那一半）----
  // UI 逻辑视口尺寸（数据里的 `dataset.screen_w × screen_h`）；UI 的坐标空间就是它。
  void ui_screen_size(double& w, double& h) const;
  // 平台事件 → `LfwPointingEvent`（`none` = 这个事件不是指针事件）。
  enum class PointerKind { none, down, up, move, wheel };
  PointerKind to_pointing_event(const godot::Ref<godot::InputEvent>& event,
                                LfwPointingEvent& out) const;
  Args::ClickAt* inject_clicks(int frame);  // `--click`：返回这一帧点过的第一个（没有则 nullptr）
  std::string ui_pages_text() const;
  void draw_item(const GodotDrawItem& item);
  void draw_text_item(const GodotDrawItem& item);
  bool capture_screenshot();       // 视口 → PNG
  void finish();
  void quit_with(int code) const;

  Args _args;
  std::unique_ptr<LfwGodotHost> _host;
  std::vector<std::unique_ptr<FsZip>> _zips;
  std::unique_ptr<lfw::LFW> _lfw;
  std::unique_ptr<GodotTextureCache> _textures;
  godot::Ref<godot::Font> _font;  // 引擎的默认字体（文字度量 + `_draw` 的 `draw_string`）
  bool _running = false;
  int _frames = 0;
  bool _capture_pending = false;  // 这一帧要截图（上一帧的绘制已经落进视口）
  bool _captured = false;         // 截过图（只截一张）
};

}  // namespace lfw::host

