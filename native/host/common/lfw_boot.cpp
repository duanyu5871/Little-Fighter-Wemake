#include "host/common/lfw_boot.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

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
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/mersenne_twister.h"
#include "lfw/utils/utf8.h"
#include "lfw/world.h"

namespace lfw::host {
namespace {

std::string to_utf8(const std::u16string& s) {
  const std::vector<uint8_t> buf = lfw::encode_utf8(s);
  return std::string(buf.begin(), buf.end());
}

const std::u16string* as_str(const Value& v) { return std::get_if<std::u16string>(&v); }

// `world.dataset.get(k)` 缺省时（`undefined`）给 TS 的缺省值。
double num_field(const Value& v, double fallback) {
  if (std::holds_alternative<std::monostate>(v)) return fallback;
  const double n = lfw::to_number(v);
  return std::isnan(n) ? fallback : n;
}

}  // namespace

BootResult boot_scene(lfw::LFW& lfw, const BootOptions& options) {
  BootResult result;
  lfw::World& world = lfw.world();
  const bool is_vs = options.vs;

  // ---- 选舞台 / 背景（stage 数据里的 `bg`；缺省取第一个舞台）----
  // ⚠ `field_or` **按值**返回 `Value`：`as_str(field_or(...))` 拿到的是临时对象内部的指针，
  // 语句一结束就悬空（踩过一次：读回空串）。凡是要留指针的地方都先存进局部 `Value`。
  std::u16string stage_id = options.stage;
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
    if (bg != nullptr) result.bg_id = *bg;
  }
  if (!result.bg_id.empty()) lfw.change_bg(result.bg_id);

  // ---- 出场的角色（给了就用给的，否则取数据包里的前 `players` 个）----
  std::vector<std::u16string> fighter_ids = options.fighters;
  if (fighter_ids.empty()) {
    for (const Value& f : lfw.datas().fighters()) {
      const Value id_v = lfw::field_or(f, u"id");
      const std::u16string* const id = as_str(id_v);
      if (id != nullptr && !id->empty()) fighter_ids.push_back(*id);
      if (static_cast<int>(fighter_ids.size()) >= options.players) break;
    }
  }
  if (fighter_ids.empty()) return result;  // `ok` 仍是 false

  result.ok = true;

  const double begin_blink = num_field(world.dataset.get(u"begin_blink_time"), 0.0);
  for (int i = 0; i < options.players; ++i) {
    const std::u16string& fid = fighter_ids[static_cast<size_t>(i) % fighter_ids.size()];
    const Value data = lfw.datas_fighters_find(Value(fid));
    if (!lfw::truthy(data)) {
      if (options.on_failure != nullptr && !options.on_failure(BootFailure{fid, false})) return result;
      continue;
    }
    const std::u16string player_id = lfw::number_to_string(static_cast<double>(i) + 1.0);
    Entity* const fighter = lfw.create_entity_with_bot(player_id, world, data);
    if (fighter == nullptr) {
      if (options.on_failure != nullptr && !options.on_failure(BootFailure{fid, true})) return result;
      continue;
    }
    if (is_vs) {
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
      if (options.on_spawn != nullptr) {
        SpawnInfo info;
        info.player_id = player_id;
        info.fighter_id = fid;
        info.vs = true;
        info.x = x;
        info.y = y;
        info.z = z;
        info.team = fighter->team();
        options.on_spawn(info);
      }
    } else {
      fighter->set_team(lfw::number_to_string(static_cast<double>(i) + 1.0));
      fighter->stat_bar = 0;
      fighter->facing = (i % 2 == 0) ? 1 : -1;
      fighter->name_visible = 1;
      fighter->as_key_role(Value(true));
      fighter->set_hp(500);
      fighter->set_hp_max(500);
      fighter->set_mp(500);
      fighter->set_mp_max(500);
      const double x = -80.0 + 160.0 * static_cast<double>(i);
      const double z = 0.0;
      const lfw::ITerrainInfo& seg = world.ground.segment(x, z);
      const double y = lfw::Ground::y(seg, x, z);
      fighter->set_position(Value(x), Value(y), Value(z));
      fighter->set_blinking(begin_blink);
      fighter->set_invulnerable(begin_blink);
      fighter->attach();
      if (options.on_spawn != nullptr) {
        SpawnInfo info;
        info.player_id = player_id;
        info.fighter_id = fid;
        info.vs = false;
        info.x = x;
        info.y = y;
        info.z = z;
        info.team = fighter->team();
        options.on_spawn(info);
      }
    }
  }

  if (!stage_id.empty()) lfw.change_stage(stage_id);
  if (is_vs) {
    // TS `VsModeLogic.on_start`：`world.paused = false`、`dataset.playrate = 1`、
    // `dataset.infinity_mp = 0`（对战模式没有闯关的阶段/难度那套）。
    world.set_paused(false);
    world.dataset.set(u"playrate", Value(1.0));
    world.dataset.set(u"infinity_mp", Value(0.0));
  }
  result.stage_id = stage_id;
  return result;
}

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

}  // namespace lfw::host
