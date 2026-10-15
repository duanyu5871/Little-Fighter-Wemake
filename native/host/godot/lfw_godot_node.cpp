#include "host/godot/lfw_godot_node.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
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
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include "host/common/key_code.h"
#include "host/common/lfw_boot.h"
#include "host/common/ui_input_handle.h"
#include "lfw/base/clock.h"
#include "lfw/core/js_string.h"
#include "lfw/core/value.h"
#include "lfw/defines/cmd.h"
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

// 控件表里那两路音量（只有它们有悬停弹层）。
bool is_volume_control(int index) {
  const auto& list = lfw::host::titlebar_controls();
  if (index < 0 || index >= static_cast<int>(list.size())) return false;
  const lfw::host::TitleAction action = list[static_cast<std::size_t>(index)].action;
  return action == lfw::host::TitleAction::bgm_mute || action == lfw::host::TitleAction::sound_mute;
}

// 整个文件读成字节（顶栏那 11 张 PNG 在仓库的 `src/assets` 里，不在数据包里 ⇒ 不走 `IZip`）。
bool read_file_bytes(const std::filesystem::path& path, std::string& out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return !out.empty();
}

// 从 `start` 一路往上找"含 `src/assets/btn_1_1.png` 的目录"（= 仓库根）。Godot 跑工程时会把 cwd
// 切到**工程目录**（`--path` 的副作用）⇒ 从 `native/host/godot/project` 往上 4 层就是仓库根。
std::filesystem::path find_repo_root(std::filesystem::path start) {
  std::error_code ec;
  for (int depth = 0; depth < 8; ++depth) {
    if (start.empty()) break;
    if (std::filesystem::is_regular_file(start / "src" / "assets" / "btn_1_1.png", ec)) {
      return start;
    }
    const std::filesystem::path parent = start.parent_path();
    if (parent == start) break;
    start = parent;
  }
  return {};
}

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
         "               [--hover-window <x>,<y>[@<frame>]] [--click-frame N]\n"
         "               [--titlebar native|none|custom] [--window-scale N]\n"
         "               [--window-size WxH] [--icons <dir>]\n"
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
         "  --hover-window  只推一条 `InputEventMouseMotion`（窗口像素）—— 悬停态（顶栏的拖拽层高亮、\n"
         "             控件缩 0.8、音量弹层）只有它能推；`--click*` 只推按钮事件。\n"
         "  --key          在第 `--key-frame` 帧（或 `@frame`）按一下（按下 + `--key-tap` 帧后抬起），\n"
         "             走 `HeadlessHost::key_down/key_up`（与真键盘的 `_input` 同一条链）；\n"
         "             键名取小写 `KeyboardEvent.key`（`space` / `esc` / `up` / `j`…）。可重复。\n"
         "  --key-down/--key-up  只按下 / 只抬起（要「按住」就用这一对）。\n"
         "  --key-frame    没写 `@frame` 的按键落在哪一帧（缺省 60）\n"
         "  --key-tap      「按一下」的按下 → 抬起间隔（帧，缺省 2）\n"
         "  --titlebar 窗口那一套：native（系统边框）/ none（无边框、不画栏）/ custom（无边框 +\n"
         "             自绘栏，照 Electron 那份顶栏：11 个 `btn_*.png` 控件 + 拖拽层 + 音量弹层）。\n"
         "             缺省：`--play` 时 custom，其余 native。\n"
         "  --icons    顶栏那 11 张 `btn_*.png` 的目录（缺省 = 从仓库根解析出来的 `src/assets`；\n"
         "             它们**不在数据包里**，导出/打包时得自己带上）。\n"
         "  --window-scale  窗口 = 逻辑视口（794×450）× N；逻辑视口不变（`project.godot` 的 stretch）。\n"
         "  --window-size  直接给窗口像素尺寸 `WxH`（可不等比，压过 `--window-scale`）；窗口可自由拉伸，\n"
         "             下限是逻辑视口 794×450；画面按 stretch 的 `aspect=keep` 等比 contain + 居中（补黑边）。\n"
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
    } else if (a == "--hover-window") {
      if (!need(i, v)) return false;
      out.hover_specs.push_back(v);
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
    } else if (a == "--window-size") {
      if (!need(i, v)) return false;
      // `WxH`（大小写都认，分隔符 `x` 或 `X`）——与 Godot 自己的 `--resolution WxH` 同形。
      int w = 0;
      int h = 0;
      if (std::sscanf(v.c_str(), "%dx%d", &w, &h) != 2 || w < 1 || h < 1 || w > 16384 || h > 16384) {
        print_error("[godot] bad --window-size value: " + v + " (want WxH, e.g. 1191x675)");
        return false;
      }
      out.window_w = w;
      out.window_h = h;
    } else if (a == "--icons") {
      if (!need(i, v)) return false;
      out.icons = std::filesystem::path(v);
      out.icons_given = true;
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
  for (const std::string& spec : out.hover_specs) {
    Args::ClickAt click;
    if (!parse_click_spec(spec, out.click_frame, click.x, click.y, click.frame)) {
      print_error("[godot] bad --hover-window value: " + spec + " (want <x>,<y>[@<frame>])");
      return false;
    }
    click.window_space = true;
    click.hover = true;
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
  setup_icons();  // 自绘顶栏那 11 张图：与数据无关（在仓库的 `src/assets` 里，不在数据包里）
  if (!setup()) {
    quit_with(1);
    return;
  }
  // ⚠ GDExtension 必须显式打开输入处理，不然引擎不会调 `_input`（默认关）。
  set_process_input(true);
  _running = true;
  // 证据行：这一局开窗时条上有哪几个控件（`src/App.tsx` 那 11 个 `Show show=` 的求值结果）。
  if (titlebar_visible()) report_titlebar_controls();
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
                    clicked->hover ? "hover-win" : (clicked->window_space ? "click-win" : "click"),
                    frame, clicked->x, clicked->y, ui_pages_text().c_str());
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
// 边缘方向 → 缩放游标形状。⚠ 两个**对角**形状（`FDIAGSIZE` / `BDIAGSIZE`）的命名在 Godot 里
// 没有逐个核对过（有的 UI 把正向对角当 `\`、有的当 `/`）⇒ 万一反了只是光标图标镜像，不影响拖拽。
inline godot::DisplayServer::CursorShape cursor_for_edge(TitleEdge edge) {
  using DS = godot::DisplayServer;
  switch (edge) {
    case TitleEdge::left:
    case TitleEdge::right:
      return DS::CURSOR_HSIZE;
    case TitleEdge::top:
    case TitleEdge::bottom:
      return DS::CURSOR_VSIZE;
    case TitleEdge::top_left:
    case TitleEdge::bottom_right:
      return DS::CURSOR_FDIAGSIZE;
    case TitleEdge::top_right:
    case TitleEdge::bottom_left:
      return DS::CURSOR_BDIAGSIZE;
    case TitleEdge::none:
    default:
      return DS::CURSOR_ARROW;
  }
}

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
  const bool explicit_size = _args.window_w > 0 && _args.window_h > 0;
  const godot::Vector2i want =
      explicit_size
          ? godot::Vector2i(_args.window_w, _args.window_h)
          : godot::Vector2i(static_cast<int>(std::lround(static_cast<double>(logical.x) * scale)),
                            static_cast<int>(std::lround(static_cast<double>(logical.y) * scale)));
  // **允许自由拉伸**：`RESIZE_DISABLED` 明确关掉（无边框窗口在 Windows 上是 `WS_POPUP` ⇒ 没有原生
  // 可拖的边框，边缘那条 6 像素带自己叫 `window_start_resize`，见 `titlebar_edge_at`）。
  // 下限取逻辑视口 794×450 —— 与 Electron 桌面端的 `minWidth/minHeight: 794/450` 同口径
  //（再小就等于把逻辑屏幕缩没，UI 命中与文字度量都开始失真）。
  ds->window_set_flag(DisplayServer::WINDOW_FLAG_RESIZE_DISABLED, false);
  ds->window_set_min_size(logical);
  if (ds->window_get_size() != want) {
    ds->window_set_size(want);
    if (scale != 1.0 || explicit_size) {
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
  // 内容在窗口里的矩形（= contain 之后的黑边量）：`get_final_transform()` 就是「逻辑像素 → 窗口像素」
  // 那一步（`window_transform * stretch_transform`），它的平移量就是那圈黑边的偏移 ⇒ 左右/上下是否
  // **对称居中**由这两个数一眼看出（`aspect=keep` 时 Godot 取 `min(w/794, h/450)` 并居中）。
  int cx = 0;
  int cy = 0;
  int cw = logical.x;
  int ch = logical.y;
  if (const godot::Viewport* const vp = get_viewport(); vp != nullptr) {
    const godot::Transform2D t = vp->get_final_transform();
    const godot::Vector2 o = t.xform(godot::Vector2());
    const godot::Vector2 s = vp->get_visible_rect().size * t.get_scale();
    cx = static_cast<int>(std::lround(o.x));
    cy = static_cast<int>(std::lround(o.y));
    cw = static_cast<int>(std::lround(s.x));
    ch = static_cast<int>(std::lround(s.y));
  }
  char buf[448];
  std::snprintf(buf, sizeof(buf),
                "[godot] window: display=%s titlebar=%s scale=%.2f borderless=%d size=%dx%d "
                "deco=%dx%d pos=%d,%d mode=%d logical=%dx%d resizable=%d min=%dx%d "
                "content=%dx%d+%d+%d",
                display.c_str(), titlebar_name(_args.titlebar), scale,
                ds->window_get_flag(DisplayServer::WINDOW_FLAG_BORDERLESS) ? 1 : 0, now.x, now.y,
                deco.x, deco.y, pos.x, pos.y, static_cast<int>(ds->window_get_mode()), logical.x,
                logical.y,
                ds->window_get_flag(DisplayServer::WINDOW_FLAG_RESIZE_DISABLED) ? 0 : 1,
                ds->window_get_min_size().x, ds->window_get_min_size().y, cw, ch, cx, cy);
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

// ---- 自绘顶栏（`--titlebar custom`）：可见性 / 选中态 / 动作 ----
//
// `lfw_godot_titlebar.h` 只管几何与一张条件表；这里把端口现成的状态填进去（`src/App.tsx` 的每个
// `Show show=` / `ToggleImgButton.src` 逐条对位），并把点击接到端口已有的那些面上
//（`LFW::push_cmd(CMD.F*)` / `LFW::sounds_*` / `DisplayServer` 的窗口 API）。

// 可见性表。求值不了的两条照实写在 README 里：
//   * `is_mobile_container` / `toy_mobile`（UA + 触摸判定）⇒ 桌面宿主恒 false；
//   * `window.runtime`（Wails/Electron 的桥）⇒ 用"真窗口"（`_windowed`）代替。
lfw::host::TitleBarConditions LfwGodot::titlebar_conditions() const {
  lfw::host::TitleBarConditions c;
  if (_lfw == nullptr) return c;
  const lfw::World& world = _lfw->world();
  c.cheat_gim_ink = _lfw->is_cheat(lfw::cmd::kGIM_INK);
  lfw::ui::UILayers* const layers = _lfw->ui_layers();
  const lfw::ui::UINode* const ui = layers != nullptr ? layers->ui() : nullptr;
  c.has_ui = ui != nullptr;
  c.settings_page = ui != nullptr && lfw::to_string(ui->id()) == u"settings";
  const lfw::ui::UILayer* const bottom = layers != nullptr ? layers->bottom() : nullptr;
  c.bottom_pages = bottom != nullptr ? static_cast<int>(bottom->pages().size()) : 0;
  if (const lfw::Background* const bg = world.bg()) {
    c.bg_is_void = lfw::to_string(bg->id()) == u"VOID_BG";
  }
  c.first_page_init_demo = _lfw->first_page == u"init_demo";
  c.has_window = _windowed;
  c.mobile_or_toy = false;
  if (godot::DisplayServer* const ds = godot::DisplayServer::get_singleton()) {
    const godot::DisplayServer::WindowMode mode = ds->window_get_mode();
    c.fullscreen = mode == godot::DisplayServer::WINDOW_MODE_FULLSCREEN ||
                   mode == godot::DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
  }
  return c;
}

// 每个控件的 `checked`（决定显示 `src[0]` 还是 `src[1]`，见 `lfw_godot_titlebar.h` 那条反直觉的规则）。
// 来源逐条对 `src/App.tsx`：`app_state.bgm_muted` / `sound_muted` 在端口的等价物就是宿主那两位
//（`LFW::sounds_*`）；`paused` = `world.paused()`；`is_fullscreen` / `is_maximised` = 窗口模式；
// `dev_ui_open` 端口没有（恒 false）。
bool LfwGodot::titlebar_checked(lfw::host::TitleAction action) const {
  using lfw::host::TitleAction;
  if (_lfw == nullptr) return false;
  godot::DisplayServer* const ds = godot::DisplayServer::get_singleton();
  const godot::DisplayServer::WindowMode mode =
      ds != nullptr ? ds->window_get_mode() : godot::DisplayServer::WINDOW_MODE_WINDOWED;
  switch (action) {
    case TitleAction::bgm_mute:
      return _lfw->sounds_bgm_muted();
    case TitleAction::sound_mute:
      return _lfw->sounds_sound_muted();
    case TitleAction::f1:
      return _lfw->world().paused();
    case TitleAction::f5:  // `checked={lfw?.world.dataset.playrate != 1}`
      return lfw::to_number(_lfw->world().dataset.get(u"playrate")) != 1.0;
    case TitleAction::fullscreen:
      return mode == godot::DisplayServer::WINDOW_MODE_FULLSCREEN ||
             mode == godot::DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN;
    case TitleAction::maximize:
      return mode == godot::DisplayServer::WINDOW_MODE_MAXIMIZED;
    case TitleAction::dev_ui:
    case TitleAction::f4:
    case TitleAction::settings:
    case TitleAction::minimize:
    case TitleAction::close:
    case TitleAction::none:
      break;
  }
  return false;
}

// 点下去干什么。能做的都与 Electron 那一行等义；做不了的只有 `dev_ui`（端口没有那个面板）。
void LfwGodot::titlebar_activate(int index) {
  using lfw::host::TitleAction;
  const auto& list = lfw::host::titlebar_controls();
  if (_lfw == nullptr || index < 0 || index >= static_cast<int>(list.size())) return;
  godot::DisplayServer* const ds = godot::DisplayServer::get_singleton();
  const TitleAction action = list[static_cast<std::size_t>(index)].action;
  switch (action) {
    case TitleAction::dev_ui:
      // Electron 那边是 `set_app_state(d => d.dev_ui_open = !d.dev_ui_open)` —— 端口没有那个面板
      // ⇒ 按键/悬停都照做，动作只打一行（照实说，不假装）。
      print_line("[godot] titlebar: dev-ui (no dev panel in this host)");
      break;
    case TitleAction::f4:
      _lfw->push_cmd({std::u16string(lfw::cmd::kF4)});
      print_line("[godot] titlebar: f4 pages=" + ui_pages_text());
      break;
    case TitleAction::f1:
      _lfw->push_cmd({std::u16string(lfw::cmd::kF1)});
      print_line("[godot] titlebar: f1 pages=" + ui_pages_text());
      break;
    case TitleAction::f5:
      _lfw->push_cmd({std::u16string(lfw::cmd::kF5)});
      print_line("[godot] titlebar: f5 pages=" + ui_pages_text());
      break;
    case TitleAction::settings: {
      _lfw->push_cmd({std::u16string(lfw::cmd::kF2)});
      lfw::ui::UILayers* const layers = _lfw->ui_layers();
      lfw::ui::UINode* const ui = layers != nullptr ? layers->ui() : nullptr;
      lfw::ui::UILayer* const layer = ui != nullptr ? ui->layer() : nullptr;
      if (layer != nullptr && lfw::to_string(ui->id()) == u"settings") {
        layer->pop(lfw::ui::UIPopPageOpts{});  // `lfw.ui.layer?.pop()`
        print_line("[godot] titlebar: settings (pop) pages=" + ui_pages_text());
      } else if (layers != nullptr) {
        auto opts = std::make_shared<lfw::Object>();
        opts->set(u"id", Value(std::u16string(u"settings")));
        layers->set_page(Value(std::move(opts)), 1.0);  // `layers.set_page({id:"settings"}, 1)`
        print_line("[godot] titlebar: settings (set_page) pages=" + ui_pages_text());
      }
      break;
    }
    case TitleAction::bgm_mute: {
      const bool muted = !_lfw->sounds_bgm_muted();
      _lfw->sounds_set_bgm_muted(muted);
      print_line(std::string("[godot] titlebar: bgm_muted=") + (muted ? "true" : "false"));
      break;
    }
    case TitleAction::sound_mute: {
      const bool muted = !_lfw->sounds_sound_muted();
      _lfw->sounds_set_sound_muted(muted);
      print_line(std::string("[godot] titlebar: sound_muted=") + (muted ? "true" : "false"));
      break;
    }
    case TitleAction::fullscreen: {
      if (ds == nullptr) break;
      const bool full = titlebar_checked(TitleAction::fullscreen);
      ds->window_set_mode(full ? godot::DisplayServer::WINDOW_MODE_WINDOWED
                               : godot::DisplayServer::WINDOW_MODE_FULLSCREEN);
      print_line(std::string("[godot] titlebar: fullscreen=") + (full ? "false" : "true"));
      break;
    }
    case TitleAction::minimize:
      if (ds == nullptr) break;
      ds->window_set_mode(godot::DisplayServer::WINDOW_MODE_MINIMIZED);
      print_line("[godot] titlebar: minimize (" + window_state_text(ds) + ")");
      break;
    case TitleAction::maximize: {
      if (ds == nullptr) break;
      const bool maximized = ds->window_get_mode() == godot::DisplayServer::WINDOW_MODE_MAXIMIZED;
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
      break;
  }
  queue_redraw();
}

// 悬停态：照 `.volume_hover:hover` 的意义——指针在某个音量控件上、或**在它自己的弹层里** ⇒ 那一层
// 开着；否则关（同时只可能开一个）。拖滑杆期间不关（`setPointerCapture` 的等价物）。
// `drag_hover` = 指针在 `.wails_drag` 那一层上（`#88888855` 那块高亮）。变了才 `queue_redraw()`。
void LfwGodot::titlebar_update_hover(const lfw::host::TitleBarLayout& layout,
                                     const lfw::host::TitleHit& hit, double bx, double by) {
  using lfw::host::TitleBarMetrics;
  using lfw::host::TitleHitKind;
  const int hover = hit.kind == TitleHitKind::button ? hit.index : -1;
  int popup = -1;
  if (_bar.slider_drag >= 0) {
    popup = _bar.slider_drag;
  } else {
    for (int i = 0; i < TitleBarMetrics::count; ++i) {
      if (!layout.visible[i] || !is_volume_control(i)) continue;
      if (hover == i ||
          lfw::host::titlebar_rect_contains(lfw::host::titlebar_popup_rect(layout.rect[i]), bx,
                                            by)) {
        popup = i;
      }
    }
  }
  const bool drag_hover = hit.kind == TitleHitKind::drag;
  if (hover == _bar.hover && drag_hover == _bar.drag_hover && popup == _bar.popup) return;
  _bar.hover = hover;
  _bar.drag_hover = drag_hover;
  _bar.popup = popup;
  queue_redraw();
}

// 滑杆动了：`VerticalSlider.onChange` 那两句 —— 设音量，且"拖起来就取消静音"。
void LfwGodot::titlebar_set_volume(int index, double v) {
  using lfw::host::TitleAction;
  const auto& list = lfw::host::titlebar_controls();
  if (_lfw == nullptr || index < 0 || index >= static_cast<int>(list.size())) return;
  const TitleAction action = list[static_cast<std::size_t>(index)].action;
  if (action == TitleAction::bgm_mute) {
    _lfw->sounds_set_bgm_volume(v);
    if (v > 0.0 && _lfw->sounds_bgm_muted()) _lfw->sounds_set_bgm_muted(false);
  } else if (action == TitleAction::sound_mute) {
    _lfw->sounds_set_sound_volume(v);
    if (v > 0.0 && _lfw->sounds_sound_muted()) _lfw->sounds_set_sound_muted(false);
  } else {
    return;
  }
  queue_redraw();
}

// 音量滑杆的位置（0..1）：与 `app_state.bgm_volume` 等价的那位 = 宿主的音量（`HeadlessHost` 存的）。
double LfwGodot::titlebar_volume(int index) const {
  using lfw::host::TitleAction;
  const auto& list = lfw::host::titlebar_controls();
  if (_lfw == nullptr || index < 0 || index >= static_cast<int>(list.size())) return 0.0;
  const TitleAction action = list[static_cast<std::size_t>(index)].action;
  if (action == TitleAction::bgm_mute) return _lfw->sounds_bgm_volume();
  if (action == TitleAction::sound_mute) return _lfw->sounds_sound_volume();
  return 0.0;
}

// 顶栏的「窗口像素」里，条有多宽 = **整个窗口宽**（不是内容区宽）。
// ⚠ 别退回"内容区宽"：`aspect=keep` 补黑边时内容区比窗口窄（例：窗口 1400×500 ⇒ 内容 882×500 居中），
// 按内容宽布局会让条只盖到中间那 882 px、右上角按钮跑到内容右缘（用户实测报的 bug）。
// Electron 那边 `.top_bar` 是 `left:0; right:0` 的 CSS 像素 ⇒ 口径与这里一致。
double LfwGodot::titlebar_window_width(const godot::Vector2i& window_size) const {
  return static_cast<double>(window_size.x);
}

// 可见集合变了就打一行（证据行：哪几个控件在条上）。诊断用，不进摘要。
void LfwGodot::report_titlebar_controls() {
  godot::DisplayServer* const ds = godot::DisplayServer::get_singleton();
  if (ds == nullptr) return;
  const godot::Vector2i win_size = ds->window_get_size();
  const double bar_w = titlebar_window_width(win_size);
  const lfw::host::TitleBarLayout layout = lfw::host::titlebar_layout(bar_w, titlebar_conditions());
  const auto& list = lfw::host::titlebar_controls();
  std::string text;
  for (int i = 0; i < lfw::host::TitleBarMetrics::count; ++i) {
    if (!layout.visible[i]) continue;
    if (!text.empty()) text += " ";
    text += lfw::host::titlebar_action_name(
        list[static_cast<std::size_t>(i)].action);  // 只看名字（文件名见 README 那张表）
  }
  char buf[512];
  std::snprintf(buf, sizeof(buf), "[godot] titlebar: icons=%s bar=%.0fx%.0f controls(%d)=%s",
                _icons_dir.empty() ? "<none>" : _icons_dir.c_str(), bar_w,
                lfw::host::TitleBarMetrics::bar_height, layout.count, text.c_str());
  const std::string line(buf);
  if (line == _icon_report) return;
  _icon_report = line;
  print_line(line);
}

// 条上与窗口边缘的事件处理。返回 true = 宿主吃掉（调用方标 handled，别再喂给游戏/UI）。
//
// 次序（照 Electron 的层序）：① 边缘缩放带（**窗口**像素，压过一切 —— 与原生边框同口径）
// → ② 音量弹层（`z-index: 2`，挂在条下沿、盖住画面那一带）→ ③ 控件盒 → ④ `.wails_drag`
// → ⑤ 起手在条上/弹层里、现在跑到外面的 move/up 也吞（免得松手那一下点着游戏里的按钮）。
// ⚠ 条上没有背景、拖拽层只占 90% 高、`.top_bar` 又是 `pointer-events: none` ⇒ 底部那 3.2 像素的
// 缝在 Electron 上是**漏给画面**的，这里也一样（只有按钮盒/弹层/拖拽层吞）。
bool LfwGodot::titlebar_consume(const godot::Ref<godot::InputEvent>& event) {
  using lfw::host::TitleBarMetrics;
  using lfw::host::TitleHit;
  using lfw::host::TitleHitKind;
  if (!titlebar_visible()) return false;
  LfwPointingEvent e;
  const PointerKind kind = to_pointing_event(event, e);
  if (kind == PointerKind::none) return false;
  godot::DisplayServer* const ds = godot::DisplayServer::get_singleton();
  if (ds == nullptr) return false;
  const godot::Viewport* const vp = get_viewport();
  if (vp == nullptr) return false;

  // 窗口像素（`get_final_transform()` 的正变换）。条自己就是**窗口像素**里的一层（绘制那边设的是
  // `final.affine_inverse()`，条上 (0,0) = 窗口左上角）⇒ 命中坐标**不减** letterbox 偏移。
  const godot::Transform2D final = vp->get_final_transform();
  const godot::Vector2 win =
      final.xform(godot::Vector2(static_cast<float>(e.x), static_cast<float>(e.y)));
  const godot::Vector2i win_size = ds->window_get_size();
  const double bx = static_cast<double>(win.x);
  const double by = static_cast<double>(win.y);
  const double bar_w = titlebar_window_width(win_size);
  const lfw::host::TitleBarConditions cond = titlebar_conditions();
  const lfw::host::TitleBarLayout layout = lfw::host::titlebar_layout(bar_w, cond);
  report_titlebar_controls();

  // 弹层几何（只有音量那两个控件有；开着的那个才吃事件）。
  const bool popup_open = _bar.popup >= 0 && layout.visible[_bar.popup];
  const lfw::host::TitleBarRect popup_rect =
      popup_open ? lfw::host::titlebar_popup_rect(layout.rect[_bar.popup])
                 : lfw::host::TitleBarRect{};
  const TitleHit hit = lfw::host::titlebar_hit(bx, by, layout);
  const bool in_popup = popup_open && lfw::host::titlebar_rect_contains(popup_rect, bx, by);

  // 悬停态（控件/拖拽层/弹层）：移动**与按下**都先刷一遍 —— 按下的位置就是指针的位置，
  // 所以「从控件挪到自己的弹层上」这一步在真实事件流里一定先有一条 move，这里等价补齐。
  if (kind == PointerKind::move || kind == PointerKind::down) {
    titlebar_update_hover(layout, hit, bx, by);
  }

  // 悬停到边缘 ⇒ 换成对应方向的缩放游标；音量滑杆那一带是 `cursor: pointer`。只在形状变化时调。
  if (kind == PointerKind::move) {
    using DS = godot::DisplayServer;
    DS::CursorShape shape = cursor_for_edge(titlebar_edge_at(win.x, win.y, win_size.x, win_size.y));
    const bool on_volume = in_popup || (hit.kind == TitleHitKind::button &&
                                        is_volume_control(hit.index));
    if (shape == DS::CURSOR_ARROW && on_volume) shape = DS::CURSOR_POINTING_HAND;
    if (static_cast<int>(shape) != _cursor_shape) {
      _cursor_shape = static_cast<int>(shape);
      ds->cursor_set_shape(shape);
    }
  }

  // 拖拽/缩放那两条：压过条上的一切（与原生边框同口径）。
  if (kind == PointerKind::down && e.button == 0.0 &&
      ds->window_get_mode() == godot::DisplayServer::WINDOW_MODE_WINDOWED &&
      !ds->window_get_flag(godot::DisplayServer::WINDOW_FLAG_RESIZE_DISABLED)) {
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

  // ---- 移动：悬停/弹层已经在上面刷过，这里只管滑杆拖动与"吞不吞"----
  if (kind == PointerKind::move) {
    if (_bar.slider_drag >= 0) {
      // 正在拖滑杆（`setPointerCapture` 的等价物）：弹层不关，值跟着 y 走。
      // 轨道按**当前**布局重算（拖的过程中页面可能换了、条上的格子在挪）。
      lfw::host::TitleBarRect drag_popup;
      lfw::host::TitleBarRect drag_track;
      lfw::host::titlebar_popup_rects(layout.rect[_bar.slider_drag], drag_popup, drag_track);
      titlebar_set_volume(_bar.slider_drag, lfw::host::titlebar_slider_value(drag_track, by));
      return true;
    }
    if (hit.kind == TitleHitKind::none && !in_popup) return false;  // 缝里/条外的移动照旧给游戏
    return true;
  }

  // ---- 按下 ----
  if (kind == PointerKind::down) {
    // 按下就落在弹层里（指针从控件挪到弹层上）⇒ 弹层几何要按**刚刷过**的状态重算一次。
    lfw::host::TitleBarRect down_popup;
    lfw::host::TitleBarRect down_track;
    bool down_in_popup = false;
    if (_bar.popup >= 0 && layout.visible[_bar.popup]) {
      lfw::host::titlebar_popup_rects(layout.rect[_bar.popup], down_popup, down_track);
      down_in_popup = lfw::host::titlebar_rect_contains(down_popup, bx, by);
    }
    if (e.button != 0.0) {
      // 非左键：条上/弹层里的按下照样吞（别漏给游戏），但不做动作。
      return hit.kind != TitleHitKind::none || down_in_popup;
    }
    if (down_in_popup) {
      _bar_capture = true;
      // 弹层里：先按点到哪设到哪（`on_pointer_down` 的第一句），再抓住这个滑杆（`setPointerCapture`）。
      titlebar_set_volume(_bar.popup, lfw::host::titlebar_slider_value(down_track, by));
      _bar.slider_drag = _bar.popup;
      return true;
    }
    if (hit.kind == TitleHitKind::button) {
      _bar_capture = true;
      titlebar_activate(hit.index);
      return true;
    }
    if (hit.kind == TitleHitKind::drag) {
      _bar_capture = true;
      // 拖拽区。`window_start_drag` 在 Windows 上就是
      // `SendMessage(WM_SYSCOMMAND, SC_MOVE | HTCAPTION)` ⇒ 这一段**阻塞**到松开为止
      //（Godot 自己的标题栏同款）⇒ 重入用 `_bar_dragging` 挡掉。
      if (!_bar_dragging) {
        _bar_dragging = true;
        print_line("[godot] titlebar: drag");
        ds->window_start_drag();
        _bar_dragging = false;
      }
      return true;
    }
    return false;  // 底部那 3.2 像素的缝 ⇒ 漏给画面（同 Electron）
  }

  // ---- 抬起 / 滚轮 ----
  if (kind == PointerKind::up) {
    if (_bar.slider_drag >= 0) {
      _bar.slider_drag = -1;
      _bar_capture = false;
      return true;
    }
    if (hit.kind != TitleHitKind::none || in_popup) {
      _bar_capture = false;
      return true;
    }
  }
  if (kind == PointerKind::wheel) return hit.kind != TitleHitKind::none || in_popup;
  if (in_popup) return true;
  if (_bar_capture) {
    if (kind == PointerKind::up) _bar_capture = false;
    return true;
  }
  return false;
}

// 图标目录：`--icons <dir>` 优先；否则从**进程 cwd**与**可执行文件所在目录**往上找 `src/assets`。
// 找不到就打印一次并退回"只画命中盒"（那 11 张 PNG **不在数据包里** ⇒ 导出/打包要自己带上，见 README）。
void LfwGodot::setup_icons() {
  std::vector<std::filesystem::path> probes;
  if (_args.icons_given) probes.push_back(_args.icons);
  std::error_code ec;
  probes.push_back(std::filesystem::current_path(ec));
  if (godot::OS* const os = godot::OS::get_singleton()) {
    probes.push_back(std::filesystem::path(os->get_executable_path().utf8().get_data())
                         .parent_path());
  }
  for (const std::filesystem::path& probe : probes) {
    if (probe.empty()) continue;
    std::error_code stat_ec;
    // ① 直接指到放图的那一层；② 指到仓库根（自己找 `src/assets`）。
    if (std::filesystem::is_regular_file(probe / "btn_1_1.png", stat_ec)) {
      _icons_dir = probe.generic_string();
    } else {
      const std::filesystem::path root = find_repo_root(probe);
      if (!root.empty()) _icons_dir = (root / "src" / "assets").generic_string();
    }
    if (!_icons_dir.empty()) break;
  }
  if (_icons_dir.empty()) {
    print_error("[godot] titlebar icons: no btn_1_1.png (给 `--icons <dir>`；条上只画命中盒)");
    return;
  }
  const std::string dir = _icons_dir;
  _icons = std::make_unique<GodotTextureCache>(
      [dir](const std::string& name, std::string& out) {
        return read_file_bytes(std::filesystem::path(dir) / name, out);
      });
  print_line("[godot] titlebar icons: " + dir);
}

// 自绘顶栏的画面。全部按 **DOM 的规则**来：
//   * 条**没有背景**（`.top_bar` 只有布局）—— 只有 `.wails_drag` 悬停时那层 `#88888855`（90% 高）；
//   * 每个可见控件的 32×32 图缩在盒子正中（0.6，悬停 0.8）；
//   * 悬停那两路音量时，条下沿挂一个竖直音量条（`.volume_popup`）。
// 画之前把画布换到"窗口像素"（`get_final_transform()` 的逆）⇒ 这里所有数字都是 CSS 像素。
void LfwGodot::draw_titlebar() {
  using lfw::host::TitleBarMetrics;
  using lfw::host::TitleBarRect;
  godot::DisplayServer* const ds = godot::DisplayServer::get_singleton();
  const godot::Viewport* const vp = get_viewport();
  if (ds == nullptr || vp == nullptr) return;
  const godot::Transform2D final = vp->get_final_transform();
  const double bar_w = titlebar_window_width(ds->window_get_size());
  const lfw::host::TitleBarLayout layout = lfw::host::titlebar_layout(bar_w, titlebar_conditions());
  if (!(bar_w > 0.0)) return;
  report_titlebar_controls();  // 只在可见集合变化时打一行（证据行）
  draw_set_transform_matrix(final.affine_inverse());

  const float drag_h = static_cast<float>(TitleBarMetrics::drag_height);
  if (_bar.drag_hover) {
    // `src/App.module.scss:57-63` 的 `.wails_drag:hover { background-color: #88888855 }`。
    draw_rect(Rect2(0.0f, 0.0f, static_cast<float>(bar_w), drag_h),
              Color(0x88 / 255.0f, 0x88 / 255.0f, 0x88 / 255.0f, 0x55 / 255.0f), true);
  }

  const auto& list = lfw::host::titlebar_controls();
  for (int i = 0; i < TitleBarMetrics::count; ++i) {
    if (!layout.visible[i]) continue;
    const lfw::host::TitleBarControl& control = list[static_cast<std::size_t>(i)];
    const bool checked = titlebar_checked(control.action);
    const TitleBarRect icon = lfw::host::titlebar_icon_rect(layout.rect[i], _bar.hover == i);
    godot::Ref<godot::ImageTexture> tex;
    if (_icons != nullptr) tex = _icons->get(lfw::host::titlebar_control_image(control, checked));
    if (tex.is_null()) {
      // 图没解出来（没给 `--icons` 或目录不对）：留一个浅色占位盒，别静默什么都不画。
      draw_rect(Rect2(static_cast<float>(icon.x), static_cast<float>(icon.y),
                      static_cast<float>(icon.w), static_cast<float>(icon.h)),
                Color(0.85f, 0.88f, 0.95f, 0.18f), true);
      continue;
    }
    draw_texture_rect(tex,
                      Rect2(static_cast<float>(icon.x), static_cast<float>(icon.y),
                            static_cast<float>(icon.w), static_cast<float>(icon.h)),
                      false, Color(1.0f, 1.0f, 1.0f, 1.0f));
  }

  // 音量弹层（`z-index: 2`，挂在那两个控件下面）。
  if (_bar.popup >= 0 && layout.visible[_bar.popup]) {
    TitleBarRect popup;
    TitleBarRect track;
    lfw::host::titlebar_popup_rects(layout.rect[_bar.popup], popup, track);
    const double v = titlebar_volume(_bar.popup);
    const double clamped = v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
    // `.volume_popup_inner { background-color: #0F1830CC; border-radius: 8px }`（圆角用角上的圆近似：
    // `draw_rect` 没有圆角；阴影 `box-shadow: 0 0 12px #00000066` 也略去）。
    draw_rounded_rect(popup, TitleBarMetrics::popup_radius, Color(0x0F / 255.0f, 0x18 / 255.0f,
                                                                  0x30 / 255.0f, 0xCC / 255.0f));
    // `.vertical_slider { background: #00000088; border-radius: 3px }`
    draw_rounded_rect(track, TitleBarMetrics::slider_w * 0.5, Color(0.0f, 0.0f, 0.0f, 0x88 / 255.0f));
    // `.fill { background-color: #5A88E0 }`（`bottom: 0; height: v * 100%`）
    draw_rounded_rect(lfw::host::titlebar_fill_rect(track, clamped), TitleBarMetrics::slider_w * 0.5,
                      Color(0x5A / 255.0f, 0x88 / 255.0f, 0xE0 / 255.0f, 1.0f));
    // `.knob { width: 14px; height: 14px; border-radius: 50%; background: #FFF }`
    const TitleBarRect knob = lfw::host::titlebar_knob_rect(track, clamped);
    draw_circle(Vector2(static_cast<float>(knob.x), static_cast<float>(knob.y)),
                static_cast<float>(knob.w * 0.5), Color(1.0f, 1.0f, 1.0f, 1.0f));
  }
  draw_set_transform_matrix(godot::Transform2D());  // 画布还给逻辑像素（下一帧的绘制项照旧）
}

// 圆角矩形（Godot 的 `draw_rect` 没有半径）：两块矩形 + 四个角上的圆。
void LfwGodot::draw_rounded_rect(const lfw::host::TitleBarRect& r, double radius,
                                 const godot::Color& color) {
  const double limit = r.w < r.h ? r.w : r.h;
  const float rad = static_cast<float>(radius > limit * 0.5 ? limit * 0.5 : radius);
  if (!(rad > 0.0f)) {
    draw_rect(Rect2(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.w),
                    static_cast<float>(r.h)),
              color, true);
    return;
  }
  const float x = static_cast<float>(r.x);
  const float y = static_cast<float>(r.y);
  const float w = static_cast<float>(r.w);
  const float h = static_cast<float>(r.h);
  draw_rect(Rect2(x + rad, y, w - 2.0f * rad, h), color, true);
  draw_rect(Rect2(x, y + rad, w, h - 2.0f * rad), color, true);
  draw_circle(Vector2(x + rad, y + rad), rad, color);
  draw_circle(Vector2(x + w - rad, y + rad), rad, color);
  draw_circle(Vector2(x + rad, y + h - rad), rad, color);
  draw_circle(Vector2(x + w - rad, y + h - rad), rad, color);
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

// `--hover-window`：只推一条 `InputEventMouseMotion`（同一套窗口像素口径）—— 顶栏的悬停态
//（拖拽层那层 `#88888855`、控件缩 0.8、音量弹层弹出来）只有它推得动（`--click*` 只推按钮事件）。
void LfwGodot::push_window_hover(double x, double y) {
  godot::Viewport* const vp = get_viewport();
  if (vp == nullptr) return;
  godot::Ref<godot::InputEventMouseMotion> mm;
  mm.instantiate();
  mm->set_position(Vector2(static_cast<float>(x), static_cast<float>(y)));
  mm->set_global_position(mm->get_position());
  mm->set_relative(Vector2(0.0f, 0.0f));
  vp->push_input(mm, false);
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
    if (click.hover) {
      push_window_hover(click.x, click.y);
    } else if (click.window_space) {
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
