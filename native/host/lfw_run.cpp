// `lfw_run`：headless 入口 —— 把 `native/host/headless_host.h` 的宿主接到 `LFW` 上，
// 加载真实数据包（`--zip` 给的已解包目录 + `--root` 下的 `builtin_data/`），起一场
// bot 对 bot 的对局，然后**手动推帧**（`World::update_once`），每 `--every` 帧打一行摘要。
//
// 用法：
//   lfw_run --root <dir> --zip <dir> [--zip <dir> ...] [--stage <id>] [--frames 600]
//           [--players 2] [--fighter <id>] [--time <ms>] [--every 60] [--dev]
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "host/fs_zip.h"
#include "host/headless_host.h"
#include "lfw/bg/background.h"
#include "lfw/core/state_hash.h"
#include "lfw/core/json5.h"
#include "lfw/core/value.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/facing_flag.h"
#include "lfw/lfw.h"
#include "lfw/stage/stage.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/utf8.h"

namespace {

using lfw::Array;
using lfw::Entity;
using lfw::Object;
using lfw::Value;

struct Args {
  std::vector<std::filesystem::path> roots;
  std::vector<std::filesystem::path> zips;
  std::vector<std::u16string> fighters;
  std::u16string stage;
  int frames = 600;
  int players = 2;
  int every = 60;
  double time = 0;
  bool dev = false;
  bool dump = false;
  // `--vs`：按 TS 的「对战模式」构造场景（`GamePrepareLogic.start_game` 的 VS 分支 +
  // `VsModeLogic.on_start`），而不是宿主原来的闯关模式口径。
  bool vs = false;
};

std::string to_utf8(const std::u16string& s) {
  const std::vector<uint8_t> buf = lfw::encode_utf8(s);
  return std::string(buf.begin(), buf.end());
}

// `Value` 里取字符串（`lfw` 侧同名的 `as_str` 是 lfw.cpp 的文件内助手，不是公开 API）。
const std::u16string* as_str(const Value& v) { return std::get_if<std::u16string>(&v); }

// `world.dataset.get(k)` 缺省时（`undefined`）给 TS 的缺省值。
double num_field(const Value& v, double fallback) {
  if (std::holds_alternative<std::monostate>(v)) return fallback;
  const double n = lfw::to_number(v);
  return std::isnan(n) ? fallback : n;
}

void usage() {
  std::printf(
      "usage: lfw_run --root <dir> --zip <dir> [--zip <dir> ...] [--stage <id>] [--frames N]\n"
      "               [--players N] [--fighter <id>] [--time ms] [--every N] [--dev]\n"
      "  --root    import 根目录（含 builtin_data/…），可重复\n"
      "  --zip     数据包目录（已解包），可重复；按给定顺序 load\n"
      "  --fighter 指定出场的角色 id，可重复（缺省用数据包里的前几个）\n");
}

bool parse_args(int argc, char** argv, Args& out) {
  const auto need = [&](int& i) -> const char* {
    if (i + 1 >= argc) return nullptr;
    return argv[++i];
  };
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--root") {
      const char* v = need(i);
      if (v == nullptr) return false;
      out.roots.emplace_back(v);
    } else if (a == "--zip") {
      const char* v = need(i);
      if (v == nullptr) return false;
      out.zips.emplace_back(v);
    } else if (a == "--fighter") {
      const char* v = need(i);
      if (v == nullptr) return false;
      out.fighters.push_back(lfw::decode_utf8(std::vector<uint8_t>(v, v + std::strlen(v))));
    } else if (a == "--stage") {
      const char* v = need(i);
      if (v == nullptr) return false;
      out.stage = lfw::decode_utf8(std::vector<uint8_t>(v, v + std::strlen(v)));
    } else if (a == "--frames") {
      const char* v = need(i);
      if (v == nullptr) return false;
      out.frames = std::atoi(v);
    } else if (a == "--players") {
      const char* v = need(i);
      if (v == nullptr) return false;
      out.players = std::atoi(v);
    } else if (a == "--every") {
      const char* v = need(i);
      if (v == nullptr) return false;
      out.every = std::atoi(v);
    } else if (a == "--time") {
      const char* v = need(i);
      if (v == nullptr) return false;
      out.time = std::atof(v);
    } else if (a == "--dev") {
      out.dev = true;
    } else if (a == "--dump") {
      out.dump = true;
    } else if (a == "--vs") {
      out.vs = true;
    } else if (a == "--help" || a == "-h") {
      usage();
      std::exit(0);
    } else {
      std::fprintf(stderr, "unknown argument: %s\n", a.c_str());
      return false;
    }
  }
  if (out.zips.empty()) return false;
  if (out.roots.empty()) out.roots.emplace_back(".");
  return true;
}

// 一帧的可复现摘要：实体表（按 id 排序）+ 位置 / 血量 / 队 / 状态。
std::string frame_digest(const lfw::World& world, int frame) {
  std::vector<const Entity*> list;
  for (const Entity* const e : world.entities) {
    if (e != nullptr) list.push_back(e);
  }
  std::sort(list.begin(), list.end(), [](const Entity* a, const Entity* b) { return a->id < b->id; });
  lfw::StateHash h;
  h.str("lfw_run");
  h.i32(frame);
  h.u32(static_cast<uint32_t>(list.size()));
  for (const Entity* const e : list) {
    h.str(to_utf8(e->id));
    h.f64(e->position.x);
    h.f64(e->position.y);
    h.f64(e->position.z);
    h.f64(e->hp());
    h.f64(e->mp());
    h.str(to_utf8(e->team()));
    h.str(to_utf8(lfw::to_string(e->state())));
  }
  return h.hex();
}

double total_hp(const lfw::World& world) {
  double sum = 0;
  for (const Entity* const e : world.entities) {
    if (e != nullptr) sum += e->hp();
  }
  return sum;
}

}  // namespace

int main(int argc, char** argv) {
  std::setvbuf(stdout, nullptr, _IONBF, 0);  // 崩了也要看得见走到哪一步
  Args args;
  if (!parse_args(argc, argv, args)) {
    usage();
    return 64;
  }

  lfw::host::ManualClock clock;
  lfw::host::NoopTimeout timeout;
  lfw::set_clock(&clock);
  lfw::set_timeout(&timeout);
  clock.set(args.time);

  lfw::host::HeadlessHost host(args.roots, args.dev);

  std::vector<std::unique_ptr<lfw::host::FsZip>> zips;
  std::vector<lfw::LFW::ZipItem> items;
  for (const std::filesystem::path& dir : args.zips) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
      std::fprintf(stderr, "not a directory: %s\n", dir.generic_string().c_str());
      return 66;
    }
    std::printf("[lfw_run] indexing %s ...\n", dir.generic_string().c_str());
    const std::string dir_name = dir.generic_string();
    const std::u16string zip_name =
        lfw::decode_utf8(std::vector<uint8_t>(dir_name.begin(), dir_name.end()));
    zips.push_back(std::make_unique<lfw::host::FsZip>(zip_name, dir));
    items.push_back(lfw::LFW::ZipItem{u"", zips.back().get()});
    std::printf("[lfw_run] package: %s (%zu entries)\n", dir.generic_string().c_str(),
                zips.back()->entry_count());
  }

  std::printf("[lfw_run] constructing LFW ...\n");
  lfw::LFW lfw(host, args.dev);

  std::u16string error;
  std::printf("[lfw_run] loading ...\n");
  if (!lfw.load(items, error)) {
    std::fprintf(stderr, "[lfw_run] load failed: %s\n", to_utf8(error).c_str());
    for (const std::u16string& e : host.last_errors()) {
      std::fprintf(stderr, "  - %s\n", to_utf8(e).c_str());
    }
    return 2;
  }
  std::printf(
      "[lfw_run] loaded: fighters=%zu weapons=%zu balls=%zu entities=%zu stages=%zu bg=%zu bots=%zu "
      "moves=%zu\n",
      lfw.datas().fighters().size(), lfw.datas().weapons().size(), lfw.datas().balls().size(),
      lfw.datas().entities().size(), lfw.datas().stages().size(),
      lfw.datas().backgrounds().size(), lfw.datas().bots().size(), lfw.datas().moves().size());
  if (args.dump) {
    const auto dump = [](const char* tag, const std::vector<Value>& list) {
      for (size_t i = 0; i < list.size() && i < 2; ++i) {
        const lfw::Json5TextResult r = lfw::json5_stringify(list[i]);
        std::u16string s = r.ok ? r.text : lfw::to_string(list[i]);
        if (s.size() > 300) s = s.substr(0, 300) + u"...";
        std::printf("[dump] %s[%zu] = %s\n", tag, i, to_utf8(s).c_str());
      }
    };
    dump("stage", lfw.datas().stages());
    dump("fighter", lfw.datas().fighters());
    dump("bg", lfw.datas().backgrounds());
    // 所有 id 一行一个：挑 `--fighter` / `--stage` 时用。
    const auto ids = [](const char* tag, const std::vector<Value>& list) {
      std::printf("[ids] %s (%zu):", tag, list.size());
      for (const Value& v : list) {
        const Value id_v = lfw::field_or(v, u"id");
        const std::u16string* const id = as_str(id_v);
        std::printf(" %s", id != nullptr ? to_utf8(*id).c_str() : "?");
      }
      std::printf("\n");
    };
    ids("stage", lfw.datas().stages());
    ids("fighter", lfw.datas().fighters());
    ids("bg", lfw.datas().backgrounds());
  }

  lfw::World& world = lfw.world();

  // ---- 选舞台 / 背景（stage 数据里的 `bg`；缺省取第一个舞台）----
  // ⚠ `field_or` **按值**返回 `Value`：`as_str(field_or(...))` 拿到的是临时对象内部的指针，
  // 语句一结束就悬空（踩过一次：读回空串）。凡是要留指针的地方都先存进局部 `Value`。
  std::u16string stage_id = args.stage;
  std::u16string bg_id;
  if (!lfw.datas().stages().empty()) {
    const Value* stage = nullptr;
    for (const Value& s : lfw.datas().stages()) {
      const Value id_v = lfw::field_or(s, u"id");
      const std::u16string* const id = as_str(id_v);
      if (id == nullptr) continue;
      if (!stage_id.empty()) {
        if (*id == stage_id) {
          stage = &s;
          break;
        }
        continue;
      }
      const bool starting = lfw::truthy(lfw::field_or(s, u"is_starting"));
      if (stage == nullptr || starting) stage = &s;
      if (starting) break;
    }
    if (stage == nullptr) stage = &lfw.datas().stages().front();
    if (stage_id.empty()) {
      const Value id_v = lfw::field_or(*stage, u"id");
      const std::u16string* const id = as_str(id_v);
      if (id != nullptr) stage_id = *id;
    }
    const Value bg_v = lfw::field_or(*stage, u"bg");
    const std::u16string* const bg = as_str(bg_v);
    if (bg != nullptr) bg_id = *bg;
  }
  if (!bg_id.empty()) lfw.change_bg(bg_id);

  // ---- 出场的角色（`GamePrepareLogic::start_game` 的最小版）----
  std::vector<std::u16string> fighter_ids = args.fighters;
  if (fighter_ids.empty()) {
    for (const Value& f : lfw.datas().fighters()) {
      const Value id_v = lfw::field_or(f, u"id");
      const std::u16string* const id = as_str(id_v);
      if (id != nullptr && !id->empty()) fighter_ids.push_back(*id);
      if (static_cast<int>(fighter_ids.size()) >= args.players) break;
    }
  }
  if (fighter_ids.empty()) {
    std::fprintf(stderr, "[lfw_run] no fighter data in the loaded packages\n");
    return 3;
  }

  const double begin_blink = num_field(world.dataset.get(u"begin_blink_time"), 0.0);
  for (int i = 0; i < args.players; ++i) {
    const std::u16string& fid = fighter_ids[static_cast<size_t>(i) % fighter_ids.size()];
    const Value data = lfw.datas_fighters_find(Value(fid));
    if (!lfw::truthy(data)) {
      std::fprintf(stderr, "[lfw_run] fighter data not found: %s\n", to_utf8(fid).c_str());
      continue;
    }
    const std::u16string player_id = lfw::number_to_string(i + 1);
    Entity* const fighter = lfw.create_entity_with_bot(player_id, world, data);
    if (fighter == nullptr) {
      std::fprintf(stderr, "[lfw_run] create fighter failed: %s\n", to_utf8(fid).c_str());
      continue;
    }
    if (args.vs) {
      // TS `GamePrepareLogic.start_game` 的**对战模式**分支（`is_stage_mode === false`）。
      // 队伍走 `lfw.new_team`（自增 `team_N`）；朝向是 `mt.pick([Left, Right])`；
      // 站位 `x = mt.range(screen_w/3, 2*screen_w/3) + cam_x`（宿主没有准备页相机 ⇒ `cam_x = 0`）、
      // `z = mt.range(far, near)`；hp/mp **不覆盖**（保持 fighter 数据里的值），
      // 只把 `mp` 收到 `mp_max * 2 / 5`（TS 的 `if (is_vs_mode)` 那一行）。
      fighter->set_team(lfw.new_team());
      fighter->stat_bar = 0;
      const std::vector<Value> facing_options{
          Value(static_cast<double>(lfw::FacingFlag::Left)),
          Value(static_cast<double>(lfw::FacingFlag::Right))};
      const Value facing_pick =
          lfw.mt()->pick_value(Value(std::make_shared<Array>(Array(facing_options))));
      fighter->facing = num_field(facing_pick, 1.0);
      fighter->name_visible = 1;
      fighter->as_key_role(Value(true));
      lfw.mt()->mark = u"gpl_fighter_x";
      const double screen_w = lfw::defines::num(u"Defines.MODERN_SCREEN_WIDTH");
      const double x = lfw.mt()->range(screen_w / 3.0, 2.0 * screen_w / 3.0);
      lfw.mt()->mark = u"gpl_fighter_z";
      const lfw::Background* const bg = world.bg();
      const double z = bg != nullptr ? lfw.mt()->range(bg->far_plane(), bg->near_plane()) : 0.0;
      const lfw::ITerrainInfo& seg = world.ground.segment(x, z);
      const double y = lfw::Ground::y(seg, x, z);
      fighter->set_position(Value(x), Value(y), Value(z));
      fighter->set_blinking(begin_blink);
      fighter->set_invulnerable(begin_blink);
      fighter->set_mp(fighter->mp_max() * 2.0 / 5.0);
      fighter->attach();
      std::printf("[lfw_run] spawn p%s %s @(%.1f, %.1f, %.1f) team=%s (vs)\n",
                  to_utf8(player_id).c_str(), to_utf8(fid).c_str(), x, y, z, to_utf8(fighter->team()).c_str());
    } else {
      fighter->set_team(lfw::number_to_string(i + 1));
      fighter->stat_bar = 0;
      fighter->facing = (i % 2 == 0) ? 1 : -1;
      fighter->name_visible = 1;
      fighter->as_key_role(Value(true));
      fighter->set_hp(500);
      fighter->set_hp_max(500);
      fighter->set_mp(500);
      fighter->set_mp_max(500);
      const double x = -80.0 + 160.0 * i;
      const double z = 0.0;
      const lfw::ITerrainInfo& seg = world.ground.segment(x, z);
      const double y = lfw::Ground::y(seg, x, z);
      fighter->set_position(Value(x), Value(y), Value(z));
      fighter->set_blinking(begin_blink);
      fighter->set_invulnerable(begin_blink);
      fighter->attach();
      std::printf("[lfw_run] spawn p%s %s @(%.1f, %.1f, %.1f) team=%s\n", to_utf8(player_id).c_str(),
                  to_utf8(fid).c_str(), x, y, z, to_utf8(fighter->team()).c_str());
    }
  }

  if (!stage_id.empty()) lfw.change_stage(stage_id);
  if (args.vs) {
    // TS `VsModeLogic.on_start`：`world.paused = false`、`dataset.playrate = 1`、
    // `dataset.infinity_mp = 0`（对战模式没有闯关的阶段/难度那套）。
    world.set_paused(false);
    world.dataset.set(u"playrate", Value(1.0));
    world.dataset.set(u"infinity_mp", Value(0.0));
  }
  std::printf("[lfw_run] stage=%s bg=%s entities=%zu\n", to_utf8(stage_id).c_str(),
              to_utf8(bg_id).c_str(), world.entities.size());

  // ---- 诊断：bg / stage / 每个实体的 bound -------------------------------------------
  // bot 的 `get_bound()`（`can_back_off` / `cornered` / `should_chase`）与
  // `lock_when_stand_and_rest`（bg 的 width/near/far）都吃这些值，所以先打出来。
  if (const lfw::Background* const bg = world.bg()) {
    std::printf(
        "[bg] id=%s name=%s size=(%.1f, %.1f, %.1f) planes=(near=%.1f far=%.1f) "
        "left/right=(%.1f, %.1f) zoom=(%.2f, %.2f, %.2f) layers=%zu update_times=%.1f "
        "layer_data_index=%.1f\n",
        to_utf8(lfw::to_string(bg->id())).c_str(), to_utf8(lfw::to_string(bg->name())).c_str(),
        bg->width(), bg->height(), bg->depth(), bg->near_plane(), bg->far_plane(), bg->left(),
        bg->right(), bg->zoom_x(), bg->zoom_y(), bg->zoom_z(), bg->layers().size(),
        bg->update_times(), bg->layer_data_index());
  } else {
    std::printf("[bg] none\n");
  }
  if (const lfw::Stage* const stage = world.stage()) {
    std::printf(
        "[stage] team=%s player=(%.1f, %.1f) enemy=(%.1f, %.1f) drink=(%.1f, %.1f) "
        "planes=(near=%.1f far=%.1f)\n",
        to_utf8(lfw::to_string(stage->team())).c_str(), stage->player_l, stage->player_r,
        stage->enemy_l, stage->enemy_r, stage->drink_l, stage->drink_r, stage->near_plane(),
        stage->far_plane());
  } else {
    std::printf("[stage] none\n");
  }
  for (Entity* const e : world.entities) {
    if (e == nullptr) continue;
    const std::vector<double> b = world.get_bound(*e);
    std::printf("[bound] %s team=%s type=%s bot=%d l=%.1f r=%.1f near=%.1f far=%.1f\n",
                to_utf8(e->id).c_str(), to_utf8(e->team()).c_str(),
                to_utf8(lfw::to_string(e->data_type())).c_str(), e->ctrl_is_bot() ? 1 : 0,
                b.size() > 0 ? b[0] : 0.0, b.size() > 1 ? b[1] : 0.0, b.size() > 2 ? b[2] : 0.0,
                b.size() > 3 ? b[3] : 0.0);
  }

  // ---- 推帧 ----
  const double ups = num_field(world.dataset.get(u"UPS"), 60.0);
  const double dt = 1000.0 / (ups > 0 ? ups : 60.0);
  const int every = args.every > 0 ? args.every : 60;
  std::string digest;
  for (int frame = 0; frame < args.frames; ++frame) {
    clock.advance(dt);
    world.update_once(dt);
    if (frame % every == 0 || frame == args.frames - 1) {
      digest = frame_digest(world, frame);
      std::printf("[frame %6d] entities=%zu ghosts=%zu hp=%.1f digest=%s\n", frame,
                  world.entities.size(), world.ghosts.size(), total_hp(world), digest.c_str());
      // 诊断行：各实体的 state / frame.id / 位置 + bg 运动（看帧链有没有在推进）
      std::printf("[  state]");
      for (Entity* const e : world.entities) {
        if (e == nullptr) continue;
        std::printf(" %s{state=%s frame=%s x=%.1f y=%.1f hp=%.0f bot=%d}", to_utf8(e->id).c_str(),
                    to_utf8(lfw::to_string(e->state())).c_str(),
                    to_utf8(lfw::to_string(e->frame_id())).c_str(), e->position.x, e->position.y,
                    e->hp(), e->ctrl_is_bot() ? 1 : 0);
      }
      if (const lfw::Background* const bg = world.bg()) {
        std::printf(" | bg{update_times=%.1f layer_data_index=%.1f left=%.1f right=%.1f}",
                    bg->update_times(), bg->layer_data_index(), bg->left(), bg->right());
      }
      std::printf("\n");
    }
  }

  std::printf(
      "[lfw_run] done: frames=%d ups=%.0f digest=%s warns=%d errors=%d ignored_images=%lld "
      "renderer(+-)=%lld/%lld clock=%.1fms\n",
      args.frames, ups, digest.c_str(), host.warn_count(), host.error_count(),
      host.ignored_images(), host.renderer().added, host.renderer().removed, clock.now());
  for (const Entity* const e : world.entities) {
    if (e == nullptr) continue;
    std::printf("[lfw_run] final %s team=%s pos=(%.1f, %.1f, %.1f) hp=%.1f/%.1f mp=%.1f state=%s\n",
                to_utf8(e->id).c_str(), to_utf8(e->team()).c_str(), e->position.x, e->position.y,
                e->position.z, e->hp(), e->hp_max(), e->mp(), to_utf8(lfw::to_string(e->state())).c_str());
  }
  return 0;
}
