#include "host/godot/lfw_godot_node.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include "host/common/lfw_boot.h"
#include "lfw/base/clock.h"
#include "lfw/core/value.h"
#include "lfw/utils/utf8.h"
#include "lfw/world.h"

namespace lfw::host {
namespace {

using godot::String;
using godot::UtilityFunctions;
using godot::Color;
using godot::Rect2;
using godot::Vector2;

// UTF-8 → `std::u16string`：`Value` 里的字符串是 UTF-16（§G5）。
std::u16string from_utf8(const std::string& s) {
  return decode_utf8(std::vector<uint8_t>(s.begin(), s.end()));
}

std::string to_utf8(const std::u16string& s) {
  const std::vector<uint8_t> buf = encode_utf8(s);
  return std::string(buf.begin(), buf.end());
}

// `world.dataset.get(k)` 缺省时（`undefined`）给 TS 的缺省值。
double num_field(const Value& v, double fallback) {
  if (std::holds_alternative<std::monostate>(v)) return fallback;
  const double n = to_number(v);
  return std::isnan(n) ? fallback : n;
}

// 宿主自己的输出走 Godot 的打印（headless 下就是 stdout，编辑器里进 Output 面板）。
void print_line(const std::string& text) { UtilityFunctions::print(String::utf8(text.c_str())); }

void print_error(const std::string& text) { UtilityFunctions::printerr(String::utf8(text.c_str())); }

// 报错时把「进程当前目录」一起带上：Godot 跑工程时会把 cwd 切到**工程目录**（`--path` 的副作用）
// ⇒ `--zip` 给相对路径时最常见的失败就是这个。
std::string cwd_string() {
  std::error_code ec;
  const std::filesystem::path cwd = std::filesystem::current_path(ec);
  return ec ? std::string("<unknown>") : cwd.generic_string();
}

}  // namespace

const char* LfwGodot::usage() {
  return "usage: <godot> --path native/host/godot/project -- --root <dir> --zip <dir>\n"
         "               [--zip <dir> ...] [--fighter <id>] [--stage <id>] [--players N]\n"
         "               [--frames N] [--every N] [--time ms] [--delta seconds] [--vs]\n"
         "               [--screenshot <file.png>] [--shot-frame N]\n"
         "  --root     import 根目录（含 builtin_data/…），可重复；缺省 `.`\n"
         "  --zip      数据包目录（已解包），可重复；按给定顺序 load\n"
         "  --fighter  指定出场的角色 id，可重复（缺省用数据包里的前 `--players` 个）\n"
         "  --delta    只覆盖 `_process` 的节拍（秒），不改固定步长（默认用 Godot 给的 delta）\n"
         "  --screenshot  画到第 `--shot-frame` 帧后存一张 PNG（**别加 --headless**：headless\n"
         "                没有渲染，存出来是空白）；给绝对路径\n"
         "  --shot-frame  截图的帧号（缺省 120）\n";
}

std::vector<std::string> LfwGodot::user_args() {
  std::vector<std::string> out;
  const godot::PackedStringArray args = godot::OS::get_singleton()->get_cmdline_user_args();
  for (int64_t i = 0; i < args.size(); ++i) {
    const String arg = args[i];
    out.emplace_back(arg.utf8().get_data());
  }
  return out;
}

bool LfwGodot::parse_args(const std::vector<std::string>& argv, Args& out) const {
  const auto need = [&](size_t& i, std::string& value) -> bool {
    if (i + 1 >= argv.size()) return false;
    value = argv[++i];
    return true;
  };
  for (size_t i = 0; i < argv.size(); ++i) {
    const std::string& a = argv[i];
    std::string v;
    if (a == "--root") {
      if (!need(i, v)) return false;
      out.roots.emplace_back(v);
    } else if (a == "--zip") {
      if (!need(i, v)) return false;
      out.zips.emplace_back(v);
    } else if (a == "--fighter") {
      if (!need(i, v)) return false;
      out.fighters.push_back(from_utf8(v));
    } else if (a == "--stage") {
      if (!need(i, v)) return false;
      out.stage = from_utf8(v);
    } else if (a == "--frames") {
      if (!need(i, v)) return false;
      out.frames = std::atoi(v.c_str());
    } else if (a == "--players") {
      if (!need(i, v)) return false;
      out.players = std::atoi(v.c_str());
    } else if (a == "--every") {
      if (!need(i, v)) return false;
      out.every = std::atoi(v.c_str());
    } else if (a == "--time") {
      if (!need(i, v)) return false;
      out.time = std::atof(v.c_str());
    } else if (a == "--delta") {
      if (!need(i, v)) return false;
      out.delta = std::atof(v.c_str());
    } else if (a == "--screenshot") {
      if (!need(i, v)) return false;
      out.screenshot = std::filesystem::path(v);
    } else if (a == "--shot-frame") {
      if (!need(i, v)) return false;
      out.shot_frame = std::atoi(v.c_str());
    } else if (a == "--vs") {
      out.vs = true;
    } else {
      return false;
    }
  }
  if (out.roots.empty()) out.roots.emplace_back(".");
  if (out.every <= 0) out.every = 60;
  if (!out.screenshot.empty() && out.shot_frame < 0) out.shot_frame = 120;
  return !out.zips.empty() && out.frames > 0;
}

void LfwGodot::_ready() {
  const std::vector<std::string> argv = user_args();
  if (argv.empty()) {
    print_line("[godot] no LFW arguments after `--`; nothing to do.");
    print_line(usage());
    return;  // 编辑器里直接打开这个项目时不留副作用
  }
  _args = Args{};
  if (argv.size() == 1 && (argv[0] == "--help" || argv[0] == "-h")) {
    print_line(usage());
    return;
  }
  if (!parse_args(argv, _args)) {
    print_error("[godot] bad arguments");
    print_line(usage());
    quit_with(64);
    return;
  }
  if (!setup()) {
    quit_with(1);
    return;
  }
  _running = true;
}

bool LfwGodot::setup() {
  _host = std::make_unique<LfwGodotHost>(_args.roots, false);
  // 时钟 / 定时器在手（`ManualClock::add → 0`）⇒ 端口自己的 `Ticker` 不自驱（§G3 第 2 条）。
  // 必须在 `LFW` 构造**之前**装好：`host.now()` 给 `MersenneTwister` 播种（§G9）。
  lfw::set_clock(&_host->clock());
  lfw::set_timeout(&_host->timeout());
  _host->clock().set(_args.time);

  std::vector<lfw::LFW::ZipItem> items;
  for (const std::filesystem::path& dir : _args.zips) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
      print_error("[godot] not a directory: " + dir.generic_string() + " (cwd=" + cwd_string() + ")");
      return false;
    }
    _zips.push_back(std::make_unique<FsZip>(from_utf8(dir.generic_string()), dir));
    items.push_back(lfw::LFW::ZipItem{u"", _zips.back().get()});
    char buf[256];
    std::snprintf(buf, sizeof(buf), "[godot] package: %s (%zu entries)", dir.generic_string().c_str(),
                  _zips.back()->entry_count());
    print_line(buf);
  }

  _lfw = std::make_unique<lfw::LFW>(*_host, false);
  std::u16string error;
  if (!_lfw->load(items, error)) {
    print_error("[godot] load failed: " + to_utf8(error));
    for (const std::u16string& e : _host->last_errors()) print_error("  - " + to_utf8(e));
    return false;
  }
  {
    char buf[320];
    std::snprintf(buf, sizeof(buf),
                  "[godot] loaded: fighters=%zu weapons=%zu balls=%zu entities=%zu stages=%zu "
                  "bg=%zu bots=%zu moves=%zu",
                  _lfw->datas().fighters().size(), _lfw->datas().weapons().size(),
                  _lfw->datas().balls().size(), _lfw->datas().entities().size(),
                  _lfw->datas().stages().size(), _lfw->datas().backgrounds().size(),
                  _lfw->datas().bots().size(), _lfw->datas().moves().size());
    print_line(buf);
  }

  // ---- 装好这场对局：与 `lfw_run` 共用 `boot_scene`（不另写一条装配路径）----
  BootOptions boot;
  boot.fighters = _args.fighters;
  boot.stage = _args.stage;
  boot.players = _args.players;
  boot.vs = _args.vs;
  boot.on_spawn = [](const SpawnInfo& s) {
    char buf[320];
    std::snprintf(buf, sizeof(buf), "[godot] spawn p%s %s @(%.1f, %.1f, %.1f) team=%s%s",
                  to_utf8(s.player_id).c_str(), to_utf8(s.fighter_id).c_str(), s.x, s.y, s.z,
                  to_utf8(s.team).c_str(), s.vs ? " (vs)" : "");
    print_line(buf);
  };
  boot.on_failure = [](const BootFailure& f) {
    print_error(f.create_failed ? "[godot] create fighter failed: " + to_utf8(f.fighter_id)
                                : "[godot] fighter data not found: " + to_utf8(f.fighter_id));
    return true;  // `lfw_run` 口径：记账后继续
  };
  const BootResult booted = boot_scene(*_lfw, boot);
  if (!booted.ok) {
    print_error("[godot] no fighter data in the loaded packages");
    return false;
  }
  {
    char buf[320];
    std::snprintf(buf, sizeof(buf), "[godot] stage=%s bg=%s entities=%zu",
                  to_utf8(booted.stage_id).c_str(), to_utf8(booted.bg_id).c_str(),
                  _lfw->world().entities.size());
    print_line(buf);
  }

  // 固定步长来自数据本身（`dataset.UPS`），与 `lfw_run` 同一口径。
  lfw::World& world = _lfw->world();
  const double ups = num_field(world.dataset.get(u"UPS"), 60.0);
  _host->set_step_ms(1000.0 / (ups > 0 ? ups : 60.0));

  // 纹理缓存（§G2：图片解码归宿主）：字节走渲染器的资源读（数据包 → 根目录），解码用引擎的
  // PNG 解码器。渲染器是 `LFW` 构造里 `create_world_renderer` 建的 ⇒ 这时候一定在。
  if (_host->world_renderer() != nullptr) {
    _textures = std::make_unique<GodotTextureCache>([this](const std::string& path,
                                                          std::string& out) {
      GodotWorldRenderer* const renderer = _host->world_renderer();
      return renderer != nullptr && renderer->read_resource(path, out);
    });
  }
  return true;
}

void LfwGodot::_process(double p_delta) {
  if (!_running) return;
  // 截图：上一帧已经画完 ⇒ 视口纹理里就是那一帧。取完就收工（`--frames` 不再等）。
  if (_capture_pending) {
    _capture_pending = false;
    capture_screenshot();
    finish();
    return;
  }
  // 节拍只决定「这一帧推几步」；固定步长永远是 `_host->step_ms()`（§G3 第 3 条）。
  double pacing = _args.delta > 0.0 ? _args.delta : p_delta;
  if (!(pacing > 0.0)) pacing = 1.0 / 60.0;  // 节拍兜底：别让 0 delta 把验收卡死
  _host->accumulate(pacing);

  const int shot_at = _args.shot_frame > 0 ? _args.shot_frame : -1;
  bool stepped = false;
  while (_running && _frames < _args.frames && _host->can_step()) {
    GodotWorldRenderer* const renderer = _host->world_renderer();
    const long long before = renderer != nullptr ? renderer->renders() : 0;
    _host->push_frame(_lfw->world());
    render_world_frame(before);
    stepped = true;
    const int frame = _frames;
    ++_frames;
    if (frame % _args.every == 0 || frame == _args.frames - 1) report_frame(frame);
    if (shot_at >= 0 && _frames >= shot_at) {
      _capture_pending = true;  // 这一帧的绘制落在本帧末尾 ⇒ 下一帧再取视口
      break;
    }
  }
  if (stepped) queue_redraw();
  if (_capture_pending) return;
  if (_frames >= _args.frames) finish();
}

// 每个 step 之后补一次 `World::render_once`（§D4：`sync_render != Sync` 时世界自己**不会**
// 渲染 —— `IRenderScheduler` 的槽在本宿主里是空的，与 `lfw_thin` 同款）。这一步只碰渲染器与
// UI 渲染面的计数，不改模拟（三条摘要实测不变；`render_cost` 只进诊断）。
void LfwGodot::render_world_frame(long long renders_before) {
  lfw::World& world = _lfw->world();
  GodotWorldRenderer* const renderer = _host->world_renderer();
  if (renderer == nullptr) return;
  // 世界自己渲染过（`sync_render == Sync/Half`）就不要再补一次。
  if (renderer->renders() == renders_before) world.render_once(_host->step_ms());
  renderer->take_frame();  // 双缓冲：把写好的那一帧翻给 `_draw` 读
}

// 引擎侧唯一一处把 `GodotDrawItem` 变成像素的地方（§G5「只在边界转换」）。
void LfwGodot::_draw() {
  GodotWorldRenderer* const renderer = _host != nullptr ? _host->world_renderer() : nullptr;
  if (renderer == nullptr) return;
  const GodotFrame& frame = renderer->take_frame();
  if (!frame.valid) return;
  for (const GodotDrawItem& item : frame.items) {
    draw_item(item);
  }
}

void LfwGodot::draw_item(const GodotDrawItem& item) {
  if (!(item.w > 0.0) || !(item.h > 0.0)) return;
  // `Rect2` / `Vector2` 吃的是 `godot::real_t`（这份绑定是单精度）⇒ 一开始就落成 float。
  const float x = static_cast<float>(item.x);
  const float y = static_cast<float>(item.y);
  const float w = static_cast<float>(item.w);
  const float h = static_cast<float>(item.h);
  const float alpha = static_cast<float>(item.a);
  const Color color(static_cast<float>(item.r), static_cast<float>(item.g),
                    static_cast<float>(item.b), alpha);
  godot::Ref<godot::ImageTexture> tex;
  if (_textures != nullptr && !item.path.empty()) tex = _textures->get(item.path);
  if (tex.is_null()) {
    // 占位：图没解出来（或这一层本来就没图）⇒ 一个色块 + 一圈深色边框。颜色按 `tag` 定，
    // 同一个实体每帧同色（便于目视对位）。
    std::uint32_t hash = 2166136261u;
    for (const char c : item.tag) hash = (hash ^ static_cast<unsigned char>(c)) * 16777619u;
    const float hue = static_cast<float>(hash % 360u) / 360.0f;
    const Color tint = Color::from_hsv(hue, 0.55f, 0.85f, alpha);
    draw_rect(Rect2(x, y, w, h), tint, true);
    draw_rect(Rect2(x, y, w, h), Color(0.1f, 0.1f, 0.15f, alpha), false, 1.0f);
    return;
  }
  const Rect2 src(static_cast<float>(item.sx * item.tex_scale),
                  static_cast<float>(item.sy * item.tex_scale),
                  static_cast<float>(item.sw * item.tex_scale),
                  static_cast<float>(item.sh * item.tex_scale));
  // 一次「blit」：`画在哪 + 什么颜色`。镜像用等价的水平翻转变换（TS 的 `flip_x` 是 UV 镜像）。
  const auto blit = [&](float ox, float oy, const Color& c) {
    if (item.flip_x) {
      draw_set_transform(Vector2(x + ox + w, y + oy), 0.0f, Vector2(-1.0f, 1.0f));
      draw_texture_rect_region(tex, Rect2(0.0f, 0.0f, w, h), src, c);
      draw_set_transform(Vector2(0, 0), 0.0f, Vector2(1.0f, 1.0f));
    } else {
      draw_texture_rect_region(tex, Rect2(x + ox, y + oy, w, h), src, c);
    }
  };
  // 描边：TS 是着色器按 alpha 边缘向外扩 `outline_width` 像素。这里用八向偏移的同图叠加做**等价
  // 形状**的轮廓（颜色是乘性的，不是纯色；宽度=逻辑像素）。真着色器进不了 `draw_*` 这一层
  //（逐绘制的材质在 CanvasItem 里没有），要逐像素的纯色描边得给每个精灵单开一个 CanvasItem。
  if (item.outline_w > 0.0 && item.outline_a > 0.0 && !item.path.empty()) {
    const float d = static_cast<float>(item.outline_w);
    const float q = d * 0.70710678f;
    const Color oc(static_cast<float>(item.outline_r), static_cast<float>(item.outline_g),
                   static_cast<float>(item.outline_b), static_cast<float>(item.outline_a));
    blit(-d, 0, oc);
    blit(d, 0, oc);
    blit(0, -d, oc);
    blit(0, d, oc);
    blit(-q, -q, oc);
    blit(q, -q, oc);
    blit(-q, q, oc);
    blit(q, q, oc);
  }
  blit(0, 0, color);
  if (item.mix_a > 0.0) {
    blit(0, 0,
         Color(static_cast<float>(item.mix_r), static_cast<float>(item.mix_g),
               static_cast<float>(item.mix_b), static_cast<float>(item.mix_a)));
  }
}

// 视口 → PNG（`--screenshot`）。⚠ 只有**真渲染**（不加 `--headless`）才有像素可存。
bool LfwGodot::capture_screenshot() {
  if (_args.screenshot.empty() || _captured) return false;
  _captured = true;
  godot::RenderingServer* const rs = godot::RenderingServer::get_singleton();
  if (rs != nullptr) rs->force_draw();  // 把这一帧推完，免得拿到半截
  godot::Viewport* const vp = get_viewport();
  if (vp == nullptr) {
    print_error("[godot] screenshot: no viewport");
    return false;
  }
  const godot::Ref<godot::ViewportTexture> vtex = vp->get_texture();
  if (vtex.is_null()) {
    print_error("[godot] screenshot: no viewport texture");
    return false;
  }
  const godot::Ref<godot::Image> img = vtex->get_image();
  if (img.is_null()) {
    print_error("[godot] screenshot: null image (headless has no renderer ⇒ drop --headless)");
    return false;
  }
  const godot::String path(_args.screenshot.generic_string().c_str());
  const godot::Error err = img->save_png(path);
  char buf[512];
  std::snprintf(buf, sizeof(buf), "[godot] screenshot: %s %dx%d err=%d frame=%d life=%.0f",
                _args.screenshot.generic_string().c_str(), img->get_width(), img->get_height(),
                static_cast<int>(err), _frames, _lfw != nullptr ? _lfw->world().lifetime() : 0.0);
  print_line(buf);
  return err == godot::OK;
}

void LfwGodot::report_frame(int frame) const {
  const lfw::World& world = _lfw->world();
  const std::string digest = frame_digest(world, frame);
  char buf[256];
  std::snprintf(buf, sizeof(buf), "[godot frame %6d] entities=%zu ghosts=%zu hp=%.1f digest=%s",
                frame, world.entities.size(), world.ghosts.size(), total_hp(world), digest.c_str());
  print_line(buf);
}

void LfwGodot::finish() {
  _running = false;
  const lfw::World& world = _lfw->world();
  const std::string digest = frame_digest(world, _frames - 1);
  GodotWorldRenderer* const renderer = _host->world_renderer();
  char buf[512];
  // ⚠ `renderer(+-)` 必须与 `NullWorldRenderer` 那份逐位一致（`native/README.md` 的三条摘要）：
  // 本渲染器的 `add_entity` / `del_entity` 按**每次调用**计数，与 `NullWorldRenderer` 同口径。
  std::snprintf(buf, sizeof(buf),
                "[godot] done: frames=%d ups=%.0f digest=%s warns=%d errors=%d ignored_images=%lld "
                "renderer(+-)=%lld/%lld clock=%.1fms residual=%.1fms steps=%lld",
                _frames, 1000.0 / _host->step_ms(), digest.c_str(), _host->warn_count(),
                _host->error_count(), _host->ignored_images(),
                renderer != nullptr ? renderer->added() : 0,
                renderer != nullptr ? renderer->removed() : 0, _host->clock().now(),
                _host->residual_ms(), _host->steps());
  print_line(buf);
  // UI 渲染面（DESIGN D4）：与 `lfw_run` 同口径（本宿主也不装 `IRenderScheduler`）。
  std::snprintf(buf, sizeof(buf), "[godot] UI render calls: %lld (unhandled: %lld)",
                world.ui_render_calls(), world.ui_render_unhandled());
  print_line(buf);
  // 绘制面（§G2 第二半，本切片新增的观测量；不进三条摘要）：最后一帧的绘制项与纹理统计。
  if (renderer != nullptr) {
    const GodotFrame& f = renderer->take_frame();
    std::snprintf(
        buf, sizeof(buf),
        "[godot] render: bg=%s cam=%.1f,%.1f scale=%.2f,%.2f df=%.3f items=%zu bg_layers=%d "
        "ents=%d hidden=%d no_pic=%d no_file=%d unnamed_tex=%d outline=%d[%s] textures=%lld ok/%lld "
        "fail",
        f.bg_id.c_str(), f.cam_x, f.cam_y, f.scale_x, f.scale_y, f.dfactor, f.items.size(),
        f.bg_layers, f.drawn_entities, f.hidden_entities, f.no_pic_entities, f.no_file_entities,
        f.unnamed_textures, f.outline_items, f.outline_probe.c_str(),
        _textures != nullptr ? _textures->loaded() : 0,
        _textures != nullptr ? _textures->failed() : 0);
    print_line(buf);
    if (_textures != nullptr && !_textures->last_error().empty()) {
      print_line("[godot] last texture error: " + _textures->last_error());
    }
  }
  quit_with(0);
}

void LfwGodot::quit_with(int code) const {
  godot::SceneTree* const tree = get_tree();
  if (tree != nullptr) tree->quit(code);
}

void LfwGodot::_bind_methods() {}

}  // namespace lfw::host
