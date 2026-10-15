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

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_pan_gesture.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include "host/common/key_code.h"
#include "host/common/lfw_boot.h"
#include "host/common/ui_input_handle.h"
#include "lfw/base/clock.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/ui/uilayer.h"
#include "lfw/utils/container_help/field_or.h"
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

const char* titlebar_name(LfwGodot::TitlebarMode mode) {
  switch (mode) {
    case LfwGodot::TitlebarMode::native:
      return "native";
    case LfwGodot::TitlebarMode::none:
      return "none";
    case LfwGodot::TitlebarMode::custom:
      return "custom";
  }
  return "?";
}

// 自绘栏判出来的方向 → `DisplayServer::window_start_resize` 要的枚举（两者都是 0..7 的 8 个方向，
// 但顺序不同 ⇒ 老实翻一遍）。
godot::DisplayServer::WindowResizeEdge to_window_edge(TitleEdge edge) {
  switch (edge) {
    case TitleEdge::top_left:
      return godot::DisplayServer::WINDOW_EDGE_TOP_LEFT;
    case TitleEdge::top:
      return godot::DisplayServer::WINDOW_EDGE_TOP;
    case TitleEdge::top_right:
      return godot::DisplayServer::WINDOW_EDGE_TOP_RIGHT;
    case TitleEdge::left:
      return godot::DisplayServer::WINDOW_EDGE_LEFT;
    case TitleEdge::right:
      return godot::DisplayServer::WINDOW_EDGE_RIGHT;
    case TitleEdge::bottom_left:
      return godot::DisplayServer::WINDOW_EDGE_BOTTOM_LEFT;
    case TitleEdge::bottom:
      return godot::DisplayServer::WINDOW_EDGE_BOTTOM;
    case TitleEdge::bottom_right:
      return godot::DisplayServer::WINDOW_EDGE_BOTTOM_RIGHT;
    case TitleEdge::none:
      break;
  }
  return godot::DisplayServer::WINDOW_EDGE_TOP;
}

// 一行窗口状态：标题栏按钮**动作之后**打出来 —— 这是"按下真的改变了窗口"的证据（模式/尺寸/装饰）。
std::string window_state_text(const godot::DisplayServer* ds) {
  const godot::Vector2i size = ds->window_get_size();
  const godot::Vector2i deco = ds->window_get_size_with_decorations();
  char buf[128];
  std::snprintf(buf, sizeof(buf), "mode=%d size=%dx%d deco=%dx%d",
                static_cast<int>(ds->window_get_mode()), size.x, size.y, deco.x, deco.y);
  return buf;
}

}  // namespace

const char* LfwGodot::usage() {
  return "usage: <godot> --path native/host/godot/project -- --root <dir> --zip <dir>\n"
         "               [--zip <dir> ...] [--fighter <id>] [--stage <id>] [--players N]\n"
         "               [--frames N] [--every N] [--time ms] [--delta seconds] [--vs]\n"
         "               [--play] [--click <x>,<y>[@<frame>]] [--click-window <x>,<y>[@<frame>]]\n"
         "               [--click-frame N] [--titlebar native|none|custom] [--window-scale N]\n"
         "               [--key <name>[@<frame>]] [--key-down <name>[@<frame>]]\n"
         "               [--key-up <name>[@<frame>]] [--key-frame N] [--key-tap N]\n"
         "               [--screenshot <file.png>] [--shot-frame N] [--ui-dump] [--ui-page <id>]\n"
         "  --root     import 根目录（含 builtin_data/…），可重复；缺省 `.`\n"
         "  --zip      数据包目录（已解包），可重复；按给定顺序 load\n"
         "  --fighter  指定出场的角色 id，可重复（缺省用数据包里的前 `--players` 个）\n"
         "  --play     开放模式：不 `boot_scene`（由数据入口页 `init` 驱动）、不限帧数（关窗口才\n"
         "             结束）、缺省不打帧摘要、节拍用 Godot 的真 `delta`。= `--frames 0` + 不造场景。\n"
         "  --frames   推这么多逻辑帧就收工并打摘要（验收模式）；`--frames 0` = 不限帧数\n"
         "  --delta    只覆盖 `_process` 的节拍（秒），不改固定步长（默认用 Godot 给的 delta）\n"
         "  --click    在第 `--click-frame` 帧（或 `@frame`）注入一次完整点击（按下 + 抬起）；\n"
         "             坐标是 UI 逻辑像素（左上原点、794×450 那套，与截图 1:1）。可重复。\n"
         "  --click-window  同上，但坐标是**窗口像素**：走 `push_input(ev, local=false)` ⇒ 引擎按\n"
         "             窗口缩放反算回逻辑像素（与 OS 点击同一条路）—— 证「换算对不对」用这条。\n"
         "  --key          在第 `--key-frame` 帧（或 `@frame`）按一下（按下 + `--key-tap` 帧后抬起），\n"
         "             走 `HeadlessHost::key_down/key_up`（与真键盘的 `_input` 同一条链）；\n"
         "             键名取小写 `KeyboardEvent.key`（`space` / `esc` / `up` / `j`…）。可重复。\n"
         "  --key-down/--key-up  只按下 / 只抬起（要「按住」就用这一对）。\n"
         "  --key-frame    没写 `@frame` 的按键落在哪一帧（缺省 60）\n"
         "  --key-tap      「按一下」的按下 → 抬起间隔（帧，缺省 2）\n"
         "  --titlebar 窗口那一套：native（系统边框）/ none（无边框、不画栏）/ custom（无边框 +\n"
         "             自绘栏：拖拽区 + 最小化/最大化/关闭）。缺省：`--play` 时 custom，其余 native。\n"
         "  --window-scale  窗口 = 逻辑视口（794×450）× N；逻辑视口不变（`project.godot` 的 stretch）。\n"
         "             缺省：`--play` 时 1.5（对齐 Electron 的 1191×675），其余 1.0。\n"
         "  --screenshot  画到第 `--shot-frame` 帧后存一张 PNG（**别加 --headless**：headless\n"
         "                没有渲染，存出来是空白）；给绝对路径\n"
         "  --shot-frame  截图的帧号（缺省 120）\n"
         "  --ui-dump     最后一帧逐 UI 节点打一行事实（诊断，不进摘要）\n"
         "  --ui-page     装载后把这一页设到第 0 层（id 取自烹饪好的 UI 表；缺省用首页 `init`）\n";
}

// `--click <x>,<y>[@<frame>]`：坐标取 `Number(...)` 口径（与 `--pos` 的解析同源）。
bool parse_click_spec(const std::string& text, int default_frame, double& x, double& y, int& frame) {
  const size_t at = text.find('@');
  const std::string pos = at == std::string::npos ? text : text.substr(0, at);
  const size_t comma = pos.find(',');
  if (comma == std::string::npos) return false;
  x = std::strtod(pos.substr(0, comma).c_str(), nullptr);
  y = std::strtod(pos.substr(comma + 1).c_str(), nullptr);
  if (!std::isfinite(x) || !std::isfinite(y)) return false;
  frame = default_frame;
  if (at != std::string::npos) frame = std::atoi(text.substr(at + 1).c_str());
  return frame >= 0;
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
    } else if (a == "--play") {
      out.play = true;
      out.frames = 0;  // 开放模式：帧数不限（窗口关掉才结束）
    } else if (a == "--click") {
      if (!need(i, v)) return false;
      out.click_specs.push_back(v);
    } else if (a == "--click-window") {
      if (!need(i, v)) return false;
      out.window_click_specs.push_back(v);
    } else if (a == "--titlebar") {
      if (!need(i, v)) return false;
      if (v == "native") {
        out.titlebar = TitlebarMode::native;
      } else if (v == "none") {
        out.titlebar = TitlebarMode::none;
      } else if (v == "custom") {
        out.titlebar = TitlebarMode::custom;
      } else {
        print_error("[godot] bad --titlebar value: " + v + " (want native|none|custom)");
        return false;
      }
      out.titlebar_given = true;
    } else if (a == "--window-scale") {
      if (!need(i, v)) return false;
      out.window_scale = std::atof(v.c_str());
      if (!(out.window_scale > 0.0) || out.window_scale > 8.0) {
        print_error("[godot] bad --window-scale value: " + v + " (want 0 < N <= 8)");
        return false;
      }
      out.window_scale_given = true;
    } else if (a == "--click-frame") {
      if (!need(i, v)) return false;
      out.click_frame = std::atoi(v.c_str());
      if (out.click_frame < 0) return false;
    } else if (a == "--key" || a == "--key-down" || a == "--key-up") {
      if (!need(i, v)) return false;
      out.key_specs.push_back(Args::KeySpec{
          a == "--key" ? "tap" : (a == "--key-down" ? "down" : "up"), v});
    } else if (a == "--key-frame") {
      if (!need(i, v)) return false;
      out.key_frame = std::atoi(v.c_str());
      if (out.key_frame < 0) return false;
    } else if (a == "--key-tap") {
      if (!need(i, v)) return false;
      out.key_tap = std::atoi(v.c_str());
      if (out.key_tap < 1) return false;
    } else if (a == "--ui-dump") {
      out.ui_dump = true;
    } else if (a == "--ui-page") {
      if (!need(i, v)) return false;
      out.ui_page = from_utf8(v);
    } else {
      return false;
    }
  }
  if (out.roots.empty()) out.roots.emplace_back(".");
  // 窗口那一套的缺省按**跑法**定（与"玩/验收"这条岔路口一致）：`--play` = Electron 那一套
  //（无边框 + 自绘栏 + 1.5× 窗口），其余（`--frames` 验收、headless、截图）原样不动。
  if (!out.titlebar_given) out.titlebar = out.play ? TitlebarMode::custom : TitlebarMode::native;
  if (!out.window_scale_given) out.window_scale = out.play ? 1.5 : 1.0;
  // 0 = 自动：开放模式缺省静默（不打帧摘要），验收模式缺省每 60 帧一行。
  if (out.every <= 0) out.every = out.play ? 0 : 60;
  if (!out.screenshot.empty() && out.shot_frame < 0) out.shot_frame = 120;
  if (out.frames < 0) return false;
  for (const std::string& spec : out.click_specs) {
    Args::ClickAt click;
    if (!parse_click_spec(spec, out.click_frame, click.x, click.y, click.frame)) {
      print_error("[godot] bad --click value: " + spec + " (want <x>,<y>[@<frame>])");
      return false;
    }
    out.clicks.push_back(click);
  }
  for (const std::string& spec : out.window_click_specs) {
    Args::ClickAt click;
    if (!parse_click_spec(spec, out.click_frame, click.x, click.y, click.frame)) {
      print_error("[godot] bad --click-window value: " + spec + " (want <x>,<y>[@<frame>])");
      return false;
    }
    click.window_space = true;
    out.clicks.push_back(click);
  }
  for (const Args::KeySpec& spec : out.key_specs) {
    const size_t at = spec.text.find('@');
    const std::string name = at == std::string::npos ? spec.text : spec.text.substr(0, at);
    int frame = out.key_frame;
    if (at != std::string::npos) frame = std::atoi(spec.text.substr(at + 1).c_str());
    if (name.empty() || frame < 0) {
      print_error("[godot] bad --key value: " + spec.text + " (want <name>[@<frame>])");
      return false;
    }
    const std::u16string key = dom_key_of_name(name);
    if (spec.kind == "tap") {
      out.keys.push_back(Args::KeyAt{frame, true, key});
      out.keys.push_back(Args::KeyAt{frame + out.key_tap, false, key});
    } else {
      out.keys.push_back(Args::KeyAt{frame, spec.kind == "down", key});
    }
  }
  std::stable_sort(out.keys.begin(), out.keys.end(),
                   [](const Args::KeyAt& a, const Args::KeyAt& b) { return a.frame < b.frame; });
  return !out.zips.empty() && (out.play || out.frames > 0);
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
  // 窗口放在装包**之前**：无边框要在第一帧之前就设上（不然系统边框会闪一下），而且它不依赖数据。
  apply_window_setup();
  if (!setup()) {
    quit_with(1);
    return;
  }
  // ⚠ GDExtension 必须显式打开输入处理，不然引擎不会调 `_input`（默认关）。
  set_process_input(true);
  _running = true;
}

bool LfwGodot::setup() {
  _host = std::make_unique<LfwGodotHost>(_args.roots, false);
  // ⚠ 文字度量必须在 `LFW::load` **之前**装好：UI 数据（`cook_ui_info` 的 `txt_info`）在装载期
  // 就按度量定尺寸（`lfw.host().measure_text`），装晚了整页的文本节点都是 0 尺寸。
  install_text_metrics();
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
  // ⚠ 开放模式（`--play`）**不**走这里：那是"玩"，入口是数据自己的首页 `init`（启动页 → 点
  // 进去 → 菜单 → …），宿主不替玩家把场景摆好。验收模式（`--frames N`）才强制造场景。
  if (_args.play) {
    print_line("[godot] play mode: no boot_scene; the entry UI page drives (entry page: init)");
  } else {
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
    char buf[320];
    std::snprintf(buf, sizeof(buf), "[godot] stage=%s bg=%s entities=%zu",
                  to_utf8(booted.stage_id).c_str(), to_utf8(booted.bg_id).c_str(),
                  _lfw->world().entities.size());
    print_line(buf);
  }

  // ---- `--ui-page <id>`：装载后把这一页设到第 0 层（与 TS 里"点进这一页"等价的一次
  // `UILayers.set_page`；不传就用数据自己的首页 `init`）。id 必须真在烹饪好的 UI 表里 ——
  // 否则 `set_page` 会静默建一个空页（`UILayer::create_page` 找不到 id 时回 nullptr）。
  if (!_args.ui_page.empty()) {
    bool found = false;
    for (const Value& v : _lfw->ui_helper().all()) {
      const Value id = lfw::field_or(v, u"id");
      const std::u16string* const s = std::get_if<std::u16string>(&id);
      if (s != nullptr && *s == _args.ui_page) {
        found = true;
        break;
      }
    }
    if (!found) {
      print_error("[godot] --ui-page: no such page id: " + to_utf8(_args.ui_page));
      return false;
    }
    auto opts = std::make_shared<lfw::Object>();
    opts->set(u"id", Value(_args.ui_page));
    _lfw->set_page(Value(std::move(opts)), 0.0);
    print_line("[godot] ui page: " + to_utf8(_args.ui_page));
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
  if (_host->world_renderer() != nullptr) _host->world_renderer()->set_ui_dump(_args.ui_dump);
  return true;
}

// 文字度量：D4 把字体度量划给宿主（"允许宿主差异"），这一份用**引擎自己的默认字体**算。
// 口径与 TS 的 `ImageMgr.measure_text` 对齐：返回 `{text, style, w, h, scale}`，核心的
// `auto_size_by_text` 读 `w/h/scale` 定节点尺寸（`uinode.cpp:193-203`）。
// ⚠ 字体与浏览器不同（TS 是 canvas 的 Arial）⇒ **同一台机器上稳定、与 TS 不逐位同宽**；
// 取不到字体时落回零尺寸（等于 headless 桩）。
void LfwGodot::install_text_metrics() {
  if (_host == nullptr) return;
  godot::ThemeDB* const db = godot::ThemeDB::get_singleton();
  if (db != nullptr) _font = db->get_fallback_font();
  if (_font.is_null()) {
    print_line("[godot] ui text metrics: no fallback font (零尺寸桩)");
  } else {
    char buf[160];
    std::snprintf(buf, sizeof(buf), "[godot] ui text metrics: engine fallback font (%s)",
                  _font->get_font_name().utf8().get_data());
    print_line(buf);
  }
  _host->set_measure_text([this](const Value& text_v, const Value& style_v) -> Value {
    const std::u16string s = lfw::to_string(text_v);
    const std::string utf8 = to_utf8(s);
    double font_size = 9.0;
    const lfw::Object* const style = lfw::as_object(style_v);
    if (style != nullptr) {
      const Value* const font = style->get(u"font");
      if (font != nullptr) {
        const std::u16string* const f = std::get_if<std::u16string>(font);
        if (f != nullptr) {
          const std::string fs = to_utf8(*f);
          for (std::size_t i = 0; i + 1 < fs.size(); ++i) {
            if (fs[i] != 'p' || fs[i + 1] != 'x') continue;
            std::size_t k = i;
            while (k > 0 && (std::isdigit(static_cast<unsigned char>(fs[k - 1])) != 0 ||
                             fs[k - 1] == '.')) {
              --k;
            }
            if (k == i) continue;
            const double n = std::strtod(fs.substr(k, i - k).c_str(), nullptr);
            if (n > 0.0) font_size = n;
            break;
          }
        }
      }
    }
    double w = 0.0;
    double h = 0.0;
    if (!_font.is_null()) {
      const int size = static_cast<int>(std::lround(font_size));
      const int px = size > 0 ? size : 1;
      // 逐行：`w` 取最宽的一行、`h` 累加（TS `split_text_to_lines` 同一口径）。
      std::size_t start = 0;
      int lines = 0;
      while (true) {
        const std::size_t nl = utf8.find('\n', start);
        const std::string line = utf8.substr(start, nl == std::string::npos ? std::string::npos
                                                                            : nl - start);
        const godot::Vector2 sz = _font->get_string_size(String::utf8(line.c_str()),
                                                        godot::HORIZONTAL_ALIGNMENT_LEFT, -1.0f, px);
        if (static_cast<double>(sz.x) > w) w = static_cast<double>(sz.x);
        ++lines;
        if (nl == std::string::npos) break;
        start = nl + 1;
      }
      h = static_cast<double>(_font->get_height(px)) * static_cast<double>(lines);
    }
    auto info = std::make_shared<lfw::Object>();
    info->set(u"text", text_v);
    info->set(u"style", style_v);
    info->set(u"w", Value(w));
    info->set(u"h", Value(h));
    info->set(u"scale", Value(1.0));
    return Value(info);
  });
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
  while (_running && (unlimited() || _frames < _args.frames) && _host->can_step()) {
    GodotWorldRenderer* const renderer = _host->world_renderer();
    const long long before = renderer != nullptr ? renderer->renders() : 0;
    const int frame = _frames;
    // `--click`：命令先入队，接着这一步的 `World::step` 消化掉 ⇒ 打完这一帧再报页面。
    Args::ClickAt* const clicked = inject_clicks(frame);
    const Args::KeyAt* const keyed = inject_keys(frame);
    _host->push_frame(_lfw->world());
    render_world_frame(before);
    stepped = true;
    ++_frames;
    if (clicked != nullptr) {
      char buf[256];
      std::snprintf(buf, sizeof(buf), "[godot] %s @frame %d at=(%.1f, %.1f) pages=%s",
                    clicked->window_space ? "click-win" : "click", frame, clicked->x, clicked->y,
                    ui_pages_text().c_str());
      print_line(buf);
    }
    if (keyed != nullptr) {
      const auto pressed = _host->pressed_keys().find(keyed->key);
      char buf[256];
      std::snprintf(buf, sizeof(buf), "[godot] key @frame %d %s %s times=%d pages=%s", frame,
                    keyed->down ? "down" : "up", to_utf8(keyed->key).c_str(),
                    pressed == _host->pressed_keys().end() ? 0 : pressed->second,
                    ui_pages_text().c_str());
      print_line(buf);
    }
    if (_args.every > 0 && (frame % _args.every == 0 || frame == _args.frames - 1)) {
      report_frame(frame);
    }
    if (shot_at >= 0 && _frames >= shot_at) {
      _capture_pending = true;  // 这一帧的绘制落在本帧末尾 ⇒ 下一帧再取视口
      break;
    }
  }
  if (stepped) queue_redraw();
  if (_capture_pending) return;
  if (!unlimited() && _frames >= _args.frames) finish();
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

// ---- 指针：平台事件 → `LfwPointingEvent` → `POINTER_*` 命令 ----
//
// 坐标：`_draw()` 直接按**逻辑屏幕像素**画（`GodotUINodeRenderer` 的 `_box_*`，左上原点、y 向下），
// 而窗口/视口就是 794×450（`project/project.godot` 的 `display/window/size/viewport_*`，没有 stretch）
// ⇒ 「视口像素 = UI 逻辑像素」，`scene(NDC)` 由 `ui_scene_of_point` 反算（与 `UiInputHandle`
// 的 `to_ui` 互逆：`sx = x/w*2-1`、`sy = -(y/h)*2+1`）。控件（`ScrollView` 等）自己也是按这两条
// 公式从 `scene_x/scene_y` 反算的，所以对得上。
void LfwGodot::ui_screen_size(double& w, double& h) const {
  const lfw::host::UiScreenSize s = lfw::host::ui_screen_size(_lfw->world());
  w = s.w;
  h = s.h;
}

// ---- 窗口：无边框 + 自绘标题栏 + 缩放（Electron 端那一套的引擎侧等价物）----
//
// 对位 `desktop/app/main.mjs:915` 的 `frame: false`、`src/App.tsx:788-811` 自绘的那三个按钮，以及
// 那边 `1191×675 = 794×450 × 1.5` 的窗口。逻辑视口**永远是** 794×450（`project.godot` 的
// `display/window/stretch/*` = `canvas_items`）⇒ 缩放全在引擎那一侧：`_draw()` 照样按逻辑像素画，
// OS 的窗口像素由引擎按 `Window::get_final_transform()`（`= window_transform * stretch_transform`）
// 的反变换换算回逻辑像素后才进 `_input`（`--click-window` 走的就是同一条）⇒ 宿主不做缩放补偿。
void LfwGodot::apply_window_setup() {
  using godot::DisplayServer;
  DisplayServer* const ds = DisplayServer::get_singleton();
  if (ds == nullptr) return;
  const std::string display(ds->get_name().utf8().get_data());
  // headless 没有窗口（也就没有边框/装饰/缩放）：一个窗口 API 都不碰（省得引擎打 warn），
  // 栏也不画 ⇒ 三条摘要那一路完全没变。
  if (display == "headless") {
    print_line("[godot] window: headless display server (no window, no titlebar)");
    return;
  }
  _windowed = true;
  if (_args.titlebar != TitlebarMode::native) {
    ds->window_set_flag(DisplayServer::WINDOW_FLAG_BORDERLESS, true);
  }
  // 窗口 = **逻辑视口** × `--window-scale`。逻辑视口从视口自己读（`project.godot` 的
  // `window/size/viewport_*` ⇒ stretch 的 `content_scale_size`），**不**读 `window_get_size()`：
  // 刚把系统边框摘掉那一刻客户区还带着原来的非客户区尺寸（实测 794×450 的窗口报 810×489）⇒
  // 按它缩放会把窗口放大成 1.53×/1.63×、比例也歪掉（内容就被 letterbox 了）。
  const godot::Viewport* const viewport = get_viewport();
  const godot::Vector2i logical =
      viewport != nullptr ? godot::Vector2i(viewport->get_visible_rect().size) : ds->window_get_size();
  const double scale = _args.window_scale;
  const godot::Vector2i want(static_cast<int>(std::lround(static_cast<double>(logical.x) * scale)),
                            static_cast<int>(std::lround(static_cast<double>(logical.y) * scale)));
  if (ds->window_get_size() != want) {
    ds->window_set_size(want);
    if (scale != 1.0) {
      // 窗口从左上角长大 ⇒ 放大后自己居中一下（不然会往右下溢出屏幕）。
      const int screen = ds->window_get_current_screen();
      const godot::Vector2i screen_pos = ds->screen_get_position(screen);
      const godot::Vector2i screen_size = ds->screen_get_size(screen);
      ds->window_set_position(screen_pos + (screen_size - want) / 2);
    }
  }
  // 证据行（`native/README.md` 的「窗口」一节引的就是它）：`borderless=1` = 无边框真的设上了；
  // `deco=size` = 没有非客户区（无边框窗口的装饰矩形与客户区矩形重合；有系统边框时 deco > size）；
  // `logical=794x450` = 逻辑视口没被窗口尺寸带跑（stretch 的 content_scale_size）。
  const godot::Vector2i now = ds->window_get_size();
  const godot::Vector2i deco = ds->window_get_size_with_decorations();
  const godot::Vector2i pos = ds->window_get_position();
  char buf[384];
  std::snprintf(buf, sizeof(buf),
                "[godot] window: display=%s titlebar=%s scale=%.2f borderless=%d size=%dx%d "
                "deco=%dx%d pos=%d,%d mode=%d logical=%dx%d",
                display.c_str(), titlebar_name(_args.titlebar), scale,
                ds->window_get_flag(DisplayServer::WINDOW_FLAG_BORDERLESS) ? 1 : 0, now.x, now.y,
                deco.x, deco.y, pos.x, pos.y, static_cast<int>(ds->window_get_mode()), logical.x,
                logical.y);
  print_line(buf);
}

// 自绘栏只在 `--titlebar custom`、真窗口、且**不是全屏**时存在（镜像 Electron：全屏时整簇按钮
// `show={!is_fullscreen && …}` 消失）。栏没了 ⇒ 条上也不吞点击，全屏的指针全给游戏。
bool LfwGodot::titlebar_visible() const {
  if (!_windowed || _args.titlebar != TitlebarMode::custom) return false;
  godot::DisplayServer* const ds = godot::DisplayServer::get_singleton();
  if (ds == nullptr) return false;
  const godot::DisplayServer::WindowMode mode = ds->window_get_mode();
  return mode != godot::DisplayServer::WINDOW_MODE_FULLSCREEN &&
         mode != godot::DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
}

// 条上与窗口边缘的事件处理。返回 true = 宿主吃掉（调用方标 handled，别再喂给游戏/UI）。
//
// 次序：① 边缘缩放带（窗口像素，压过按钮 —— 与原生边框同口径）→ ② 条内（按钮动作 / 拖拽 / 一律吞）
// → ③ 起手在条上、现在跑到条外的 move/up 也吞（免得松手那一下点着游戏里的按钮）。
bool LfwGodot::titlebar_consume(const godot::Ref<godot::InputEvent>& event) {
  if (!titlebar_visible()) return false;
  LfwPointingEvent e;
  const PointerKind kind = to_pointing_event(event, e);
  if (kind == PointerKind::none) return false;
  godot::DisplayServer* const ds = godot::DisplayServer::get_singleton();
  if (ds == nullptr) return false;
  double screen_w = 794;
  double screen_h = 450;
  ui_screen_size(screen_w, screen_h);

  if (kind == PointerKind::down && e.button == 0.0 &&
      ds->window_get_mode() == godot::DisplayServer::WINDOW_MODE_WINDOWED &&
      !ds->window_get_flag(godot::DisplayServer::WINDOW_FLAG_RESIZE_DISABLED)) {
    const godot::Viewport* const vp = get_viewport();
    const godot::Vector2 local(static_cast<float>(e.x), static_cast<float>(e.y));
    const godot::Vector2 win = vp != nullptr ? vp->get_final_transform().xform(local) : local;
    const godot::Vector2i win_size = ds->window_get_size();
    const TitleEdge edge = titlebar_edge_at(win.x, win.y, win_size.x, win_size.y);
    if (edge != TitleEdge::none) {
      char buf[128];
      std::snprintf(buf, sizeof(buf), "[godot] titlebar: resize edge=%d at=(%.0f, %.0f)",
                    static_cast<int>(edge), win.x, win.y);
      print_line(buf);
      _bar_capture = true;  // 嵌套的尺寸循环里那些 move/up 也别漏给游戏
      ds->window_start_resize(to_window_edge(edge));  // 阻塞到松开（Godot 自己的边框同款）
      return true;
    }
  }

  if (titlebar_in_strip(e.x, e.y, screen_w)) {
    if (kind == PointerKind::down) {
      _bar_capture = true;
    } else if (kind == PointerKind::up) {
      _bar_capture = false;
    }
    if (kind == PointerKind::move) {
      const TitleAction hover = titlebar_action_at(e.x, e.y, screen_w);
      const bool hover_drag = hover == TitleAction::none;
      if (hover != _bar_hover || hover_drag != _bar_hover_drag) {
        _bar_hover = hover;
        _bar_hover_drag = hover_drag;
        queue_redraw();
      }
      return true;
    }
    if (kind == PointerKind::down && e.button == 0.0) {
      switch (titlebar_action_at(e.x, e.y, screen_w)) {
        case TitleAction::minimize:
          ds->window_set_mode(godot::DisplayServer::WINDOW_MODE_MINIMIZED);
          print_line("[godot] titlebar: minimize (" + window_state_text(ds) + ")");
          break;
        case TitleAction::maximize: {
          const bool maximized =
              ds->window_get_mode() == godot::DisplayServer::WINDOW_MODE_MAXIMIZED;
          ds->window_set_mode(maximized ? godot::DisplayServer::WINDOW_MODE_WINDOWED
                                        : godot::DisplayServer::WINDOW_MODE_MAXIMIZED);
          print_line(std::string("[godot] titlebar: ") + (maximized ? "restore (" : "maximize (") +
                     window_state_text(ds) + ")");
          break;
        }
        case TitleAction::close:
          print_line("[godot] titlebar: close");
          print_exit_summary();
          quit_with(0);
          break;
        case TitleAction::none:
          // 拖拽区。`window_start_drag` 在 Windows 上就是
          // `SendMessage(WM_SYSCOMMAND, SC_MOVE | HTCAPTION)` ⇒ 这一段**阻塞**到松开为止
          //（Godot 自己的标题栏同款）⇒ 重入用 `_bar_dragging` 挡掉。
          if (!_bar_dragging) {
            _bar_dragging = true;
            print_line("[godot] titlebar: drag");
            ds->window_start_drag();
            _bar_dragging = false;
          }
          break;
      }
    }
    return true;
  }
  if (_bar_capture) {
    if (kind == PointerKind::up) _bar_capture = false;
    return true;
  }
  return false;
}

// 自绘栏的画面：一条半透明底 + 右侧三个按钮（最小化=横线 / 最大化=方框 / 关闭=叉）。
// 拖拽区悬停时整条亮一点（Electron 的 `-webkit-app-region: drag` hover `#88888855`），
// 按钮悬停时给自己加一块底。⚠ 只是"能用"的程度：没有用 Electron 那几张 `btn_*.png`（它们在 TS 的
// `src/assets/` 里，不在数据包里），也不做圆角/阴影/主题。
void LfwGodot::draw_titlebar() {
  double screen_w = 794;
  double screen_h = 450;
  ui_screen_size(screen_w, screen_h);
  const float w = static_cast<float>(screen_w);
  const float h = static_cast<float>(TitleBarMetrics::height);
  draw_rect(Rect2(0.0f, 0.0f, w, h),
            _bar_hover_drag ? Color(0.10f, 0.12f, 0.17f, 0.88f) : Color(0.08f, 0.10f, 0.14f, 0.72f),
            true);
  draw_rect(Rect2(0.0f, h - 1.0f, w, 1.0f), Color(1.0f, 1.0f, 1.0f, 0.12f), true);
  TitleBarButton buttons[TitleBarMetrics::count];
  titlebar_layout(screen_w, buttons);
  const Color fg(0.84f, 0.87f, 0.94f, 1.0f);
  for (const TitleBarButton& button : buttons) {
    const float x = static_cast<float>(button.rect.x);
    const float y = static_cast<float>(button.rect.y);
    const float bw = static_cast<float>(button.rect.w);
    const float bh = static_cast<float>(button.rect.h);
    if (button.action == _bar_hover) {
      draw_rect(Rect2(x, y, bw, bh), Color(1.0f, 1.0f, 1.0f, 0.16f), true);
    }
    const float cx = x + bw * 0.5f;
    const float cy = y + bh * 0.5f;
    switch (button.action) {
      case TitleAction::minimize:
        draw_line(Vector2(cx - 5.0f, cy + 4.0f), Vector2(cx + 5.0f, cy + 4.0f), fg, 1.0f);
        break;
      case TitleAction::maximize:
        draw_rect(Rect2(cx - 4.5f, cy - 4.5f, 9.0f, 9.0f), fg, false, 1.0f);
        break;
      case TitleAction::close:
        draw_line(Vector2(cx - 4.0f, cy - 4.0f), Vector2(cx + 4.0f, cy + 4.0f), fg, 1.0f);
        draw_line(Vector2(cx + 4.0f, cy - 4.0f), Vector2(cx - 4.0f, cy + 4.0f), fg, 1.0f);
        break;
      case TitleAction::none:
        break;
    }
  }
}

// `--click-window`：把一次完整点击按**窗口像素**推进引擎的真实事件路径
//（`Viewport::push_input(ev, /*local=*/false)` ⇒ `_make_input_local` 拿 `Window::get_final_transform()`
// 的反变换，正是 OS 事件走的那一条）⇒ 它证明的是「窗口像素 → 逻辑像素」这条换算，不只是"注入"。
void LfwGodot::push_window_click(double x, double y) {
  godot::Viewport* const vp = get_viewport();
  if (vp == nullptr) return;
  for (int pass = 0; pass < 2; ++pass) {
    godot::Ref<godot::InputEventMouseButton> mb;
    mb.instantiate();
    mb->set_button_index(godot::MOUSE_BUTTON_LEFT);
    mb->set_pressed(pass == 0);
    mb->set_position(Vector2(static_cast<float>(x), static_cast<float>(y)));
    mb->set_global_position(mb->get_position());
    vp->push_input(mb, false);
  }
}

LfwGodot::PointerKind LfwGodot::to_pointing_event(const godot::Ref<godot::InputEvent>& event,
                                                  LfwPointingEvent& out) const {
  double w = 794, h = 450;
  ui_screen_size(w, h);

  const godot::Ref<godot::InputEventPanGesture> pan = event;
  if (pan.is_valid()) {
    // 触控板双指滚动：Godot 的 `delta` 是"视口尺寸的比例"，**正 = 上/左**
    //（与浏览器的 `deltaY` 反号）⇒ 这里取负并乘回逻辑尺寸，落到与滚轮同一套 UI 像素口径。
    const godot::Vector2 d = pan->get_delta();
    out.delta_x = -static_cast<double>(d.x) * w;
    out.delta_y = -static_cast<double>(d.y) * h;
    out.button = 0;
    return PointerKind::wheel;
  }

  const godot::Ref<godot::InputEventMouseButton> mb = event;
  if (mb.is_valid()) {
    const godot::Vector2 pos = mb->get_position();
    out.x = static_cast<double>(pos.x);
    out.y = static_cast<double>(pos.y);
    out.delta_x = 0;
    out.delta_y = 0;
    const int64_t idx = static_cast<int64_t>(mb->get_button_index());
    // 滚轮：浏览器口径（一个齿 ≈ 100 DOM_DELTA_PIXEL），符号按 Godot 的方向 ——
    // 上 = 内容往上走（浏览器 `deltaY < 0`），下 = 反之；左右同理。
    if (idx == godot::MOUSE_BUTTON_WHEEL_UP || idx == godot::MOUSE_BUTTON_WHEEL_DOWN ||
        idx == godot::MOUSE_BUTTON_WHEEL_LEFT || idx == godot::MOUSE_BUTTON_WHEEL_RIGHT) {
      if (!mb->is_pressed()) return PointerKind::none;  // 齿只有按下那一下算
      const double factor = std::fabs(static_cast<double>(mb->get_factor()));
      const double notch = 100.0 * (factor > 0.0 ? factor : 1.0);
      if (idx == godot::MOUSE_BUTTON_WHEEL_UP) {
        out.delta_y = -notch;
      } else if (idx == godot::MOUSE_BUTTON_WHEEL_DOWN) {
        out.delta_y = notch;
      } else if (idx == godot::MOUSE_BUTTON_WHEEL_LEFT) {
        out.delta_x = -notch;
      } else {
        out.delta_x = notch;
      }
      out.button = 0;  // TS `__PointingEvent`：滚轮的 `button` 是 0
      return PointerKind::wheel;
    }
    // 左/中/右：浏览器 `PointerEvent.button` 的 0/1/2（`--b` 就是这一套）。
    out.button = idx == godot::MOUSE_BUTTON_LEFT     ? 0.0
                 : idx == godot::MOUSE_BUTTON_MIDDLE ? 1.0
                 : idx == godot::MOUSE_BUTTON_RIGHT  ? 2.0
                                                     : -1.0;
    return mb->is_pressed() ? PointerKind::down : PointerKind::up;
  }

  const godot::Ref<godot::InputEventMouseMotion> mm = event;
  if (mm.is_valid()) {
    const godot::Vector2 pos = mm->get_position();
    out.x = static_cast<double>(pos.x);
    out.y = static_cast<double>(pos.y);
    // TS：非滚轮的 `delta_*` 是 0，移动事件的 `button` 是 -1（`mousemove` 的 DOM 口径）。
    out.delta_x = 0;
    out.delta_y = 0;
    out.button = -1;
    return PointerKind::move;
  }
  return PointerKind::none;
}

// Godot 的 `Key` → 真 `KeyboardEvent.key`（小写；口径见 `src/DittoImpl/Keyboard.ts` 的
// `e.key?.toLowerCase()`）。两档：① 控制键/编辑键这种「有名字」的按 DOM 名字给；
// ② 剩下的看 `get_unicode()`（引擎已经按当前键盘布局解出了字符）——`Shift+A` 给 "A"，宿主
// 照 TS 一样再 `.toLowerCase()`。取不到（如纯修饰键之外的死键）⇒ 空串（`LFW` 那边自然不匹配）。
std::u16string dom_key_of_godot(const godot::Ref<godot::InputEventKey>& key) {
  struct Entry {
    godot::Key code;
    const char16_t* name;
  };
  static const Entry kTable[] = {
      {godot::KEY_LEFT, u"arrowleft"},     {godot::KEY_RIGHT, u"arrowright"},
      {godot::KEY_UP, u"arrowup"},         {godot::KEY_DOWN, u"arrowdown"},
      {godot::KEY_ENTER, u"enter"},        {godot::KEY_KP_ENTER, u"enter"},
      {godot::KEY_ESCAPE, u"escape"},      {godot::KEY_TAB, u"tab"},
      {godot::KEY_BACKSPACE, u"backspace"},{godot::KEY_DELETE, u"delete"},
      {godot::KEY_INSERT, u"insert"},      {godot::KEY_HOME, u"home"},
      {godot::KEY_END, u"end"},            {godot::KEY_PAGEUP, u"pageup"},
      {godot::KEY_PAGEDOWN, u"pagedown"},  {godot::KEY_SHIFT, u"shift"},
      {godot::KEY_CTRL, u"control"},       {godot::KEY_ALT, u"alt"},
      {godot::KEY_META, u"meta"},          {godot::KEY_CAPSLOCK, u"capslock"},
      {godot::KEY_SPACE, u" "},            {godot::KEY_PERIOD, u"."},
      {godot::KEY_COMMA, u","},            {godot::KEY_SLASH, u"/"},
      {godot::KEY_SEMICOLON, u";"},        {godot::KEY_APOSTROPHE, u"'"},
      {godot::KEY_BRACKETLEFT, u"["},      {godot::KEY_BRACKETRIGHT, u"]"},
      {godot::KEY_BACKSLASH, u"\\"},       {godot::KEY_MINUS, u"-"},
      {godot::KEY_EQUAL, u"="},            {godot::KEY_QUOTELEFT, u"`"},
  };
  const godot::Key code = key->get_keycode();
  for (const Entry& entry : kTable) {
    if (code == entry.code) return entry.name;
  }
  // 功能键 F1..F12（Godot 的 `KEY_F1` 是连续的）。
  if (code >= godot::KEY_F1 && code <= godot::KEY_F12) {
    return u"f" + std::u16string(1, static_cast<char16_t>(u'1' + (code - godot::KEY_F1)));
  }
  // 其余看 `get_unicode()`（引擎已经按当前键盘布局解出了字符）：`Shift+A` 给 "A"，
  // 这里照 TS 的 `.toLowerCase()` 再落到小写。
  const char32_t unicode = key->get_unicode();
  if (unicode < 32 || unicode > 0x7e) return std::u16string();
  char16_t c = static_cast<char16_t>(unicode);
  if (c >= u'A' && c <= u'Z') c = static_cast<char16_t>(c - u'A' + u'a');
  return std::u16string(1, c);
}

// 平台事件进来的唯一入口。**只**推事件、不改模拟：命令入队后由下一步的 `World::step` 消化
//（与 `lfw_run --click` 同一条路径 ⇒ 无头脚本化点击能证明这条链是通的）。
// ⚠ 自绘标题栏那一带先吞（`titlebar_consume`）：条上/边缘的点击是**窗口**的动作，不许落到游戏/UI
//（端口现在指针是通的，一个走空的点击就会点到菜单按钮）。
void LfwGodot::_input(const godot::Ref<godot::InputEvent>& p_event) {
  if (!_running || _host == nullptr || _lfw == nullptr || p_event.is_null()) return;
  if (titlebar_consume(p_event)) {
    if (godot::Viewport* const vp = get_viewport()) vp->set_input_as_handled();
    return;
  }
  // 键盘：`InputEventKey` → `KeyboardEvent.key`（DOM 口径）→ `HeadlessHost::key_down/key_up`
  //（与 `--key` 注入、`lfw_run --key` 同一条链）。`echo` = 操作系统的重复按下 ⇒ 事件里的
  // `times` 递增（`LFW::on_key_down` 只在 `times == 0` 时把按下翻成 `KEY_EVENT`，重复不再翻）。
  if (const godot::Ref<godot::InputEventKey> key_event = p_event; !key_event.is_null()) {
    const std::u16string key = dom_key_of_godot(key_event);
    if (key.empty()) return;
    if (key_event->is_pressed()) {
      const bool first = _host->pressed_keys().find(key) == _host->pressed_keys().end();
      _host->key_down(key);
      if (first) print_line("[godot] key: down " + to_utf8(key));
    } else {
      _host->key_up(key);
      print_line("[godot] key: up " + to_utf8(key));
    }
    return;
  }
  LfwPointingEvent e;
  const PointerKind kind = to_pointing_event(p_event, e);
  if (kind == PointerKind::none) return;
  ui_scene_of_point(_lfw->world(), e.x, e.y, e.scene_x, e.scene_y);
  switch (kind) {
    case PointerKind::down:
      _host->emit_pointer_down(e);
      break;
    case PointerKind::up:
      _host->emit_pointer_up(e);
      break;
    case PointerKind::move:
      _host->emit_pointer_move(e);
      break;
    case PointerKind::wheel:
      _host->emit_wheel(e);
      break;
    case PointerKind::none:
      break;
  }
}

// 窗口关掉（或 `NOTIFICATION_WM_CLOSE_REQUEST`）时打一行收尾：开放模式没有帧数上限，
// 正常玩下去只会这样结束 —— 这一行就是「跑到第几帧、时钟走到哪」的证据。
// 自绘栏的 close 按钮走 `print_exit_summary()` + `quit_with(0)`（同一个口径的一行）。
void LfwGodot::print_exit_summary() const {
  if (!_running || _lfw == nullptr || _host == nullptr) return;
  char buf[256];
  std::snprintf(buf, sizeof(buf), "[godot] exit: frames=%d clock=%.0fms steps=%lld pages=%s",
                _frames, _host->clock().now(), _host->steps(), ui_pages_text().c_str());
  print_line(buf);
}

void LfwGodot::_notification(int p_what) {
  if (p_what != NOTIFICATION_WM_CLOSE_REQUEST) return;
  print_exit_summary();
}

LfwGodot::Args::ClickAt* LfwGodot::inject_clicks(int frame) {
  if (_args.clicks.empty() || _host == nullptr || _lfw == nullptr) return nullptr;
  Args::ClickAt* first = nullptr;
  for (Args::ClickAt& click : _args.clicks) {
    if (click.frame != frame) continue;
    if (click.window_space) {
      push_window_click(click.x, click.y);
    } else {
      _host->click_ui(*_lfw, click.x, click.y);
    }
    if (first == nullptr) first = &click;
  }
  return first;
}

// `--key` / `--key-down` / `--key-up`：把这一帧的按键喂进宿主。次序与 `inject_clicks` 一样 ——
// 事件先入队（`LFW::on_key_down` 变成 `KEY_EVENT` 命令），由**这一步**的 `World::step` 消化。
const LfwGodot::Args::KeyAt* LfwGodot::inject_keys(int frame) {
  if (_args.keys.empty() || _host == nullptr || _lfw == nullptr) return nullptr;
  const Args::KeyAt* first = nullptr;
  for (const Args::KeyAt& key : _args.keys) {
    if (key.frame != frame) continue;
    if (key.down) {
      _host->key_down(key.key);
    } else {
      _host->key_up(key.key);
    }
    if (first == nullptr) first = &key;
  }
  return first;
}

// UI 的层/页栈（`<层号>:<该层栈顶页 id>`）：点击有没有把页面推下去的**直接**证据。
std::string LfwGodot::ui_pages_text() const {
  lfw::ui::UILayers* const layers = _lfw->ui_layers();
  if (layers == nullptr) return "<no layers>";
  std::string out;
  for (lfw::ui::UILayer* const layer : layers->all()) {
    if (layer == nullptr) continue;
    if (!out.empty()) out += " ";
    out += to_utf8(lfw::number_to_string(layer->index()));
    out += ":";
    lfw::ui::UINode* const page = layer->ui();
    out += page != nullptr ? to_utf8(lfw::to_string(page->id())) : "-";
  }
  return out.empty() ? "<empty>" : out;
}

// 引擎侧唯一一处把 `GodotDrawItem` 变成像素的地方（§G5「只在边界转换」）。
//
// 三趟扫：`pass` -1 = UI 背景、0 = 世界、+1 = UI 前景 —— 这是 D1(a) 的五遍次序
//（UI 背景 → 背景/实体/前景 → UI 前景）。同一趟内保持收集次序（世界那一趟是画家次序，
// UI 那两趟已按 (z, 树序) 排好）。
void LfwGodot::_draw() {
  GodotWorldRenderer* const renderer = _host != nullptr ? _host->world_renderer() : nullptr;
  if (renderer == nullptr) {
    if (titlebar_visible()) draw_titlebar();
    return;
  }
  const GodotFrame& frame = renderer->take_frame();
  if (!frame.valid) {
    // 这一帧没有世界/UI 的绘制项（比如刚起来还没推过步）：栏照画。
    if (titlebar_visible()) draw_titlebar();
    return;
  }
  static const int kPasses[3] = {-1, 0, 1};
  for (const int pass : kPasses) {
    for (const GodotDrawItem& item : frame.items) {
      if (item.pass != pass) continue;
      if (!item.text.empty()) {
        draw_text_item(item);
      } else {
        draw_item(item);
      }
    }
  }
  // 自绘标题栏压在**所有**绘制项之上（它就是"窗口的边框"，与 Electron 在 DOM 最后画那一层同理）。
  if (titlebar_visible()) draw_titlebar();
}

// UI 文字（`UITextRenderer` / `TextMesh` 的口径）：字号来自 `style.font`、颜色来自 `fill_style`、
// 对齐来自 `text_align`、左上各 2 像素内边距（`UITextRenderer._normalize_style`）。逐行画
//（TS 也是在画布上逐行 `fillText`），基线用字体的 ascent。
void LfwGodot::draw_text_item(const GodotDrawItem& item) {
  if (item.text.empty()) return;
  godot::Ref<godot::Font> font = _font;
  if (font.is_null()) return;
  const int size = static_cast<int>(std::lround(item.font_size > 0.0 ? item.font_size : 9.0));
  const int font_size = size > 0 ? size : 1;
  const Color color(static_cast<float>(item.r), static_cast<float>(item.g),
                    static_cast<float>(item.b), static_cast<float>(item.a));
  const godot::HorizontalAlignment align =
      item.text_align == 1   ? godot::HORIZONTAL_ALIGNMENT_CENTER
      : item.text_align == 2 ? godot::HORIZONTAL_ALIGNMENT_RIGHT
                             : godot::HORIZONTAL_ALIGNMENT_LEFT;
  const float pad = 2.0f;
  const float width = static_cast<float>(item.w);
  const float line_h = font->get_height(font_size);
  float y = static_cast<float>(item.y) + pad;
  std::size_t start = 0;
  while (start <= item.text.size()) {
    const std::size_t nl = item.text.find('\n', start);
    const std::string line = item.text.substr(start, nl == std::string::npos ? std::string::npos
                                                                            : nl - start);
    if (!line.empty()) {
      draw_string(font, Vector2(static_cast<float>(item.x) + pad, y + font->get_ascent(font_size)),
                  String::utf8(line.c_str()), align, width, font_size, color);
    }
    if (nl == std::string::npos) break;
    y += line_h;
    start = nl + 1;
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
  // UI 的纯色块（覆盖色 / `background` / `foreground`）：直接填，不去查纹理、也不画
  //「取不到图」的占位色块（那是实体那一遍的诊断）。
  if (item.flat) {
    draw_rect(Rect2(x, y, w, h), color, true);
    return;
  }
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
    // UI 那一遍（D4 第二半，本切片新增的观测量；不进三条摘要）。
    const GodotUIStats& ui = renderer->ui_stats();
    std::snprintf(buf, sizeof(buf),
                  "[godot] ui: layers=%lld nodes=%lld drawn=%lld items=%lld texts=%lld hidden=%lld "
                  "empty=%lld clipped=%lld missing_img=%lld total_items=%zu",
                  ui.layers, ui.nodes, ui.drawn, ui.items, ui.texts, ui.hidden, ui.empty,
                  ui.clipped, ui.missing_img, f.items.size());
    print_line(buf);
  }
  // `--ui-dump`：最后一帧逐 UI 节点的一行事实（诊断；不进摘要）。
  if (_args.ui_dump && renderer != nullptr) {
    for (const std::string& line : renderer->ui_dump_lines()) print_line(line);
  }
  quit_with(0);
}

void LfwGodot::quit_with(int code) const {
  godot::SceneTree* const tree = get_tree();
  if (tree != nullptr) tree->quit(code);
}

void LfwGodot::_bind_methods() {}

}  // namespace lfw::host
