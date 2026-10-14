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

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "host/common/lfw_boot.h"
#include "lfw/base/clock.h"
#include "lfw/core/value.h"
#include "lfw/utils/utf8.h"
#include "lfw/world.h"

namespace lfw::host {
namespace {

using godot::String;
using godot::UtilityFunctions;

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
  return "usage: <godot> --headless --path native/host/godot/project -- --root <dir> --zip <dir>\n"
         "               [--zip <dir> ...] [--fighter <id>] [--stage <id>] [--players N]\n"
         "               [--frames N] [--every N] [--time ms] [--delta seconds] [--vs]\n"
         "  --root     import 根目录（含 builtin_data/…），可重复；缺省 `.`\n"
         "  --zip      数据包目录（已解包），可重复；按给定顺序 load\n"
         "  --fighter  指定出场的角色 id，可重复（缺省用数据包里的前 `--players` 个）\n"
         "  --delta    只覆盖 `_process` 的节拍（秒），不改固定步长（默认用 Godot 给的 delta）\n";
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
    } else if (a == "--vs") {
      out.vs = true;
    } else {
      return false;
    }
  }
  if (out.roots.empty()) out.roots.emplace_back(".");
  if (out.every <= 0) out.every = 60;
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
  return true;
}

void LfwGodot::_process(double p_delta) {
  if (!_running) return;
  // 节拍只决定「这一帧推几步」；固定步长永远是 `_host->step_ms()`（§G3 第 3 条）。
  double pacing = _args.delta > 0.0 ? _args.delta : p_delta;
  if (!(pacing > 0.0)) pacing = 1.0 / 60.0;  // 节拍兜底：别让 0 delta 把验收卡死
  _host->accumulate(pacing);

  while (_running && _frames < _args.frames && _host->can_step()) {
    _host->push_frame(_lfw->world());
    const int frame = _frames;
    ++_frames;
    if (frame % _args.every == 0 || frame == _args.frames - 1) report_frame(frame);
  }
  if (_frames >= _args.frames) finish();
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
  char buf[512];
  std::snprintf(buf, sizeof(buf),
                "[godot] done: frames=%d ups=%.0f digest=%s warns=%d errors=%d ignored_images=%lld "
                "renderer(+-)=%lld/%lld clock=%.1fms residual=%.1fms steps=%lld",
                _frames, 1000.0 / _host->step_ms(), digest.c_str(), _host->warn_count(),
                _host->error_count(), _host->ignored_images(), _host->renderer().added,
                _host->renderer().removed, _host->clock().now(), _host->residual_ms(),
                _host->steps());
  print_line(buf);
  // UI 渲染面（DESIGN D4）：与 `lfw_run` 同口径（本宿主也不装 `IRenderScheduler` ⇒ 0）。
  std::snprintf(buf, sizeof(buf), "[godot] UI render calls: %lld (unhandled: %lld)",
                world.ui_render_calls(), world.ui_render_unhandled());
  print_line(buf);
  quit_with(0);
}

void LfwGodot::quit_with(int code) const {
  godot::SceneTree* const tree = get_tree();
  if (tree != nullptr) tree->quit(code);
}

void LfwGodot::_bind_methods() {}

}  // namespace lfw::host
