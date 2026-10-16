// Godot 侧的宿主：与 WASM 宿主同一种做法 —— **继承** `HeadlessHost`
// （`host/headless_host.{h,cpp}` + `host/fs_zip.{h,cpp}` 直接编进扩展，装载 / `IZip` / 存档 /
// XML / `import_as_*` 一个都不重写），只把「帧从哪来」这件事按 DESIGN §G3 钉死。
//
// §G3 的四条规矩，这一份就是它们的落地：
//   1. **不装** `IRenderScheduler`（槽空 ⇒ `render_add` 回 0）；本类也不碰它。
//   2. 时钟是**注入的虚拟时钟**：`HeadlessHost` 的 `ManualClock` 的 `add` 回 0
//      ⇒ 端口自己的 `Ticker` 永远不自驱（这正是 WASM 那边「一帧走两步」的坑）。
//   3. 世界由宿主用**固定步长**手动推：`clock().advance(step)` + `world.update_once(step)`。
//      Godot `_process(delta)` 的 `delta`（秒）**只当节拍**喂进累加器，绝不进模拟。
//   4. 于是这一支与 `lfw_run` 的手动模式逐位同形 ⇒ 三条摘要可比（§G3 的硬指标）。
//
// ⚠ 这一层**不许**出现 Godot 类型（`godot::String` / `Node` / …）：它只做「虚拟时钟 +
// 固定步长累加器」，引擎那一侧在 `lfw_godot_node.{h,cpp}`。分开放的理由是 §G5 的
// 「只在边界转换」——本文件里一个转换都不需要。
#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

#include "host/godot/lfw_godot_renderer.h"
#include "host/godot/lfw_godot_ui_renderer.h"
#include "host/headless_host.h"

namespace lfw {
class World;
}  // namespace lfw

namespace lfw::host {

// 固定步长 + 注入虚拟时钟的推帧器（挂在 `HeadlessHost` 上）。
class LfwGodotHost : public HeadlessHost {
 public:
  LfwGodotHost(std::vector<std::filesystem::path> roots, bool dev);

  // 一帧的固定步长（毫秒）：`1000 / dataset.UPS`（与 `lfw_run` 同款）。
  void set_step_ms(double step_ms) { _step_ms = step_ms; }
  double step_ms() const { return _step_ms; }

  // 喂节拍：`delta` 是**秒**，只用来决定「这一帧推几步」，不进模拟。
  void accumulate(double delta_seconds) { _accumulator_ms += delta_seconds * 1000.0; }

  // 累加器够不够一个步长（够就 `push_frame`）。
  bool can_step() const { return _accumulator_ms >= _step_ms; }

  // 推一帧：虚拟时钟前进一个步长 + `World::update_once`。这两句必须成对出现
  //（只推时钟不推世界 ⇒ 从 `clock_now()` 派生的量会漂；只推世界不推时钟 ⇒ 反过来）。
  void push_frame(World& world);

  // 诊断：累加器余量（毫秒）与累计推过的帧数。
  double residual_ms() const { return _accumulator_ms; }
  long long steps() const { return _steps; }

  // ---- 世界渲染器（§G2 第二半）----
  // `LFW` 构造里 `new Ditto.WorldRender(world)` 那一处：每次新建一份（一个 `LFW` 一份）。
  IWorldRenderer* create_world_renderer(LFW& lfw) override {
    _renderer = std::make_unique<GodotWorldRenderer>(lfw, *this);
    return _renderer.get();
  }
  GodotWorldRenderer* world_renderer() const { return _renderer.get(); }

  // ---- UI 渲染器（D4 第二半）----
  // `LFW` 构造里每个 `UINode` 的 `new Ditto.UINodeRenderer(node)` 那一处（`uinode.cpp` 的 ctor）。
  // 所有权归节点；绘制项落进世界渲染器"正在写的那一帧"（`GodotWorldRenderer::ui_append`）。
  // ⚠ `_renderer` 一定已经在（`create_world_renderer` 在 `LFW` ctor 里先跑）。
  ui::IUINodeRenderer* create_ui_node_renderer(ui::UINode& node) override {
    static_cast<void>(node);
    if (_renderer == nullptr) return nullptr;
    return new GodotUINodeRenderer(*_renderer);
  }

  // ---- 文字度量（D4 的"允许宿主差异"那一格）----
  // 字体只有引擎那一侧有（§G5）⇒ 度量函数由 `lfw_godot_node.cpp` 装进来；没装就退回
  // `HeadlessHost` 的零尺寸桩（布局照算，只是文字挤在一起）。
  using MeasureTextFn = std::function<Value(const Value& text, const Value& style)>;
  void set_measure_text(MeasureTextFn fn) { _measure_text = std::move(fn); }
  Value measure_text(const Value& text, const Value& style) override;

  // ---- 图片尺寸（DESIGN 允许的"设备侧差异"那一格）----
  // `ui_image_load` 的结果里 `w`/`h`/`scale` 是**图片尺寸**：TS 是浏览器解码出来的（`ImageInfo`），
  // 端口这一侧由资源表（命中条目里的 `@Nx` + PNG 头）算出来。不补这三个数，`UIImgLoader::load`
  // 的 `node.resize(w / scale, h / scale)` 就会算出 `NaN` —— 那会把节点尺寸写成 NaN：盒子 0×0
  // 画不出来，`cook_ui_info` 按 `img_info` 补零轴那一步也一起失效。
  bool ui_image_load(const std::u16string& img_key, const Value& path, const Value& ops, Value& out,
                     std::u16string& error) override;

 private:
  double _step_ms = 1000.0 / 60.0;
  double _accumulator_ms = 0.0;
  long long _steps = 0;
  std::unique_ptr<GodotWorldRenderer> _renderer;
  MeasureTextFn _measure_text;
};

}  // namespace lfw::host
