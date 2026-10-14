// 录制型 `IWorldRenderer`：**不画**，只把「核心每帧要渲染器画什么」记下来。
//
// 为什么要有它：`native/lfw/**` 里的 `IWorldRenderer`（`lfw/world.h:61-68`）是**保留式**的
// —— `add_entity` / `del_entity` 只在实体进出世界时各调一次，`render(dt)` 每帧调一次，而且
// 拿的是**活实体的引用**（节点/精灵在真实渲染器里长期持有）。所以「一帧要画什么」这件事
// 只有渲染器自己知道 —— 它必须**每帧**从它持有的活实体上取一份当前值。
//
// 这一份就是那份快照：字段全部来自 `Entity` 现有的公开面（`native/lfw/entity/entity.h`），
// **没有一个是编的**；缺什么在报告里点出来（例如没有逐实体 `alpha`，只有 `invisible` /
// `blinking` 两个开关量）。
//
// 输出格式（`--dump` 落盘 / 终端摘要都用它，**一个实体一行、一帧一段**）：
//
//   FRAME <n> dt=<ms> life=<lifetime> ret=<记录器持有的活体数> live=<world.entities+ghosts>
//         match=<ret==live> cam=<x>,<y> bg=<bg.id> layer=<layer_data_index> upd=<update_times>
//         xform=<x>,<y>,<z> scale=<sx>,<sy>,<sz> screen=<w>,<h> ents=<本帧记录条数>
//   E id=<entity.id> data=<data.id> type=<data.type> pos=<x>,<y>,<z> prev=<上次快照的 pos>
//     facing=<±1> wait=<帧内计时> frame=<frame.id> state=<frame.state> pf=<prev_frame.id>
//     tex=<frame.pic.tex 素材 id> crop=<x>,<y>,<w>,<h> pics=<frame.pics 层数>
//     c=<frame.centerx>,<centery>
//     hp=<hp>/<hp_max> mp=<mp>/<mp_max> tough=<toughness>/<toughness_max>
//     invis=<invisible> blink=<blinking> invuln=<invulnerable>
//     name_vis=<name_visible> ctrl_vis=<ctrl_visible> name=<显示名> variant=<队伍变体>
//     shake=<受击抖动> life=<实体存活帧数> mix=<mix_color>@<mix_strength> grey=<greyscale>
//     outline=<outline_color>@<outline_alpha>,<outline_width> ol_en=<outline_enabled>
//     fx=<render_effect_time> ghost=<ghosted> mount=<mounted>
//
// 数字一律 `%.4f`（位置/速度都落在 1/1000 格点上，四位足够且逐位可复现）；取不到的字符串
// 字段落成 `-`。`prev` / 相机那条要留意：`p0` 那一半在真渲染器里也是**自己留的**（见下）。
//
// ⚠ **`prev=<...>` 是记录器自己上一次 `render()` 看到的 `pos`**，就是 TS `EntityRenderer.p0/p1`
// 的那对（`src/DittoImpl/renderer/EntityRenderer.ts` 的 `update_position` / `render(dt, df)`）：
// 逐实体插值靠「渲染器留一份上次值」+ 一个帧内比例。端口 `IWorldRenderer::render(double dt)`
// **不传那个比例**（TS 的 `dfactor` 是渲染器自己用 `world.TU` / `lifetime` / `FPS` 现算的），
// 所以这里只记 `pos` / `prev` 一对原始值，不猜比例。
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "lfw/lfw.h"
#include "lfw/world.h"

namespace lfw::host {

// 一个活体在某一次 `render()` 时的绘制记录（字段序与 `format_entity_record` 一一对应）。
struct EntityDrawRecord {
  std::string id;
  std::string data_id;  // 画哪套素材
  double type = 0;      // data.type（0 = 角色，1 = 武器，2 = 球…）
  // 位置与「记录器上次看到的同一实体的位置」。
  double x = 0, y = 0, z = 0;
  double px = 0, py = 0, pz = 0;
  double facing = 1;  // +1 朝右 / -1 朝左（精灵翻面）
  double wait = 0;    // 帧内计时（当前动作帧走到哪儿）
  std::string frame_id;
  std::string state;
  std::string prev_frame_id;
  // 当前帧的精灵引用（`frame.pic`，`IFramePic`）：`tex` 是素材 id，`x/y/w/h` 是 atlas 裁剪矩形
  // —— 真渲染器就是拿这一组去取图 + 摆位（`EntityMainRender` 的 `frame.pic` / `frame.pics`）。
  std::string pic_tex;  // 没有 `pic` 时落成 "-"（例如纯 3D 模型帧）
  double pic_x = 0, pic_y = 0, pic_w = 0, pic_h = 0;
  double pics_count = 0;                // `frame.pics` 的层数（多图层）
  double frame_cx = 0, frame_cy = 0;    // `frame.centerx/centery`（精灵锚点）
  // 状态条（`EntityStatRender` 读的就是这一族）。
  double hp = 0, hp_max = 0, mp = 0, mp_max = 0;
  double toughness = 0, toughness_max = 0;
  // 可见度 / 闪烁 —— 端口**没有**逐实体 `alpha`，只有这两个开关量（见文件头）。
  double invisible = 0, blinking = 0, invulnerable = 0;
  double name_visible = 0, ctrl_visible = 0;
  std::string name;          // 显示名（`EntityNameRender` / 血条旁边那行）
  double variant = 0;        // 队伍/皮肤变体（`Entity` 的 `variant`）
  double shaking = 0;        // 受击抖动（渲染偏移）
  double lifetime = 0;       // 实体自身的存活帧数（出生淡入用，不是 world.lifetime）
  // 外观（TS 的 `_render_effect_*` 一族）：换色/描边/灰度 + 一个「外观改动过」的计数。
  std::string mix_color;
  double mix_strength = 0, greyscale = 0;
  std::string outline_color;
  double outline_alpha = 0, outline_width = 0;
  std::string outline_enabled;  // `outline_enabled()`；`null` ⇒ "-"（走缺省）
  double render_effect_time = 0;
  double ghosted = 0, mounted = 0;
};

// 一帧的绘制记录：帧级上下文（相机 / 背景 / 世界变换 / 屏幕尺寸）+ 本帧每条活体。
struct FrameDrawRecord {
  int index = 0;       // 记录器第几次 `render()`（从 1 起；宿主每帧只推一次 ⇒ 与帧号同序）
  double dt = 0;       // 传进来的 dt（毫秒）
  double lifetime = 0;
  int retained = 0;    // `add_entity` / `del_entity` 维护出来的活体数
  int world_live = 0;  // `world.entities + world.ghosts`（对拍：保留式记账有没有漏）
  bool retained_matches_world = true;
  double cam_x = 0, cam_y = 0;
  std::string bg_id;
  double bg_layer_data_index = 0, bg_update_times = 0;
  double tx = 0, ty = 0, tz = 0;     // world.transform（真渲染器拿它摆世界节点）
  double tsx = 1, tsy = 1, tsz = 1;  // world.transform.scale_*
  double screen_w = 0, screen_h = 0;  // dataset（`EntityRenderer` 的 Message 夹取用）
  std::vector<EntityDrawRecord> entities;
};

// 格式化的三条（`--dump` 的头部与正文都用它们，避免两处格式漂移）。
std::string format_frame_header(const FrameDrawRecord& frame);
std::string format_entity_record(const EntityDrawRecord& entity);
// `--dump` 文件里紧跟头部的一行字段名（= `EntityDrawRecord` 的字段序）。
std::string format_entity_field_names();

// 录制型世界渲染器。持 `LFW&` 是为了取帧级上下文（相机 / 背景 / transform / dataset）——
// 这些在 `IWorldRenderer` 的四个成员里拿不到，而真渲染器同样要从世界读它们。
class RecordingWorldRenderer : public IWorldRenderer {
 public:
  explicit RecordingWorldRenderer(LFW& lfw) : _lfw(&lfw) {}

  // ---- IWorldRenderer（保留式）----
  void add_entity(Entity& e) override;
  void del_entity(Entity& e) override;
  void render(double dt) override;
  void dispose() override;

  // ---- 观测面（CLI 用）----
  int render_count() const { return _render_count; }
  const std::vector<FrameDrawRecord>& frames() const { return _frames; }
  std::size_t retained_count() const { return _live.size(); }
  long long added() const { return _added; }
  long long removed() const { return _removed; }
  long long duplicate_adds() const { return _duplicate_adds; }
  long long stray_removes() const { return _stray_removes; }
  long long mismatched_frames() const { return _mismatched_frames; }
  bool disposed() const { return _disposed; }

 private:
  // 记录器持有的活体 + 上一次 `render()` 看到的位置（`p0` 的替身）。
  struct Live {
    Entity* e = nullptr;
    double px = 0, py = 0, pz = 0;
  };

  LFW* _lfw = nullptr;
  std::vector<Live> _live;  // 插入序；`render()` 里按 id 排序后记录
  std::vector<FrameDrawRecord> _frames;
  int _render_count = 0;
  long long _added = 0;
  long long _removed = 0;
  long long _duplicate_adds = 0;
  long long _stray_removes = 0;
  long long _mismatched_frames = 0;
  bool _disposed = false;
};

}  // namespace lfw::host
