#include "lfw/world.h"
#include "lfw/world_collision.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "lfw/base/clock.h"
#include "lfw/base/render_scheduler.h"
#include "lfw/base/ticker.h"
#include "lfw/collision/keeper.h"
#include "lfw/core/same_ref.h"
#include "lfw/defines/background_group.h"
#include "lfw/defines/cheat_type.h"
#include "lfw/defines/defines.h"
#include "lfw/defines/defines_data.h"
#include "lfw/defines/frame_id.h"
#include "lfw/defines/gone_frame_info.h"
#include "lfw/defines/oid.h"
#include "lfw/defines/state_enum.h"
#include "lfw/defines/sync_render_enum.h"
#include "lfw/defines/weapon_type.h"
#include "lfw/entity/entity_ref.h"
#include "lfw/entity/entity_type_check.h"
#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"
#include "lfw/utils/math/round_float.h"

namespace lfw {
namespace {

// `World.ts` 顶部的四个常量。
constexpr double kLookupUpdateInterval = 2.0;
constexpr double kMaxDebugEntities = 355.0;
constexpr double kMaxStepErrors = 120.0;
constexpr double kWeaponXSection = 750.0;

// `x_sorter`：`aabb_min_x` 相等时按 `id` 比（TS 用 `<` / `>` 判字符串）。
bool x_sorter(Entity* const a, Entity* const b) {
  const double d = a->aabb_min_x - b->aabb_min_x;
  if (d != 0) return d < 0;
  return a->id < b->id;
}

// `is_bot_ctrl(e.ctrl)` / `is_human_ctrl(...)` / `is_ball_ctrl(...)`：端口里控制器是对象
// （`BaseController`），TS 读的是 `ctrl.__is_*_ctrl__` 三个布尔 ⇒ 一一对应。
bool is_bot_ctrl_ptr(const controller::BaseController* const ctrl) {
  return ctrl != nullptr && ctrl->is_bot();
}
bool is_human_ctrl_ptr(const controller::BaseController* const ctrl) {
  return ctrl != nullptr && ctrl->is_human();
}
bool is_ball_ctrl_ptr(const controller::BaseController* const ctrl) {
  return ctrl != nullptr && ctrl->is_ball_ctrl();
}

// `Defines.<NAME>`：常量表里取现成的 `Value`（拿不到就是 `undefined`）。
Value defines_value(const char16_t* const name) {
  const Value* const found = defines::find(std::u16string(name));
  return found != nullptr ? *found : Value();
}

// `a.frame.id === FID.Gone`
bool frame_is_gone(const Entity& e) {
  return strict_equals(field_or(e.frame, u"id"), Value(std::u16string(frame_id::kGone)));
}

// `a.frame.id === FID.Gone || a.state === SE.Gone`
bool entity_is_gone(const Entity& e) {
  if (frame_is_gone(e)) return true;
  return strict_equals(e.state(), Value(static_cast<double>(StateEnum::Gone)));
}

// `this._gones` 是 `Set<Entity>`：`add` 判重（保插入序），`has` 看成员。
bool gones_has(const std::vector<Entity*>& v, const Entity* const e) {
  return std::find(v.begin(), v.end(), e) != v.end();
}
void gones_add(std::vector<Entity*>& v, Entity* const e) {
  if (!gones_has(v, e)) v.push_back(e);
}

// 异常文本转 `u16string` 的工具在 4L 接 `LFW` 时再回来（TS 的 `Ticker` 在 `on_step` 里
// `try/catch` 把异常交给 `World.on_step_error`；端口不装异常 ⇒ 这一层由宿主在驱动 `on_step`
// 时做，见 §77.3）。

Value obj_of() { return Value(std::make_shared<Object>()); }

// `e.ctrl` 的 `Value` 视图（TS 里控制器是对象，`is_bot_ctrl(ctrl)` / `ctrl.player_id` 都读它）。
Value ctrl_value(const controller::BaseController* const ctrl) {
  if (ctrl == nullptr) return Value();
  auto o = std::make_shared<Object>();
  o->set(u"__is_base_ctrl__", Value(true));
  o->set(u"__is_bot_ctrl__", Value(ctrl->is_bot()));
  o->set(u"__is_human_ctrl__", Value(ctrl->is_human()));
  o->set(u"__is_ball_ctrl__", Value(ctrl->is_ball_ctrl()));
  o->set(u"player_id", Value(ctrl->player_id));
  o->set(u"player", ctrl->player);
  return Value(o);
}

// `Stage` 要的实体面（`IStageEntity`）：把 `Entity` 直接抬上来。
class StageEntityView : public stage::IStageEntity {
 public:
  explicit StageEntityView(Entity& e) : _e(e) {}

  Value ref() const override { return ref_of(_e); }
  const Value& data() const override { return _e.data(); }
  Value team() const override { return Value(_e.team()); }
  Value ctrl() const override { return ctrl_value(_e.ctrl()); }
  Value facing() const override { return Value(_e.facing); }
  void set_facing(const Value& v) override { _e.facing = to_number(v); }
  double hp() const override { return _e.hp(); }
  void set_hp(double v) override { _e.set_hp(v); }
  double hp_max() const override { return _e.hp_max(); }
  double hp_r() const override { return _e.hp_r(); }
  void set_hp_r(double v) override { _e.set_hp_r(v); }
  double mp() const override { return _e.mp(); }
  void set_mp(double v) override { _e.set_mp(v); }
  double mp_max() const override { return _e.mp_max(); }
  bool mounted() const override { return truthy(Value(_e.mounted())); }
  double position_x() const override { return _e.position.x; }
  void set_position(const Value& x, const Value& y, const Value& z) override {
    _e.set_position(x, y, z);
  }
  IStageEntity* bearer() const override { return _bearer_view; }

  void set_bearer_view(StageEntityView* v) { _bearer_view = v; }

 private:
  Entity& _e;
  StageEntityView* _bearer_view = nullptr;
};

// `stage::IStageWorld`（`Stage` 那刀开的缝）：转发到真 `World`。
class WorldStageView : public stage::IStageWorld {
 public:
  explicit WorldStageView(World& world) : _world(world) {}

  World* world_ptr() const override { return &_world; }
  Background* bg() const override { return _world.bg(); }
  void set_bg(std::unique_ptr<Background> bg) override { _world.set_bg(std::move(bg)); }
  Stage* stage() const override { return _world.stage(); }

  // `world.entities` / `world.puppets`：TS 里是两个真数组，端口把 `Entity*` 投影成
  // `IStageEntity*`。**每次读到同一份投影**（同一个 `vector` 对象、同样的顺序），
  // 与 TS 的「同一个数组」等价。
  std::vector<stage::IStageEntity*>& entities() override {
    entity_view_.clear();
    for (Entity* const e : _world.entities) entity_view_.push_back(view_of(e));
    return entity_view_;
  }
  std::vector<stage::IStageEntity*>& puppets() override {
    puppet_view_.clear();
    for (const std::pair<std::u16string, Entity*>& kv : _world.puppets) {
      puppet_view_.push_back(view_of(kv.second));
    }
    return puppet_view_;
  }
  void del_entities(const std::vector<stage::IStageEntity*>& es) override {
    std::vector<Entity*> list;
    list.reserve(es.size());
    for (stage::IStageEntity* const e : es) {
      Entity* const real = entity_of(e);
      if (real != nullptr) list.push_back(real);
    }
    _world.del_entities(list);
  }
  // `world.dataset.difficulty`
  Value difficulty() const override { return _world.dataset.get(u"difficulty"); }
  // `world.camera.jump_x(x)`
  void camera_jump_x(const Value& x) override { _world.camera().jump_x(to_number(x)); }

 private:
  StageEntityView* view_of(Entity* const e) {
    const std::map<Entity*, std::unique_ptr<StageEntityView>>::iterator it = views_.find(e);
    if (it != views_.end()) return it->second.get();
    std::unique_ptr<StageEntityView> view = std::make_unique<StageEntityView>(*e);
    StageEntityView* const raw = view.get();
    views_.emplace(e, std::move(view));
    return raw;
  }
  Entity* entity_of(stage::IStageEntity* const view) const {
    for (const std::pair<Entity* const, std::unique_ptr<StageEntityView>>& kv : views_) {
      if (kv.second.get() == view) {
        StageEntityView* const v = static_cast<StageEntityView*>(view);
        (void)v;
        return kv.first;
      }
    }
    return nullptr;
  }

  World& _world;
  std::map<Entity*, std::unique_ptr<StageEntityView>> views_;
  std::vector<stage::IStageEntity*> entity_view_;
  std::vector<stage::IStageEntity*> puppet_view_;
};

// `entity::IEntityHost`：TS 里 `Entity` 直接读 `this.world.*` / `this.world.lfw.*`。
class WorldEntityHost : public IEntityHost {
 public:
  explicit WorldEntityHost(World& world) : _world(world) {}

  // `world.dataset[key]`
  Value world_dataset(const std::u16string& key) const override {
    return _world.dataset.get(key);
  }
  // `world.bg.data.dataset?.[key]`
  Value bg_dataset(const std::u16string& key) const override {
    const Background* const bg = _world.bg();
    if (bg == nullptr) return Value();
    return field_or(field_or(bg->data(), u"dataset"), key.c_str());
  }
  // `world.lfw.new_team` / `world.lfw.new_id`
  std::u16string new_team() const override { return _world.lfw().new_team(); }
  std::u16string new_id() override { return _world.lfw().new_id(); }
  // `world.lfw.factory.acquire_ctrl(InvalidController, "", this)`
  controller::BaseController* acquire_ctrl() override {
    return _world.lfw().acquire_invalid_ctrl(_world);
  }
  // `world.lfw.factory.release_ctrl(ctrl)`
  void release_ctrl(controller::BaseController* ctrl) override {
    _world.lfw().release_ctrl(ctrl);
  }
  // `world.mark_players_alive(this, alive)`
  void mark_players_alive(Entity& e, bool alive) override {
    _world.mark_players_alive(e, alive);
  }
  // `world.lfw.datas.find(id)`
  Value find_data(const std::u16string& id) const override {
    return _world.lfw().datas_find(Value(std::u16string(id)));
  }
  // `world.list_entities(key, predicate)`
  std::vector<Entity*> list_entities(const std::u16string& key,
                                     const std::function<bool(Entity&)>& predicate) override {
    const std::vector<Entity*>& cached = _world.list_entities(key, predicate);
    return std::vector<Entity*>(cached.begin(), cached.end());
  }
  // `world.entity_map.get(id)`
  Entity* find_entity(const std::u16string& id) const override {
    return _world.find_entity(id);
  }
  // `world.puppets.values()`
  std::vector<Entity*> puppets() override {
    std::vector<Entity*> out;
    out.reserve(_world.puppets.size());
    for (const std::pair<std::u16string, Entity*>& kv : _world.puppets) out.push_back(kv.second);
    return out;
  }
  // `world.stage.*`（`player_l` / `player_r` / `near` / `far` 是重生点的随机范围；
  // `id` / `team` / `is_stage_finish` / `is_chapter_finish` 是 `bot/*` 读的）
  Value stage_value(const std::u16string& key) const override {
    const Stage* const s = _world.stage();
    if (s == nullptr) return Value();
    if (key == u"player_l") return Value(s->player_l);
    if (key == u"player_r") return Value(s->player_r);
    if (key == u"far") return Value(s->far_plane());
    if (key == u"near") return Value(s->near_plane());
    if (key == u"id") return Value(s->id());
    if (key == u"team") return s->team();
    if (key == u"is_stage_finish") return Value(s->is_stage_finish());
    if (key == u"is_chapter_finish") return Value(s->is_chapter_finish());
    return Value();
  }
  // `world.get_bound(this)`
  std::vector<double> get_bound(Entity& e) override { return _world.get_bound(e); }
  // `world.has_players_alive`
  bool has_players_alive() const override { return _world.has_players_alive; }
  // `world.bg.width` / `.near` / `.far`
  Value bg_value(const std::u16string& key) const override {
    const Background* const bg = _world.bg();
    if (bg == nullptr) return Value();
    if (key == u"width") return Value(bg->width());
    if (key == u"near") return Value(bg->near_plane());
    if (key == u"far") return Value(bg->far_plane());
    return Value();
  }
  // `lfw.datas.find_bot(bot_id)`（`DatMgr` 未移植 ⇒ 默认找不到）
  Value find_bot(const std::u16string& bot_id) const override {
    return _world.lfw().find_bot(bot_id);
  }
  // `world.ground.step`（TS 的 `Ground.step` 是 `readonly step = 10`）
  double ground_step() const override { return _world.ground.step(); }
  // `lfw.survival_rank_mode` / `lfw.survival_rank_available`
  bool survival_rank_mode() const override { return _world.lfw().survival_rank_mode(); }
  bool survival_rank_available() const override {
    return _world.lfw().survival_rank_available();
  }
  // `lfw.is_cheat(name)`
  bool is_cheat(const std::u16string& name) const override {
    return _world.lfw().is_cheat(name);
  }
  // `world.entities.length + world.ghosts.length`
  double entity_count() const override {
    return static_cast<double>(_world.entities.size() + _world.ghosts.size());
  }
  // `world.add_entities(this)`
  void add_entities(Entity& e) override { _world.add_entities(e); }
  // `world.del_entity(this)`（`Entity::release`）
  void del_entity(Entity& e) override { _world.del_entity(e); }
  // `world.game_time`
  double game_time() const override { return _world.game_time(); }
  // `world.lfw.factory.create_entity_with_bot("", this.world, data)`
  Entity* create_entity_with_bot(const Value& data) override {
    return _world.lfw().create_entity(_world, data);
  }
  // `world.restrict(this)`
  Vector3 world_restrict(Entity& e) override { return _world.restrict(e); }
  // `lfw.mt`
  MersenneTwister& mt() override { return *_world.lfw().mt(); }
  // `lfw.broadcast(m)`
  void broadcast(const Value& m) override { _world.lfw().broadcast(m); }
  // `lfw.factory.create_ctrl(data.id, this.ctrl.player_id, this)`
  controller::BaseController* create_ctrl(const std::u16string& data_id,
                                          const std::u16string& player_id) override {
    return _world.lfw().create_ctrl(data_id, player_id);
  }

 private:
  World& _world;
};

}

World::World(IWorldLfw& lfw, IWorldRenderer& renderer, state::States* states)
    : lfw_(&lfw), renderer_(&renderer), states_(states) {
  host_ = std::make_unique<WorldEntityHost>(*this);
  stage_view_ = std::make_unique<WorldStageView>(*this);
  // `this._bg = new Background(this, Defines.VOID_BG)`
  bg_ = std::make_unique<Background>(this, defines_value(u"Defines.VOID_BG"));
  // `this.transform.set_scale(bg.zoom_x, bg.zoom_y, bg.zoom_z)`
  transform.set_scale(bg_->zoom_x(), bg_->zoom_y(), bg_->zoom_z());
  // `this._stage = new Stage(this, Defines.VOID_STAGE)`
  stage_ = std::make_unique<Stage>(stage_view_.get(), lfw_, defines_value(u"Defines.VOID_STAGE"));
  // `this.renderer = new Ditto.WorldRender(this)`：宿主注入（渲染未移植）。
  // `this.camera = new Camera(this)`
  camera_ = std::make_unique<Camera>(this);
  // `this.dataset.on_dataset_change = this.on_dataset_change.bind(this)`
  dataset.on_dataset_change = [this](const std::u16string& key, const Value& curr,
                                     const Value& prev) { on_dataset_change(key, curr, prev); };
}

World::~World() = default;

Value World::world_stage() {
  // TS 的这一处是属性读 `this.world.stage`；端口把 `Stage` 的属性快照成对象（每次调用一份，
  // 与「读几次就是几次」对齐）。
  auto o = std::make_shared<Object>();
  const Stage* const s = stage_.get();
  if (s == nullptr) return Value(o);
  o->set(u"id", Value(s->id()));
  o->set(u"cam_l", Value(s->cam_l));
  o->set(u"cam_r", Value(s->cam_r));
  o->set(u"player_l", Value(s->player_l));
  o->set(u"player_r", Value(s->player_r));
  o->set(u"enemy_l", Value(s->enemy_l));
  o->set(u"enemy_r", Value(s->enemy_r));
  o->set(u"left", Value(s->left));
  o->set(u"right", Value(s->right));
  o->set(u"near", Value(s->near_plane()));
  o->set(u"far", Value(s->far_plane()));
  o->set(u"width", Value(s->width));
  o->set(u"depth", Value(s->depth));
  o->set(u"drink_l", Value(s->drink_l));
  o->set(u"drink_r", Value(s->drink_r));
  o->set(u"world_pause", Value(s->world_pause()));
  o->set(u"is_stage_finish", Value(s->is_stage_finish()));
  o->set(u"is_chapter_finish", Value(s->is_chapter_finish()));
  return Value(o);
}

Value World::world_bg() {
  auto o = std::make_shared<Object>();
  const Background* const b = bg_.get();
  if (b == nullptr) return Value(o);
  o->set(u"id", b->id());
  o->set(u"name", b->name());
  o->set(u"left", Value(b->left()));
  o->set(u"right", Value(b->right()));
  o->set(u"near", Value(b->near_plane()));
  o->set(u"far", Value(b->far_plane()));
  o->set(u"width", Value(b->width()));
  o->set(u"height", Value(b->height()));
  o->set(u"depth", Value(b->depth()));
  o->set(u"zoom_x", Value(b->zoom_x()));
  o->set(u"zoom_y", Value(b->zoom_y()));
  o->set(u"zoom_z", Value(b->zoom_z()));
  return Value(o);
}

Value World::dataset_value() const { return dataset.dump_dataset(); }

Value World::world_dataset() { return dataset_value(); }

WorldCollisionHost& World::collision_host() {
  if (collision_host_ == nullptr) collision_host_ = std::make_unique<WorldCollisionHost>(*this);
  return *collision_host_;
}

void World::set_bg(std::unique_ptr<Background> v) {
  if (v.get() == bg_.get()) return;
  std::unique_ptr<Background> const o = std::move(bg_);
  bg_ = std::move(v);
  transform.set_scale(bg_->zoom_x(), bg_->zoom_y(), bg_->zoom_z());
  o->dispose();
}

void World::set_stage(std::unique_ptr<Stage> v) {
  if (v.get() == stage_.get()) return;
  std::unique_ptr<Stage> const o = std::move(stage_);
  stage_ = std::move(v);
  callbacks.call(u"on_stage_change",
                 {WorldCallbackArgs{this, nullptr, stage_.get(), o.get()}});
  o->dispose();
  stage_->enter_phase(0);
  for (Entity* const e : entities) {
    controller::BaseController* const ctrl = e->ctrl();
    if (!is_bot_ctrl_ptr(ctrl)) continue;
    if (!lfw_->ctrl_goingto(*ctrl)) continue;
    lfw_->ctrl_come(*ctrl, e->position.x, e->position.y, e->position.z);
  }
}

double World::player_l() const { return stage_ != nullptr ? stage_->player_l : 0.0; }
double World::player_r() const { return stage_ != nullptr ? stage_->player_r : 0.0; }
double World::left() const { return stage_ != nullptr ? stage_->left : 0.0; }
double World::right() const { return stage_ != nullptr ? stage_->right : 0.0; }
double World::near_plane() const { return stage_ != nullptr ? stage_->near_plane() : 0.0; }
double World::far_plane() const { return stage_ != nullptr ? stage_->far_plane() : 0.0; }
double World::width() const { return stage_ != nullptr ? stage_->width : 0.0; }
double World::depth() const { return stage_ != nullptr ? stage_->depth : 0.0; }
const Background::Middle& World::middle() const {
  static const Background::Middle kEmpty{};
  if (stage_ == nullptr) return kEmpty;
  return stage_->middle;
}

bool World::stage_limit() const {
  return stage_->id() != std::u16string(u"VOID_STAGE") &&
         !lfw_->is_cheat(std::u16string(cheat_enum::kHERO_FT));
}

bool World::world_pause() const { return stage_ != nullptr && stage_->world_pause(); }
bool World::is_stage_finish() const { return stage_ != nullptr && stage_->is_stage_finish(); }
bool World::is_chapter_finish() const {
  return stage_ != nullptr && stage_->is_chapter_finish();
}

void World::on_dataset_change(const std::u16string& k, const Value& curr, const Value& prev) {
  callbacks.call(u"on_dataset_change",
                 {WorldCallbackArgs{this, nullptr, nullptr, nullptr, curr, prev, k}});
}

void World::set_paused(bool v) { set_paused_value(v ? 1.0 : 0.0); }

void World::set_paused_value(double v) {
  if (paused_ == v) return;
  const bool changed = (!truthy(Value(v))) != (!truthy(Value(paused_)));
  paused_ = v;
  if (changed) callbacks.call(u"on_pause_change", {WorldCallbackArgs{this, nullptr, nullptr,
                                                                    nullptr, Value(), Value(),
                                                                    std::u16string(),
                                                                   0.0, 0.0, 0.0,
                                                                    truthy(Value(v))}});
}

void World::set_fn_locked(bool v) { set_fn_locked_value(v ? 1.0 : 0.0); }

void World::set_fn_locked_value(double v) {
  if (fn_locked_ == v) return;
  fn_locked_ = v;
  callbacks.call(u"on_fn_locked_change",
                 {WorldCallbackArgs{this, nullptr, nullptr, nullptr, Value(), Value(),
                                    std::u16string(), v}});
}

void World::team_come(const std::u16string& team, double x, double y, double z) {
  for (Entity* const e : entities) {
    controller::BaseController* const ctrl = e->ctrl();
    if (team == e->team() && is_bot_ctrl_ptr(ctrl)) lfw_->ctrl_come(*ctrl, x, y, z);
  }
}

void World::team_move(const std::u16string& team) {
  for (Entity* const e : entities) {
    controller::BaseController* const ctrl = e->ctrl();
    if (team == e->team() && is_bot_ctrl_ptr(ctrl)) lfw_->ctrl_move(*ctrl);
  }
}

void World::team_stay(const std::u16string& team) {
  for (Entity* const e : entities) {
    controller::BaseController* const ctrl = e->ctrl();
    if (team == e->team() && is_bot_ctrl_ptr(ctrl)) lfw_->ctrl_stay(*ctrl);
  }
}

void World::team_follow(Entity& target) {
  for (Entity* const e : entities) {
    controller::BaseController* const ctrl = e->ctrl();
    if (target.team() == e->team() && is_bot_ctrl_ptr(ctrl)) lfw_->ctrl_follow(*ctrl, target);
  }
}

Entity* World::map_get(const std::vector<std::pair<std::u16string, Entity*>>& m,
                       const std::u16string& key) {
  for (const std::pair<std::u16string, Entity*>& kv : m) {
    if (kv.first == key) return kv.second;
  }
  return nullptr;
}

void World::map_set(std::vector<std::pair<std::u16string, Entity*>>& m, std::u16string key,
                    Entity* value) {
  for (std::pair<std::u16string, Entity*>& kv : m) {
    if (kv.first == key) {
      kv.second = value;
      return;
    }
  }
  m.emplace_back(std::move(key), value);
}

bool World::vec_has(const std::vector<std::u16string>& v, const std::u16string& key) {
  for (const std::u16string& s : v) {
    if (s == key) return true;
  }
  return false;
}

void World::add_entities(Entity& e) {
  if (map_get(entity_map, e.id) != nullptr) return;

  if (entity::is_fighter(ref_of(e))) {
    callbacks.call(u"on_fighter_add",
                   {WorldCallbackArgs{this, &e, nullptr, nullptr, Value(), Value(),
                                      std::u16string(), 0.0, 0.0, 0.0, false}});
    controller::BaseController* const ctrl = e.ctrl();
    const std::u16string player_id = ctrl != nullptr ? ctrl->player_id : std::u16string();
    PlayerInfo* const player = lfw_->player(Value(std::u16string(player_id)));
    if (player != nullptr) {
      player->set_fighter(&e);
      map_set(puppets, player_id, &e);
      e.puppet = true;
      callbacks.call(u"on_puppet_add",
                     {WorldCallbackArgs{this, nullptr, nullptr, nullptr, Value(), Value(),
                                        std::u16string(player_id)}});
    }
  }
  if (truthy(Value(e.ghosted()))) {
    ghosts.push_back(&e);
  } else {
    entities.push_back(&e);
  }
  map_set(entity_map, e.id, &e);
  renderer_->add_entity(e);
  controller::BaseController* const ctrl = e.ctrl();
  mark_players_alive(e, is_human_ctrl_ptr(ctrl) && e.hp() > 0);
}

const std::vector<Entity*>& World::list_entities(
    const std::u16string& name, const std::function<bool(Entity&)>& predicate) {
  const std::map<std::u16string, std::vector<Entity*>>::iterator it = entities_map_.find(name);
  if (it != entities_map_.end()) return it->second;
  std::vector<Entity*>& ret = entities_map_[name];
  for (Entity* const o : entities) {
    if (predicate(*o)) ret.push_back(o);
  }
  return ret;
}

void World::del_entity(Entity& e) { e.set_frame(gone_frame_info()); }

void World::del_entities(const std::vector<Entity*>& list) {
  for (Entity* const e : list) del_entity(*e);
}

void World::mark_players_alive(Entity& e, bool is_alive) {
  if (is_alive) {
    if (std::find(alive_players_.begin(), alive_players_.end(), &e) == alive_players_.end()) {
      alive_players_.push_back(&e);
    }
  } else {
    alive_players_.erase(std::remove(alive_players_.begin(), alive_players_.end(), &e),
                         alive_players_.end());
  }
  has_players_alive = !alive_players_.empty();
}

void World::collect_dead_buff(buff::Buff& b) {
  b.update(to_number(dataset.get(u"atom_time")));
  if (b.dead()) dead_buffs_.push_back(&b);
}

void World::add_collision(const collision::Collision& c) {
  collision::Collision* prev = nullptr;
  for (std::pair<std::u16string, collision::Collision>& kv : collisions) {
    if (kv.first == c.id) {
      prev = &kv.second;
      break;
    }
  }
  if (prev != nullptr && prev->m_distance <= c.m_distance) return;
  if (prev != nullptr) pair_collisions_.remove(prev->aid, prev->vid);
  bool replaced = false;
  for (std::pair<std::u16string, collision::Collision>& kv : collisions) {
    if (kv.first == c.id) {
      kv.second = c;
      replaced = true;
      break;
    }
  }
  if (!replaced) collisions.emplace_back(c.id, c);
  pair_collisions_.add(c.aid, c.vid, c);
}

void World::render_once(double dt) {
  const double t0 = clock_now();
  renderer_->render(dt);
  const double spent = clock_now() - t0;
  render_cost = truthy(Value(render_cost)) ? render_cost * 0.9 + spent * 0.1 : spent;
}

void World::update_camera() {
  const double old_cam_x = round(camera_->position.x);
  const double old_cam_y = round(camera_->position.y);
  camera_->update();
  const double new_cam_x = round(camera_->position.x);
  const double new_cam_y = round(camera_->position.y);
  if (old_cam_x != new_cam_x || old_cam_y != new_cam_y) {
    callbacks.call(u"on_cam_move", {WorldCallbackArgs{this, nullptr, nullptr, nullptr, Value(),
                                                     Value(), std::u16string(), new_cam_x,
                                                     new_cam_y}});
  }
}

void World::spark(double x, double y, double z, const std::u16string& f) {
  if (static_cast<double>(entities.size() + ghosts.size()) > kMaxDebugEntities) return;
  const std::u16string oid(built_in_dats::kSpark);
  if (!truthy(spark_data_)) spark_data_ = lfw_->datas_find(Value(std::u16string(oid)));
  const Value data = spark_data_;
  if (!truthy(data)) {
    lfw_->warn(std::u16string(u"[World::spark] \"") + oid + u"\" data not found!");
    return;
  }
  Entity* const e = lfw_->create_entity(*this, data);
  if (e == nullptr) {
    lfw_->warn(std::u16string(u"[World::spark] failed"));
    return;
  }
  e->set_outline_alpha(0);
  e->set_outline_width(0);
  e->set_outline_color(std::u16string());
  e->set_position(Value(x), Value(y), Value(z));
  e->enter_frame_by_id(Value(std::u16string(f)));
  e->attach(Value(true));
}

void World::etc(double x, double y, double z, const std::u16string& f) {
  const std::u16string oid(oid::kEtc);
  if (!truthy(etc_data_)) etc_data_ = lfw_->datas_find(Value(std::u16string(oid)));
  const Value data = etc_data_;
  if (!truthy(data)) {
    lfw_->warn(std::u16string(u"[World::etc] oid \"") + oid + u"\" data not found!");
    return;
  }
  Entity* const e = lfw_->create_entity(*this, data);
  if (e == nullptr) {
    lfw_->warn(std::u16string(u"[World::etc] failed"));
    return;
  }
  e->position.x = round(x);
  e->position.y = round(y);
  e->position.z = round(z);
  e->enter_frame_by_id(Value(std::u16string(f)));
  e->attach(Value(false));
}

Value World::get_bounding(Entity& e, const Value& f, const Value& i) {
  const Value ql = defines_value(u"Defines.DAFUALT_QUBE_LENGTH");
  const double l_default = to_number(ql);
  const Value xv = field_or(i, u"x");
  const Value yv = field_or(i, u"y");
  const Value wv = field_or(i, u"w");
  const Value hv = field_or(i, u"h");
  const Value lv = field_or(i, u"l");
  const Value zv = field_or(i, u"z");
  const double x = std::holds_alternative<std::monostate>(xv) ? 0.0 : to_number(xv);
  const double y = std::holds_alternative<std::monostate>(yv) ? 0.0 : to_number(yv);
  const double w = std::holds_alternative<std::monostate>(wv) ? 0.0 : to_number(wv);
  const double h = std::holds_alternative<std::monostate>(hv) ? 0.0 : to_number(hv);
  const double l = std::holds_alternative<std::monostate>(lv) ? l_default : to_number(lv);
  const double z = std::holds_alternative<std::monostate>(zv) ? -l_default / 2.0 : to_number(zv);
  const double centerx = to_number(field_or(f, u"centerx"));
  const double centery = to_number(field_or(f, u"centery"));
  const double left = e.facing > 0 ? e.position.x - centerx + x
                                   : e.position.x + centerx - x - w;
  const double top = e.position.y + centery - y;
  const double far = e.position.z + z;
  auto o = std::make_shared<Object>();
  o->set(u"left", Value(round(left)));
  o->set(u"right", Value(round(left + w)));
  o->set(u"top", Value(round(top)));
  o->set(u"bottom", Value(round(top - h)));
  o->set(u"far", Value(round(far)));
  o->set(u"near", Value(round(far + l)));
  return Value(o);
}

void World::stop_render() {
  if (render_worker_id_ != 0) render_del(render_worker_id_);
  render_worker_id_ = 0;
}

double World::fps_value() const {
  const double sync_render = to_number(dataset.get(u"sync_render"));
  switch (static_cast<SyncRenderEnum>(static_cast<int>(sync_render))) {
    case SyncRenderEnum::Unlimited:
      return 1000;
    case SyncRenderEnum::FPS_60:
      return 60;
    case SyncRenderEnum::FPS_120:
      return 120;
    case SyncRenderEnum::Sync:
      return to_number(dataset.get(u"UPS"));
    case SyncRenderEnum::Half:
      return floor(to_number(dataset.get(u"UPS")) / 2);
    default:
      break;
  }
  // TS 这里落空 ⇒ `undefined` ⇒ 后续算术是 NaN。
  return std::nan("");
}

void World::start_render() {
  const Value sync_render_v = dataset.get(u"sync_render");
  const double sync_render = to_number(sync_render_v);
  const double fps = fps_value();
  if (render_worker_id_ != 0) render_del(render_worker_id_);
  if (sync_render == static_cast<double>(SyncRenderEnum::Sync)) return;
  if (sync_render == static_cast<double>(SyncRenderEnum::Half)) return;
  render_prev_time_ = 0.0;
  render_fix_radio_ = 1.0;
  const double ideally_dt = 1000 / fps;
  const std::function<void()> on_render = [this, ideally_dt, fps]() {
    const double time = clock_now();
    if (render_prev_time_ == 0) {
      render_prev_time_ = time;
      return;
    }
    const double real_dt = time - render_prev_time_;
    if (real_dt < render_fix_radio_ * ideally_dt) return;
    render_once(real_dt);
    fps_.update(real_dt);
    if (need_FPS_) {
      callbacks.call(u"on_fps_update", {WorldCallbackArgs{this, nullptr, nullptr, nullptr, Value(),
                                                         Value(), std::u16string(),
                                                         fps_.value()}});
    }
    render_fix_radio_ = 1 - clamp(6 * (fps - fps_.value()) / fps, 0, 1);
    render_prev_time_ = time;
  };
  if (render_worker_id_ != 0) render_del(render_worker_id_);
  render_worker_id_ = render_add(on_render);
}

void World::stop_update() {
  if (update_worker_ != nullptr) update_worker_->stop();
  update_worker_.reset();
}

void World::sleep() {
  sleeping_ = true;
  if (update_worker_ != nullptr) update_worker_->pause();
}

void World::awake() {
  sleeping_ = false;
  if (update_worker_ != nullptr) update_worker_->resume();
}

double World::base_step_ms() {
  Value playrate = dataset.get(u"playrate");
  Value ups = dataset.get(u"UPS");
  Value atom_time = dataset.get(u"atom_time");
  double playrate_n = to_number(playrate);
  if (!between(playrate_n, 0.01, 1000)) {
    lfw_->warn(std::u16string(u"[World::start_update] playrate must be between 0.01 and 1000, "
                             u"but got ") +
               to_string(playrate) + u", now reset to 1.0");
    dataset.set(u"playrate", Value(1.0));
    playrate = Value(1.0);
    playrate_n = 1.0;
  }
  double ups_n = to_number(ups);
  if (ups_n != 30 && ups_n != 60) {
    lfw_->warn(std::u16string(u"[World::start_update] UPS must be 30 or 60, but got ") +
               to_string(ups) + u", now reset to 60");
    dataset.set(u"UPS", Value(60.0));
    ups = Value(60.0);
    ups_n = 60.0;
    dataset.set(u"atom_time", Value(1.0));
    atom_time = Value(1.0);
  }
  double atom_n = to_number(atom_time);
  if (!(atom_n > 0)) {
    lfw_->warn(std::u16string(u"[World::start_update] atom_time must be > 0, but got ") +
               to_string(atom_time) + u", now reset to 1");
    dataset.set(u"atom_time", Value(1.0));
    atom_time = Value(1.0);
    atom_n = 1.0;
  }
  return 1000 / ups_n / playrate_n;
}

void World::on_step_error(const std::u16string& message, bool has_errors) {
  step_error_count_ += 1;
  const double now = clock_now();
  if (now - step_error_time_ > 1000) {
    step_error_time_ = now;
    lfw_->warn(message);
    if (has_errors) lfw_->warn(u"[World::start_update] errors");
  }
  if (step_error_count_ >= kMaxStepErrors) {
    lfw_->warn(std::u16string(u"[World::start_update] ") + to_string(Value(step_error_count_)) +
               u" times update error in a row, stop update loop");
    stop_update();
  }
}

std::vector<double> World::get_bound(Entity& e) {
  const Stage& s = *stage_;
  double l = -9007199254740991.0;  // `Number.MIN_SAFE_INTEGER`
  double r = 9007199254740991.0;   // `Number.MAX_SAFE_INTEGER`
  if (entity::is_fighter(ref_of(e))) {
    if (e.team() == to_string(s.team())) {
      l = s.enemy_l;
      r = s.enemy_r;
    } else {
      l = s.player_l;
      r = s.player_r;
    }
  } else if (entity::is_weapon(ref_of(e)) && e.base_type() == static_cast<double>(WeaponEnum::Drink)) {
    l = s.drink_l;
    r = s.drink_r;
  }
  bound_[0] = l;
  bound_[1] = r;
  bound_[2] = s.near_plane();
  bound_[3] = s.far_plane();
  return bound_;
}

const Vector3& World::restrict(Entity& e) {
  double x = e.position.x;
  double z = e.position.z;
  double y = e.position.y;
  if (e.bearer != nullptr || e.catcher != nullptr || truthy(Value(e.ghosted()))) {
    e.terrain = ground.base();
    restrict_result_.x = x;
    restrict_result_.y = y;
    restrict_result_.z = z;
    return restrict_result_;
  }
  const std::vector<double> b = get_bound(e);
  const double left = b[0];
  const double right = b[1];
  const double near_v = b[2];
  const double far_v = b[3];
  x = clamp(x, left, right);
  z = clamp(z, far_v, near_v);
  if (entity::is_weapon(ref_of(e))) {
    if (e.is_on_ground && x < left - e.l_len || x > right + e.r_len) {
      e.enter_frame(defines_value(u"Defines.NEXT_FRAME_GONE"));
      e.terrain = ground.base();
      restrict_result_.x = x;
      restrict_result_.y = y;
      restrict_result_.z = z;
      return restrict_result_;
    }
  } else if (entity::is_ball(ref_of(e))) {
    if (x < left - 800 - e.l_len || x > right + 800 + e.r_len) {
      e.enter_frame(defines_value(u"Defines.NEXT_FRAME_GONE"));
      e.terrain = ground.base();
      restrict_result_.x = x;
      restrict_result_.y = y;
      restrict_result_.z = z;
      return restrict_result_;
    }
  }
  const ITerrainInfo seg = ground.segment(x, z);
  if (ground.enterable(seg, x, y, z).has_value()) {
    e.terrain = seg;
    restrict_result_.x = x;
    restrict_result_.y = y;
    restrict_result_.z = z;
    return restrict_result_;
  }
  const std::vector<BlockPoint>& pushs =
      ground.block(seg, x, y, z, e.prev_position.x, e.prev_position.y, e.prev_position.y);
  for (const BlockPoint& push : pushs) {
    if (push.z < far_v || push.z > near_v || push.x < left || push.x > right) continue;
    const ITerrainInfo seg2 = ground.segment(push.x, push.z);
    if (!ground.enterable(seg2, push.x, y, push.z).has_value()) continue;
    e.terrain = seg2;
    restrict_result_.x = push.x;
    restrict_result_.y = y;
    restrict_result_.z = push.z;
    return restrict_result_;
  }
  e.terrain = seg;
  restrict_result_.x = x;
  restrict_result_.y = Ground::y(seg, x, z);
  restrict_result_.z = z;
  return restrict_result_;
}

void World::update_ui() {
  const double uidt = round_float(16.66666 * to_number(dataset.get(u"atom_time")));
  const std::vector<IWorldUi*> uis = lfw_->layer_uis();
  for (size_t i = uis.size(); i > 0; --i) {
    IWorldUi* const ui = uis[i - 1];
    if (ui == nullptr || ui->disabled()) continue;
    ui->update(uidt);
  }
}

void World::step() {
  entities_map_.clear();
  transform.update();
  update_ui();
  handle_cmds();
  update_camera();
  if (bg_ != nullptr) bg_->update();

  if (paused_ == 1.0) return;
  if (paused_ == 2.0) paused_ = 1.0;
  game_time_.add();
  team_alive_counts_.clear();
  puppet_teams.clear();

  if (stage_->world_pause()) return;
  if (lfw_->dev() && static_cast<double>(entities.size()) > kMaxDebugEntities) {
    lfw_->debug(u"[World::update_once]entities.size = " +
                number_to_string(static_cast<double>(entities.size())));
  }
  collisions.clear();
  collision_host().reset_collisions();
  pair_collisions_.clear();
  pairs_compared = 0.0;
  dead_buffs_.clear();
  for (std::pair<std::u16string, buff::Buff*>& kv : buffs) {
    if (kv.second != nullptr) collect_dead_buff(*kv.second);
  }
  for (buff::Buff* const b : dead_buffs_) {
    b->unmount();
    for (size_t i = 0; i < buffs.size(); ++i) {
      if (buffs[i].first == b->id()) {
        buffs.erase(buffs.begin() + static_cast<ptrdiff_t>(i));
        break;
      }
    }
    lfw_->recycle_buff(b);
  }

  double offset = 0;
  double puppet_x_sum = 0;
  double puppet_z_sum = 0;
  double puppet_count = 0;
  double local_x_sum = 0;
  double local_z_sum = 0;
  double human_x_sum = 0;
  double human_z_sum = 0;
  double fighter_x_sum = 0;
  double fighter_z_sum = 0;
  double local_count = 0;
  double human_count = 0;
  double fighter_count = 0;
  // 可见宽度 = screen / zoom（bg 的 zoom 会缩放世界）；镜头记录的是视口左边缘。
  const double zoom_x = bg_ != nullptr ? bg_->zoom_x() : 0.0;
  const double zoom_y = bg_ != nullptr ? bg_->zoom_y() : 0.0;
  const double view_w = to_number(dataset.get(u"screen_w")) / (truthy(Value(zoom_x)) ? zoom_x : 1);
  // 前瞻量：固定世界距离（不随 zoom 缩放）。
  const double lead = to_number(dataset.get(u"screen_w")) / 6;
  for (size_t i = 0; i < entities.size(); ++i) {
    Entity* const a = entities[i];
    if (offset != 0) {
      entities[i - static_cast<size_t>(offset)] = a;
    }
    if (entity_is_gone(*a)) {
      a->set_hp(0.0);
      a->set_hp_r(0.0);
      gones_add(gones_, a);
      offset += 1;
      continue;
    }
    if (gones_has(gones_, a)) {
      offset += 1;
      continue;
    }
    a->update();

    if (entity::is_fighter(ref_of(*a))) {
      if (a->hp() > 0) {
        double count = 0;
        for (const std::pair<std::u16string, double>& kv : team_alive_counts_) {
          if (kv.first == a->team()) {
            count = kv.second;
            break;
          }
        }
        bool replaced = false;
        for (std::pair<std::u16string, double>& kv : team_alive_counts_) {
          if (kv.first == a->team()) {
            kv.second = count + 1;
            replaced = true;
            break;
          }
        }
        if (!replaced) team_alive_counts_.emplace_back(a->team(), count + 1);
      }
      const double x = a->position.x - view_w / 2 + a->facing * lead;
      const double z = a->position.z;
      fighter_x_sum += x;
      fighter_z_sum += z;
      fighter_count += 1;
      controller::BaseController* const ctrl = a->ctrl();
      if (is_human_ctrl_ptr(ctrl) && a->hp() > 0) {
        if (truthy(field_or(ctrl->player, u"mine"))) {
          local_x_sum += x;
          local_z_sum += z;
          local_count += 1;
        } else {
          human_x_sum += x;
          human_z_sum += z;
          human_count += 1;
        }
      }
      if (a->puppet) {
        if (!vec_has(puppet_teams, a->team())) puppet_teams.push_back(a->team());
        puppet_x_sum += x;
        puppet_z_sum += z;
        puppet_count += 1;
      }
    }
  }
  entities.resize(entities.size() - static_cast<size_t>(offset));

  double goffset = 0;
  for (size_t i = 0; i < ghosts.size(); ++i) {
    Entity* const a = ghosts[i];
    if (goffset != 0) {
      ghosts[i - static_cast<size_t>(goffset)] = a;
    }
    if (entity_is_gone(*a)) {
      a->set_hp(0.0);
      a->set_hp_r(0.0);
      gones_add(gones_, a);
      goffset += 1;
      continue;
    }
    if (gones_has(gones_, a)) {
      goffset += 1;
      continue;
    }
    a->update_ghost();
  }
  ghosts.resize(ghosts.size() - static_cast<size_t>(goffset));

  const size_t len = entities.size();
  std::stable_sort(entities.begin(), entities.end(), x_sorter);
  ground_weapon_counts.clear();

  for (size_t i = 0; i < len; ++i) {
    Entity* const a = entities[i];
    if (frame_is_gone(*a)) continue;
    if (entity::is_weapon(ref_of(*a)) && a->is_on_ground) {
      const double section = round(a->position.x / kWeaponXSection);
      double count = 0;
      for (const std::pair<double, double>& kv : ground_weapon_counts) {
        if (kv.first == section) {
          count = kv.second;
          break;
        }
      }
      bool replaced = false;
      for (std::pair<double, double>& kv : ground_weapon_counts) {
        if (kv.first == section) {
          kv.second = count + 1;
          replaced = true;
          break;
        }
      }
      if (!replaced) ground_weapon_counts.emplace_back(section, count + 1);
    }
    controller::BaseController* const ctrl = a->ctrl();
    const double lifetime = a->lifetime();
    const bool lookingup = 0 == std::fmod(lifetime, kLookupUpdateInterval);
    if (lookingup && (is_ball_ctrl_ptr(ctrl) || is_bot_ctrl_ptr(ctrl))) {
      lfw_->ctrl_update_lookup(*ctrl, static_cast<double>(i), entities);
    }

    // 细致的碰撞判定
    const double a_max_x = a->aabb_max_x;
    const double a_min_z = a->aabb_min_z;
    const double a_max_z = a->aabb_max_z;
    for (size_t j = i + 1; j < len; ++j) {
      Entity* const b = entities[j];
      if (a_max_x < b->aabb_min_x) break;
      if (frame_is_gone(*b)) continue;
      if (a_max_z < b->aabb_min_z || b->aabb_max_z < a_min_z) continue;
      pairs_compared += 1.0;
      collision::Collision* const c1 = collision_host().collision_get(*a, *b);
      collision::Collision* const c2 = collision_host().collision_get(*b, *a);
      const Value inf = Value(std::numeric_limits<double>::infinity());
      // TS: `c1?.priority ?? Infinity` —— `priority` 是 `Value`（`ENTITY_PRIORITY_MAP` 未列出的
      // 类型给 `undefined`），而 `undefined <= 5` 为假、`Infinity <= 5` 也是假、
      // `5 <= Infinity` 为真 ⇒ `undefined` 那一侧必须补成 `Infinity` 才等价。
      const Value p1 = (c1 != nullptr && !std::holds_alternative<std::monostate>(c1->priority))
                           ? c1->priority
                           : inf;
      const Value p2 = (c2 != nullptr && !std::holds_alternative<std::monostate>(c2->priority))
                           ? c2->priority
                           : inf;
      if (c1 != nullptr && le(p1, p2)) add_collision(*c1);
      if (c2 != nullptr && le(p2, p1)) add_collision(*c2);
    }
  }

  // y 的偏移在写入时补（z 的采样不含半屏）；可见高度 = screen / zoom。
  const double half_h =
      to_number(dataset.get(u"screen_h")) / (2 * (truthy(Value(zoom_y)) ? zoom_y : 1));
  if (truthy(Value(local_count))) {
    camera_->destination.x = round(local_x_sum / local_count);
    camera_->destination.y = -0.5 * round(local_z_sum / local_count) - half_h;
  } else if (truthy(Value(human_count))) {
    camera_->destination.x = round(human_x_sum / human_count);
    camera_->destination.y = -0.5 * round(human_z_sum / human_count) - half_h;
  } else if (truthy(Value(puppet_count))) {
    camera_->destination.x = round(puppet_x_sum / puppet_count);
    camera_->destination.y = -0.5 * round(puppet_z_sum / puppet_count) - half_h;
  } else if (truthy(Value(fighter_count))) {
    camera_->destination.x = round(fighter_x_sum / fighter_count);
    camera_->destination.y = -0.5 * round(fighter_z_sum / fighter_count) - half_h;
  }

  // `this.collisions.forEach(c => collisions_keeper.handle(c))`：TS 的 `forEach` 会**看到**遍历
  // 期间新追加的项，所以条件每轮重读 `size()`；同时按下标取（而不是 `range-for` 的引用），
  // 因为 handler 完全可能再往 `collisions` 里塞一条（`push_back` 会让引用悬空）。
  for (size_t ci = 0; ci < collisions.size(); ++ci) {
    collision_host().handle(collisions[ci].second);
  }

  for (Entity* const entity : gones_) {
    for (size_t i = 0; i < entity_map.size(); ++i) {
      if (entity_map[i].first == entity->id) {
        entity_map.erase(entity_map.begin() + static_cast<ptrdiff_t>(i));
        break;
      }
    }
    mark_players_alive(*entity, false);
    if (entity::is_fighter(ref_of(*entity))) {
      callbacks.call(u"on_fighter_del",
                     {WorldCallbackArgs{this, entity, nullptr, nullptr, Value(), Value(),
                                        std::u16string(), 0.0, 0.0, 0.0, false}});
    }
    controller::BaseController* const ctrl = entity->ctrl();
    const std::u16string player_id = ctrl != nullptr ? ctrl->player_id : std::u16string();
    PlayerInfo* const player = lfw_->player(Value(std::u16string(player_id)));
    if (player != nullptr) player->set_fighter(nullptr);
    Entity* const puppet = map_get(puppets, player_id);
    if (puppet == entity) {
      for (size_t i = 0; i < puppets.size(); ++i) {
        if (puppets[i].first == player_id) {
          puppets.erase(puppets.begin() + static_cast<ptrdiff_t>(i));
          break;
        }
      }
    }
    entity->puppet = false;
    callbacks.call(u"on_puppet_del",
                   {WorldCallbackArgs{this, nullptr, nullptr, nullptr, Value(), Value(),
                                      std::u16string(player_id)}});
    renderer_->del_entity(*entity);
    entity->release();
    lfw_->recycle_entity(entity);
  }
  gones_.clear();
  stage_->update();
}

void World::update_once(double dt) {
  if (sleeping_) return;
  if (before_update) before_update();
  if (sleeping_) return;
  step();
  lifetime_ += 1.0;
  lfw_->clear_cmds();
  lfw_->clear_broadcasts();
  if (extra_steps > 0) catch_up();

  const double sync_render = to_number(dataset.get(u"sync_render"));
  if (sync_render == static_cast<double>(SyncRenderEnum::Sync)) {
    render_once(dt);
    fps_.update(dt);
    if (need_FPS_) {
      callbacks.call(u"on_fps_update", {WorldCallbackArgs{this, nullptr, nullptr, nullptr, Value(),
                                                         Value(), std::u16string(), fps_.value()}});
    }
  } else if (sync_render == static_cast<double>(SyncRenderEnum::Half) &&
             std::fmod(floor(lifetime_ / to_number(dataset.get(u"playrate"))), 2) != 0) {
    render_once(dt * 2);
    fps_.update(dt * 2);
    if (need_FPS_) {
      callbacks.call(u"on_fps_update", {WorldCallbackArgs{this, nullptr, nullptr, nullptr, Value(),
                                                         Value(), std::u16string(), fps_.value()}});
    }
  }

  Ticker* const worker = update_worker_.get();
  if (worker != nullptr) {
    TU = (1000 / to_number(dataset.get(u"UPS"))) * worker->span();
    if (need_UPS_) {
      callbacks.call(u"on_ups_update",
                     {WorldCallbackArgs{this, nullptr, nullptr, nullptr, Value(), Value(),
                                        std::u16string(), worker->rate(), 0.0,
                                        1 / worker->span()}});
    }
  }
  if (after_update) after_update();

  if (to_number(dataset.get(u"sync_render")) != sync_render) start_render();
}

void World::catch_up() {
  const double t0 = clock_now();
  for (double i = 0; i < extra_steps; ++i) {
    if (sleeping_) break;
    if (before_update) before_update();
    if (sleeping_) break;
    step();
    lifetime_ += 1.0;
    lfw_->clear_cmds();
    lfw_->clear_broadcasts();
    if (after_update) after_update();
    if (clock_now() - t0 >= extra_step_budget_ms) break;
  }
}

namespace {
// `new Ticker({ step_ms: () => this.base_step_ms(), on_step: dt => {...} })` 的那个 options。
class WorldUpdateOptions : public ITickerOptions {
 public:
  explicit WorldUpdateOptions(World& world) : _world(&world) {}
  double step_ms() override { return _world->base_step_ms(); }
  // TS 的 `Ticker` 在 `on_step` 里 `try/catch` 把异常交给 `World.on_step_error`；端口不装异常
  // （`lfw` 无异常）⇒ 这一层由宿主在驱动 `on_step` 时做（见 DESIGN §77.3）。
  void on_step(double dt) override {
    _world->update_once(dt);
    _world->set_step_error_count(0);
  }

 private:
  World* _world;
};
}

void World::start_update() {
  stop_update();
  base_step_ms();
  TU = 1000 / to_number(dataset.get(u"UPS"));
  update_options_ = std::make_unique<WorldUpdateOptions>(*this);
  update_worker_ = std::make_unique<Ticker>(update_options_.get());
  update_worker_->start();
}

void World::change_bg(const Value& bg_id) {
  const Value cur_id = bg_ != nullptr ? bg_->id() : Value();
  // `if (this.stage.bg.id == bg_id) return;`（松比较）
  if (equals(cur_id, bg_id)) return;

  Value bg_data;
  // `if (bg_id == Defines.RANDOM_BG.id)`
  if (equals(bg_id, field_or(defines_value(u"Defines.RANDOM_BG"), u"id"))) {
    if (truthy(dataset.get(u"LF2_NET"))) {
      bg_data = lfw_->get_random_bg({Value(std::u16string(background_group::kRegular)),
                                     Value(std::u16string(background_group::kHidden))});
    } else {
      bg_data = lfw_->get_random_bg({Value(std::u16string(background_group::kRegular))});
    }
  } else if (truthy(bg_id)) {
    bg_data = lfw_->datas_backgrounds_find(bg_id);
  }
  if (!truthy(bg_data)) bg_data = defines_value(u"Defines.VOID_BG");

  std::unique_ptr<Stage> stage =
      std::make_unique<Stage>(stage_view_.get(), lfw_, defines_value(u"Defines.VOID_STAGE"));
  stage->change_bg(bg_data);
  set_stage(std::move(stage));
}

void World::change_stage(const Value& stage_id) {
  Value stage_data = lfw_->datas_stages_find(stage_id);
  if (!truthy(stage_data)) stage_data = defines_value(u"Defines.VOID_STAGE");
  if (same_ref(stage_data, stage_->data())) return;
  set_stage(std::make_unique<Stage>(stage_view_.get(), lfw_, stage_data));
}

void World::handle_cmds() {
  if (!lfw_->has_cmds()) return;
  lfw_->handle_cmds(*this);
}

Entity* World::find_entity(const std::u16string& id) const { return map_get(entity_map, id); }

std::optional<collision::Collision> World::get_collision(const std::u16string& aid,
                                                        const std::u16string& vid) const {
  return pair_collisions_.first(aid, vid);
}

bool World::has_collision(const std::u16string& aid, const std::u16string& vid) const {
  return pair_collisions_.has(aid, vid);
}

std::vector<collision::Collision> World::get_collisions(const std::u16string& aid,
                                                       const std::u16string& vid,
                                                       std::vector<collision::Collision> out) const {
  return pair_collisions_.collect(aid, vid, std::move(out));
}

double World::weapon_section_at(double x) const { return round(x / kWeaponXSection); }

double World::random_weapon_x(std::optional<double> exclude_section) {
  const double l = left();
  const double r = right();
  const double w = r - l;
  double band_x = 0;
  double band_len = 0;
  if (exclude_section.has_value()) {
    const double half = kWeaponXSection / 2;
    const double bl = max(l, exclude_section.value() * kWeaponXSection - half);
    const double br = min(r, exclude_section.value() * kWeaponXSection + half);
    if (br - bl < w) {
      band_x = bl;
      band_len = max(0.0, br - bl);
    }
  }
  double x = lfw_->mt_range(l, r - band_len);
  if (band_len > 0 && x >= band_x) x += band_len;
  return x;
}

double World::weapon_count_at(double x) const {
  const double key = weapon_section_at(x);
  for (const std::pair<double, double>& kv : ground_weapon_counts) {
    if (kv.first == key) return kv.second;
  }
  return 0;
}

void World::add_count(const std::u16string& key, double o) {
  double value = 0;
  for (const std::pair<std::u16string, double>& kv : counts_) {
    if (kv.first == key) {
      value = kv.second;
      break;
    }
  }
  bool replaced = false;
  for (std::pair<std::u16string, double>& kv : counts_) {
    if (kv.first == key) {
      kv.second = value + o;
      replaced = true;
      break;
    }
  }
  if (!replaced) counts_.emplace_back(key, value + o);
  callbacks.call(u"on_counts", {WorldCallbackArgs{this}});
}

void World::reset_game_time() { game_time_.reset(); }

void World::clear() {
  set_fn_locked(false);
  dataset.set(u"infinity_mp", Value(0.0));
  dataset.set(u"playrate", Value(1.0));
  extra_steps = 0;
  for (Entity* const e : entities) e->set_frame(gone_frame_info());
  for (Entity* const e : ghosts) e->set_frame(gone_frame_info());
  for (std::pair<std::u16string, buff::Buff*>& kv : buffs) kv.second->set_duration(0);
  if (stage_->id() != std::u16string(u"VOID_STAGE")) {
    set_stage(std::make_unique<Stage>(stage_view_.get(), lfw_, defines_value(u"Defines.VOID_STAGE")));
  }
  if (!strict_equals(bg_->id(), field_or(defines_value(u"Defines.VOID_BG"), u"id"))) {
    stage_->change_bg(defines_value(u"Defines.VOID_BG"));
  }
  set_paused(false);
  camera_->reset();
  alive_players_.clear();
  has_players_alive = false;
  callbacks.call(u"on_counts", {WorldCallbackArgs{this}});
  counts_.clear();
}

void World::dispose() {
  callbacks.call(u"on_disposed", {WorldCallbackArgs{this}});
  stop_update();
  stop_render();
  const std::vector<Entity*> es(entities.begin(), entities.end());
  del_entities(es);
  const std::vector<Entity*> gs(ghosts.begin(), ghosts.end());
  del_entities(gs);
  alive_players_.clear();
  has_players_alive = false;
  renderer_->dispose();
  callbacks.clear();
}

void World::calc_alives() {
  puppet_teams.clear();
  team_alive_counts_.clear();
  for (Entity* const e : entities) {
    if (!entity::is_fighter(ref_of(*e))) continue;
    if (e->hp() <= 0) continue;
    const std::u16string team = e->team();
    double count = 0;
    for (const std::pair<std::u16string, double>& kv : team_alive_counts_) {
      if (kv.first == team) {
        count = kv.second;
        break;
      }
    }
    bool replaced = false;
    for (std::pair<std::u16string, double>& kv : team_alive_counts_) {
      if (kv.first == team) {
        kv.second = count + 1;
        replaced = true;
        break;
      }
    }
    if (!replaced) team_alive_counts_.emplace_back(team, count + 1);
    if (e->puppet) {
      if (!vec_has(puppet_teams, team)) puppet_teams.push_back(team);
    }
  }
}

std::u16string World::game_result(bool refresh) {
  if (refresh) calc_alives();
  if (team_alive_counts_.size() > 1) return std::u16string();
  if (team_alive_counts_.size() <= 0) return std::u16string(u"drawn");
  if (stage_->id() == std::u16string(u"VOID_STAGE")) return std::u16string(u"over");
  bool puppet_team_alive = false;
  for (const std::u16string& team : puppet_teams) {
    for (const std::pair<std::u16string, double>& kv : team_alive_counts_) {
      if (kv.first == team) {
        puppet_team_alive = true;
        break;
      }
    }
    if (puppet_team_alive) break;
  }
  if (!puppet_team_alive) return std::u16string(u"over");
  if (!stage_->is_stage_finish() || !stage_->is_chapter_finish()) return std::u16string();
  return std::u16string(u"win");
}

}

