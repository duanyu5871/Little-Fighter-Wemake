// WASM 宿主（步行骨架）：把 `lfw_core` + `native/host` 的 headless 宿主接到 WASM 边界上，
// 由 JS 逐帧驱动。这一刀只让 **时钟 + 日志 + 帧控制** 过界（DESIGN W13 第 1 步），
// 渲染 / 音频 / 图片 / 输入**没有**实现，也不假装实现。
//
// 三件事在这里落地：
//  1. `LfwPlatform` 函数表（W4）→ `JsClock` / `JsTimeout` / `JsRender` 三个端口槽的实现；
//     表里 `add` 系列回 0 时，语义与 `host/headless_host.h` 的 `ManualClock` / `NoopTimeout`
//     完全一致（`Ticker` 自己不走，帧由 JS 显式推）⇒ 手动模式下可与 `lfw_run` 对拍。
//  2. `WasmHost`：`HeadlessHost` 的子类，只改 `now/warn/error/log/debug_msg` 五面 ——
//     时钟读 JS，日志写 JS；其余（装载/`IZip`/存档/XML）原样复用 headless 的实现。
//  3. `lfw_digest()`：一帧摘要走 `host/common/lfw_boot.h` 的那一份（与 `lfw_run` 同一口径）。
#include "host/wasm/lfw_platform.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "host/common/lfw_boot.h"
#include "host/fs_zip.h"
#include "host/headless_host.h"
#include "lfw/base/clock.h"
#include "lfw/base/no_emit_callbacks.h"
#include "lfw/base/render_scheduler.h"
#include "lfw/bg/background.h"
#include "lfw/core/js_string.h"
#include "lfw/core/state_hash.h"
#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/defines/i_terrain_info.h"
#include "lfw/entity/entity.h"
#include "lfw/ground.h"
#include "lfw/lfw.h"
#include "lfw/loader/dat_mgr.h"
#include "lfw/stage/stage.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/utf8.h"
#include "lfw/world.h"

namespace {

using lfw::Entity;
using lfw::Value;

// ---------------------------------------------------------------------------
// 平台表（值拷贝：JS 填完调一次 `lfw_platform_init`，之后不再读它）
// ---------------------------------------------------------------------------

LfwPlatform g_platform{};
bool g_platform_ready = false;

constexpr uint32_t kScratchChars = 8192;
std::vector<lfw_char16> g_scratch(kScratchChars, 0);

std::u16string g_error;  // 最近一次失败的消息（`lfw_last_error_len`）

void set_error(const std::u16string& text) { g_error = text; }

void write_scratch(const std::u16string& text) {
  if (g_scratch.empty()) return;
  const size_t n = std::min(text.size(), g_scratch.size() - 1);
  std::memcpy(g_scratch.data(), text.data(), n * sizeof(lfw_char16));
  g_scratch[n] = 0;
}

// JS -> WASM 的常量字符串（stage id / fighter id / zip 目录）：句柄 = 下标 + 1，0 = 无效。
std::vector<std::u16string> g_strings;

const std::u16string* string_at(uint32_t handle) {
  if (handle == 0 || handle > g_strings.size()) return nullptr;
  return &g_strings[handle - 1];
}

std::u16string from_char16(const lfw_char16* s, uint32_t len) {
  if (s == nullptr || len == 0) return std::u16string();
  const char16_t* const p = reinterpret_cast<const char16_t*>(s);
  return std::u16string(p, p + len);
}

// ---------------------------------------------------------------------------
// JS -> WASM 的回调表（W4 的 (c)：JS 持句柄、WASM 持 cb_id）
// ---------------------------------------------------------------------------

struct CbEntry {
  std::function<void()> fn;
  int32_t js_handle = 0;  // JS 那边 `add` 回来的句柄，`del` 时原样还回去
  bool one_shot = false;  // clock / timeout 是一次性的（TS 的批量 flush / setTimeout）
};

std::map<uint32_t, CbEntry> g_callbacks;
uint32_t g_next_cb = 0;

uint32_t cb_alloc(std::function<void()> fn, bool one_shot) {
  const uint32_t id = ++g_next_cb;
  g_callbacks.emplace(id, CbEntry{std::move(fn), 0, one_shot});
  return id;
}

std::string to_utf8(const std::u16string& s) {
  const std::vector<uint8_t> buf = lfw::encode_utf8(s);
  return std::string(buf.begin(), buf.end());
}

void js_log(uint32_t level, const std::u16string& text) {
  if (g_platform.log == nullptr) return;
  std::u16string line = text;
  if (line.size() > kScratchChars - 1) line.resize(kScratchChars - 1);
  g_platform.log(g_platform.user, level, reinterpret_cast<const lfw_char16*>(line.data()),
                 static_cast<uint32_t>(line.size()));
}

std::u16string join_args(const std::vector<Value>& args) {
  std::u16string out;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) out += u" ";
    out += lfw::to_string(args[i]);
  }
  return out;
}

// `IClock`（`Ditto.Clock`）：`now()` 读 JS；`add` 回 0 = 没有句柄（= ManualClock 的回落）。
class JsClock final : public lfw::IClock {
 public:
  double now() const override {
    return g_platform.clock_now != nullptr ? g_platform.clock_now(g_platform.user) : 0.0;
  }
  int add(std::function<void()> handler) override {
    if (g_platform.clock_add == nullptr) return 0;
    const uint32_t id = cb_alloc(std::move(handler), /*one_shot=*/true);
    const int32_t js = g_platform.clock_add(g_platform.user, id);
    if (js == 0) {
      g_callbacks.erase(id);  // JS 说「没有句柄」⇒ 回调不会被触发（同 ManualClock）
      return 0;
    }
    g_callbacks[id].js_handle = js;
    return static_cast<int>(id);
  }
  void del(int handle) override {
    if (handle <= 0) return;
    const auto it = g_callbacks.find(static_cast<uint32_t>(handle));
    if (it == g_callbacks.end()) return;
    if (g_platform.clock_del != nullptr) g_platform.clock_del(g_platform.user, it->second.js_handle);
    g_callbacks.erase(it);
  }
  bool hidden() const override {
    return g_platform.clock_hidden != nullptr && g_platform.clock_hidden(g_platform.user) != 0;
  }
};

// `ITimeout`（`Ditto.Timeout`）：`setTimeout` 语义，同样是一次性。
class JsTimeout final : public lfw::ITimeout {
 public:
  int add(std::function<void()> handler, double timeout) override {
    if (g_platform.timeout_add == nullptr) return 0;
    const uint32_t id = cb_alloc(std::move(handler), /*one_shot=*/true);
    const int32_t js = g_platform.timeout_add(g_platform.user, id, timeout);
    if (js == 0) {
      g_callbacks.erase(id);
      return 0;
    }
    g_callbacks[id].js_handle = js;
    return static_cast<int>(id);
  }
  void del(int timer_id) override {
    if (timer_id <= 0) return;
    const auto it = g_callbacks.find(static_cast<uint32_t>(timer_id));
    if (it == g_callbacks.end()) return;
    if (g_platform.timeout_del != nullptr) {
      g_platform.timeout_del(g_platform.user, it->second.js_handle);
    }
    g_callbacks.erase(it);
  }
};

// `IRenderScheduler`（`Ditto.Render`）：**重复**回调（raf），与上面两个槽不能合并
// （`lfw/base/render_scheduler.h:7-14` 记着混用出过 UAF）。
class JsRender final : public lfw::IRenderScheduler {
 public:
  int add(std::function<void()> handler) override {
    if (g_platform.render_add == nullptr) return 0;
    const uint32_t id = cb_alloc(std::move(handler), /*one_shot=*/false);
    const int32_t js = g_platform.render_add(g_platform.user, id);
    if (js == 0) {
      g_callbacks.erase(id);
      return 0;
    }
    g_callbacks[id].js_handle = js;
    return static_cast<int>(id);
  }
  void del(int handle) override {
    if (handle <= 0) return;
    const auto it = g_callbacks.find(static_cast<uint32_t>(handle));
    if (it == g_callbacks.end()) return;
    if (g_platform.render_del != nullptr) g_platform.render_del(g_platform.user, it->second.js_handle);
    g_callbacks.erase(it);
  }
};

JsClock g_clock;
JsTimeout g_timeout;
JsRender g_render;

// ---------------------------------------------------------------------------
// 宿主：headless 那套原样复用，只把「时钟」与「日志」这五个面接到 JS
// ---------------------------------------------------------------------------

class WasmHost final : public lfw::host::HeadlessHost {
 public:
  WasmHost(std::vector<std::filesystem::path> roots, bool dev)
      : lfw::host::HeadlessHost(std::move(roots), dev) {}

  double now() override { return g_clock.now(); }
  void warn(const std::vector<Value>& args) override {
    ++warns_;
    js_log(LFW_LOG_WARN, join_args(args));
  }
  void error(const std::vector<Value>& args) override {
    ++errors_;
    set_error(join_args(args));
    js_log(LFW_LOG_ERROR, g_error);
  }
  void log(const std::vector<Value>& args) override {
    if (!dev()) return;
    ++logs_;
    js_log(LFW_LOG_LOG, join_args(args));
  }
  void debug_msg(const std::vector<Value>& args) override {
    if (!dev()) return;
    js_log(LFW_LOG_DEBUG, join_args(args));
  }

  uint32_t warns() const { return warns_; }
  uint32_t errors() const { return errors_; }
  uint32_t logs() const { return logs_; }

 private:
  uint32_t warns_ = 0;
  uint32_t errors_ = 0;
  uint32_t logs_ = 0;
};

// ---------------------------------------------------------------------------
// 世界状态
// ---------------------------------------------------------------------------

std::unique_ptr<WasmHost> g_host;
std::unique_ptr<lfw::LFW> g_lfw;
std::vector<std::unique_ptr<lfw::host::FsZip>> g_zips;
std::vector<lfw::LFW::ZipItem> g_items;
uint32_t g_frames = 0;
uint32_t g_probe_hits[3] = {0, 0, 0};

}  // namespace

extern "C" {

uint32_t lfw_abi_version(void) { return LFW_ABI_VERSION; }

uint32_t lfw_scratch_ptr(void) {
  return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(g_scratch.data()));
}

uint32_t lfw_scratch_capacity(void) { return static_cast<uint32_t>(g_scratch.size()); }

int32_t lfw_platform_init(const LfwPlatform* table) {
  if (table == nullptr) return -1;
  if (table->abi_version != LFW_ABI_VERSION) return -3;
  if (table->struct_size < sizeof(LfwPlatform)) return -4;
  g_platform = *table;
  g_platform_ready = true;

  // 三个槽各自独立：JS 少给哪个，哪个就退化成 headless 的「没有句柄」回落。
  lfw::set_clock(&g_clock);
  lfw::set_timeout(g_platform.timeout_add != nullptr ? &g_timeout : nullptr);
  lfw::set_render_scheduler(g_platform.render_add != nullptr ? &g_render : nullptr);
  // 组件自己的告警出口（`lfw/base/no_emit_callbacks.h:17`；`Picture::set_src` 那类用它）。
  lfw::callbacks_warn() = [](const std::u16string& msg) { js_log(LFW_LOG_WARN, msg); };
  return 0;
}

uint32_t lfw_str_put(const lfw_char16* s, uint32_t len) {
  const std::u16string text = from_char16(s, len);
  if (text.empty()) return 0;
  g_strings.push_back(text);
  return static_cast<uint32_t>(g_strings.size());
}

int32_t lfw_create(uint32_t dev) {
  if (!g_platform_ready) return -1;
  if (g_lfw != nullptr) return -2;
  // `--root dist` 的 wasm 等价物：MEMFS 根下就是 `builtin_data/…`（JS 灌进来）。
  g_host = std::make_unique<WasmHost>(std::vector<std::filesystem::path>{std::filesystem::path("/")},
                                      dev != 0);
  g_lfw = std::make_unique<lfw::LFW>(*g_host, dev != 0);
  return 0;
}

int32_t lfw_add_zip(const lfw_char16* memfs_dir, uint32_t len) {
  if (g_lfw == nullptr) return -1;
  if (memfs_dir == nullptr || len == 0) return -2;
  const std::u16string dir = from_char16(memfs_dir, len);
  const std::string dir_u8 = to_utf8(dir);
  std::error_code ec;
  if (!std::filesystem::is_directory(std::filesystem::path(dir_u8), ec)) {
    set_error(u"[wasm] not a directory in MEMFS: " + dir);
    return -3;
  }
  g_zips.push_back(
      std::make_unique<lfw::host::FsZip>(dir, std::filesystem::path(dir_u8)));
  g_items.push_back(lfw::LFW::ZipItem{u"", g_zips.back().get()});
  return 0;
}

int32_t lfw_load(void) {
  if (g_lfw == nullptr) return -1;
  if (g_items.empty()) return -2;
  std::u16string error;
  if (!g_lfw->load(g_items, error)) {
    set_error(error.empty() ? u"[wasm] load failed" : error);
    return -3;
  }
  return 0;
}

int32_t lfw_boot(uint32_t stage_h, uint32_t fighter_a_h, uint32_t fighter_b_h, uint32_t players,
                 uint32_t vs) {
  if (g_lfw == nullptr) return -1;
  lfw::LFW& lfw = *g_lfw;

  // 装配（选 stage/bg → 造战士 → `change_stage`）与 `lfw_run` 共用一份
  // （`host/common/lfw_boot.h`）：这边只把日志文本与"有错就中止"的失败策略接回去。
  lfw::host::BootOptions options;
  if (const std::u16string* const stage_arg = string_at(stage_h)) options.stage = *stage_arg;
  if (const std::u16string* const a = string_at(fighter_a_h)) options.fighters.push_back(*a);
  if (const std::u16string* const b = string_at(fighter_b_h)) options.fighters.push_back(*b);
  options.players = static_cast<int>(players);
  options.vs = vs != 0;
  options.on_spawn = [](const lfw::host::SpawnInfo& s) {
    js_log(LFW_LOG_LOG,
           u"[wasm] spawn p" + s.player_id + u" " + s.fighter_id + (s.vs ? u" (vs)" : u""));
  };
  bool failed = false;
  bool create_failed = false;
  std::u16string failed_id;
  options.on_failure = [&](const lfw::host::BootFailure& f) {
    failed = true;
    create_failed = f.create_failed;
    failed_id = f.fighter_id;
    return false;  // 有错就中止（等价于原来在这里 `return -4` / `return -5`）
  };

  const lfw::host::BootResult booted = lfw::host::boot_scene(lfw, options);
  if (!booted.ok) {
    set_error(u"[wasm] no fighter data in the loaded packages");
    return -3;
  }
  if (failed) {
    set_error((create_failed ? u"[wasm] create fighter failed: " : u"[wasm] fighter data not found: ") +
              failed_id);
    return create_failed ? -5 : -4;
  }
  js_log(LFW_LOG_LOG, u"[wasm] boot stage=" + booted.stage_id + u" bg=" + booted.bg_id +
                           u" entities=" +
                           lfw::number_to_string(static_cast<double>(lfw.world().entities.size())));
  return 0;
}

int32_t lfw_frame(double dt_ms) {
  if (g_lfw == nullptr) return -1;
  if (!(dt_ms > 0.0)) return -2;  // dt 由 JS 给；与时钟推进量必须一致（W8）
  g_lfw->world().update_once(dt_ms);
  ++g_frames;
  return 0;
}

double lfw_clock_now(void) { return g_clock.now(); }

uint32_t lfw_frames(void) { return g_frames; }

uint32_t lfw_entities(void) {
  return g_lfw != nullptr ? static_cast<uint32_t>(g_lfw->world().entities.size()) : 0;
}

uint32_t lfw_ghosts(void) {
  return g_lfw != nullptr ? static_cast<uint32_t>(g_lfw->world().ghosts.size()) : 0;
}

double lfw_lifetime(void) { return g_lfw != nullptr ? g_lfw->world().lifetime() : 0.0; }

double lfw_total_hp(void) {
  return g_lfw != nullptr ? lfw::host::total_hp(g_lfw->world()) : 0.0;
}

uint32_t lfw_warn_count(void) { return g_host != nullptr ? g_host->warns() : 0; }
uint32_t lfw_error_count(void) { return g_host != nullptr ? g_host->errors() : 0; }
uint32_t lfw_log_count(void) { return g_host != nullptr ? g_host->logs() : 0; }

uint32_t lfw_data_count(uint32_t which) {
  if (g_lfw == nullptr) return 0;
  const lfw::loader::DatMgr& d = g_lfw->datas();
  switch (which) {
    case 0: return static_cast<uint32_t>(d.fighters().size());
    case 1: return static_cast<uint32_t>(d.weapons().size());
    case 2: return static_cast<uint32_t>(d.balls().size());
    case 3: return static_cast<uint32_t>(d.entities().size());
    case 4: return static_cast<uint32_t>(d.stages().size());
    case 5: return static_cast<uint32_t>(d.backgrounds().size());
    case 6: return static_cast<uint32_t>(d.bots().size());
    case 7: return static_cast<uint32_t>(d.moves().size());
    default: return 0;
  }
}

uint32_t lfw_digest(void) {
  if (g_lfw == nullptr) {
    write_scratch(u"");
    return 0;
  }
  const int frame = g_frames == 0 ? 0 : static_cast<int>(g_frames) - 1;
  const std::string hex = lfw::host::frame_digest(g_lfw->world(), frame);
  const std::u16string text = lfw::decode_utf8(std::vector<uint8_t>(hex.begin(), hex.end()));
  write_scratch(text);
  return static_cast<uint32_t>(text.size());
}

uint32_t lfw_dump_state(void) {
  if (g_lfw == nullptr) {
    write_scratch(u"");
    return 0;
  }
  // 与 `lfw_run.cpp` 的 `[  state]` 行同款（`%.1f` / `%.0f` 的格式也一致）。
  const lfw::World& world = g_lfw->world();
  std::u16string out = u"[state]";
  char buf[64];
  for (const Entity* const e : world.entities) {
    if (e == nullptr) continue;
    const std::u16string state = lfw::to_string(e->state());
    const std::u16string frame = lfw::to_string(e->frame_id());
    std::snprintf(buf, sizeof(buf), "%.1f", e->position.x);
    const std::u16string x = lfw::decode_utf8(std::vector<uint8_t>(buf, buf + std::strlen(buf)));
    std::snprintf(buf, sizeof(buf), "%.1f", e->position.y);
    const std::u16string y = lfw::decode_utf8(std::vector<uint8_t>(buf, buf + std::strlen(buf)));
    std::snprintf(buf, sizeof(buf), "%.0f", e->hp());
    const std::u16string hp = lfw::decode_utf8(std::vector<uint8_t>(buf, buf + std::strlen(buf)));
    out += u" " + e->id + u"{state=" + state + u" frame=" + frame + u" x=" + x + u" y=" + y +
           u" hp=" + hp + u" bot=" + std::u16string(lfw::truthy(Value(e->ctrl_is_bot())) ? u"1" : u"0") +
           u" m=" + std::u16string(lfw::truthy(Value(e->mounted())) ? u"1" : u"0") + u" g=" +
           std::u16string(lfw::truthy(Value(e->ghosted())) ? u"1" : u"0") + u"}";
  }
  if (const lfw::Background* const bg = world.bg()) {
    std::snprintf(buf, sizeof(buf), "%.1f", bg->update_times());
    const std::u16string ut = lfw::decode_utf8(std::vector<uint8_t>(buf, buf + std::strlen(buf)));
    std::snprintf(buf, sizeof(buf), "%.1f", bg->layer_data_index());
    const std::u16string li = lfw::decode_utf8(std::vector<uint8_t>(buf, buf + std::strlen(buf)));
    std::snprintf(buf, sizeof(buf), "%.1f", bg->left());
    const std::u16string lf = lfw::decode_utf8(std::vector<uint8_t>(buf, buf + std::strlen(buf)));
    std::snprintf(buf, sizeof(buf), "%.1f", bg->right());
    const std::u16string rt = lfw::decode_utf8(std::vector<uint8_t>(buf, buf + std::strlen(buf)));
    out += u" | bg{update_times=" + ut + u" layer_data_index=" + li + u" left=" + lf +
           u" right=" + rt + u"}";
  }
  write_scratch(out);
  return static_cast<uint32_t>(out.size());
}

uint32_t lfw_last_error_len(void) {
  write_scratch(g_error);
  return static_cast<uint32_t>(g_error.size());
}

void lfw_invoke(uint32_t cb_id) {
  const auto it = g_callbacks.find(cb_id);
  if (it == g_callbacks.end()) return;
  const CbEntry entry = it->second;  // 拷贝：回调里可能 `del` 自己
  if (entry.one_shot) g_callbacks.erase(it);
  if (entry.fn) entry.fn();
}

int32_t lfw_probe_schedule(double timeout_ms) {
  lfw::clock_add([]() { ++g_probe_hits[0]; });
  lfw::timeout_add([]() { ++g_probe_hits[1]; }, timeout_ms);
  lfw::render_add([]() { ++g_probe_hits[2]; });
  return 0;
}

uint32_t lfw_probe_hits(uint32_t which) { return which < 3 ? g_probe_hits[which] : 0; }

void lfw_shutdown(void) {
  g_lfw.reset();
  g_host.reset();
  g_items.clear();
  g_zips.clear();
  g_callbacks.clear();
  g_strings.clear();
  g_error.clear();
  g_frames = 0;
  g_platform = LfwPlatform{};
  g_platform_ready = false;
  lfw::set_clock(nullptr);
  lfw::set_timeout(nullptr);
  lfw::set_render_scheduler(nullptr);
  lfw::callbacks_warn() = nullptr;
}

}  // extern "C"
