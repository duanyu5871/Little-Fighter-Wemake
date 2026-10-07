#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/fps.h"
#include "lfw/base/no_emit_callbacks.h"
#include "lfw/bg/background.h"
#include "lfw/buff/buff.h"
#include "lfw/camera.h"
#include "lfw/collision/collision.h"
#include "lfw/controller/base_controller.h"
#include "lfw/core/value.h"
#include "lfw/defines/i_vector3.h"
#include "lfw/entity/entity.h"
#include "lfw/ground.h"
#include "lfw/player_info.h"
#include "lfw/stage/stage.h"
#include "lfw/transform.h"
#include "lfw/utils/container_help/nested_multi_map.h"
#include "lfw/utils/times.h"
#include "lfw/world_dataset.h"

namespace lfw {

class ITickerOptions;
class Ticker;
class World;
class WorldCollisionHost;

// `Stage` 在 `lfw::stage` 里（TS 的 `stage/Stage.ts`），本文件里用得最多，起个本地别名。
using stage::Stage;

// `Callbacks<IWorldCallbacks>` 的端口替身：TS 每个回调的参数都塞进这个包（`on_cam_move` 两个
// 数、`on_stage_change` 两个 `Stage`、`on_fighter_*` 一个 `Entity`、`on_dataset_change` 一个键
// 加两个值、`on_ups_update` 三个数）。`world` 一律是「触发这次回调的 World」。
struct WorldCallbackArgs {
  World* world = nullptr;
  Entity* entity = nullptr;
  Stage* stage = nullptr;
  Stage* prev_stage = nullptr;
  Value value = Value(NullTag{});
  Value prev = Value(NullTag{});
  std::u16string key;
  double num = 0.0;
  double num2 = 0.0;
  double num3 = 0.0;
  bool flag = false;
};
using WorldCallbacks = CallbacksT<WorldCallbackArgs>;

// `lfw.layers.at(i)?.ui`（`World::update_ui`）：UI 组件未移植 ⇒ 宿主给这一面。
class IWorldUi {
 public:
  virtual ~IWorldUi() = default;
  virtual bool disabled() const = 0;
  virtual void update(double dt) = 0;
};

// `this.lfw.*`：`LFW` 未移植的那一面。继承 `stage::IStageLfw` —— `Stage` 那一刀开的缝正是
// 「`LFW` 未移植的那一面」，`World` 要的是同一个 `lfw` 对象。
class IWorldLfw : public stage::IStageLfw {
 public:
  // `lfw.players.get(player_id)`
  virtual PlayerInfo* player(const Value& player_id) = 0;
  // `lfw.datas.get_random_bg([...])`
  virtual Value get_random_bg(const std::vector<Value>& groups) = 0;
  // `lfw.factory.create_entity(this, data)`
  virtual Entity* create_entity(World& world, const Value& data) = 0;
  // `lfw.factory.recycle_entity(e)` / `lfw.factory.recycle_buff(b)`
  virtual void recycle_entity(Entity* e) = 0;
  virtual void recycle_buff(buff::Buff* b) = 0;
  // `lfw.factory.acquire_ctrl(InvalidController, "", this)` / `lfw.factory.release_ctrl(ctrl)`
  virtual controller::BaseController* acquire_invalid_ctrl(World& world) = 0;
  virtual void release_ctrl(controller::BaseController* ctrl) = 0;
  // `lfw.layers` 那些层的 `ui`（`update_ui` 倒序遍历）
  virtual std::vector<IWorldUi*> layer_uis() = 0;
  // `lfw.mt.range(min, max)`
  virtual double mt_range(double min_v, double max_v) = 0;
  // `lfw.is_cheat(name)`
  virtual bool is_cheat(const std::u16string& name) = 0;
  // `lfw.new_id`
  virtual std::u16string new_id() = 0;
  // `lfw.datas.find_bot(bot_id)`（`BotController.check_bot`；`DatMgr` 未移植 ⇒ 默认找不到）
  virtual Value find_bot(const std::u16string& bot_id) const {
    (void)bot_id;
    return Value();
  }
  // `lfw.factory.create_buff(kind, lfw, id)`（碰撞的 buff 缝：`handle_itr_kind_magic_flute`
  // 的 `create_buff` / `grant_buff` 的 `create_buff`）。`LFW` 未移植 ⇒ 宿主给这一面；
  // 默认「造不出」对应 TS 拿到 falsy 的那条路径。
  virtual buff::Buff* create_buff(const std::u16string& kind, const std::u16string& id) {
    (void)kind;
    (void)id;
    return nullptr;
  }
  // `lfw.broadcasts` 与 `lfw.broadcast(m)`（`Entity::broadcast` 转发）
  virtual void broadcast(const Value& m) = 0;
  // `lfw.factory.create_ctrl(data.id, this.ctrl.player_id, this)`（`Entity::transform` 转发）
  virtual controller::BaseController* create_ctrl(const std::u16string& data_id,
                                                 const std::u16string& player_id) = 0;
  // `lfw.cmds`（`handle_cmds` 的 `if (!cmds.length) return;`）
  virtual bool has_cmds() const = 0;
  // `lfw.cmds.length = 0` / `lfw.broadcasts.length = 0`（`update_once` / `catch_up` 每步清空）
  virtual void clear_cmds() = 0;
  virtual void clear_broadcasts() = 0;
  // `ctrl.update_lookup(i, entities)`（`step` 的每两帧一次；`BotController` / `BallController`
  // 都未移植 ⇒ 宿主回答）
  virtual void ctrl_update_lookup(controller::BaseController& ctrl, double index,
                                  std::vector<Entity*>& entities) = 0;
  // `Ditto.DEV` / `Ditto.debug(msg)`（`step` 里那个实体数超限的调试打印）
  virtual bool dev() const = 0;
  virtual void debug(const std::u16string& msg) = 0;
  // `lfw.survival_rank_mode` / `lfw.survival_rank_available`（`IEntityHost` 转发）
  virtual bool survival_rank_mode() const = 0;
  virtual bool survival_rank_available() const = 0;
  // `CMDS.handle(this, cmds)`
  virtual void handle_cmds(World& world) = 0;
  // `ctrl.come(x, y, z)` / `ctrl.move()` / `ctrl.stay()` / `ctrl.follow(target)` / `ctrl.goingto`
  // （`BotController` 未移植 ⇒ 宿主回答）
  virtual void ctrl_come(controller::BaseController& ctrl, double x, double y, double z) = 0;
  virtual void ctrl_move(controller::BaseController& ctrl) = 0;
  virtual void ctrl_stay(controller::BaseController& ctrl) = 0;
  virtual void ctrl_follow(controller::BaseController& ctrl, Entity& target) = 0;
  virtual bool ctrl_goingto(const controller::BaseController& ctrl) const = 0;
  // `Ditto.warn(...)`
  virtual void warn(const std::u16string& text) = 0;
};

// `this.renderer`（TS `Ditto.WorldRender`）：渲染未移植 ⇒ 宿主给这一面。
class IWorldRenderer {
 public:
  virtual ~IWorldRenderer() = default;
  virtual void add_entity(Entity& e) = 0;
  virtual void del_entity(Entity& e) = 0;
  virtual void render(double dt) = 0;
  virtual void dispose() = 0;
};

// TS `World.ts`：世界（实体表 / 边界 / 相机 / 场景 / 计分）。
//
// 两类宿主面各有一个视图实现（定义在 `world.cpp` 里）：`entity::IEntityHost`（`Entity` 要的
// `world.*` / `lfw.*`）与 `stage::IStageWorld`（`Stage` 要的）。`ICameraWorld` 由本类直接实现
// —— 那三个方法名（`world_stage` / `world_bg` / `world_dataset`）与 TS 的字段名不撞。
class World : public ICameraWorld {
 public:
  static constexpr const char* TAG = "World";

  World(IWorldLfw& lfw, IWorldRenderer& renderer, state::States* states);
  ~World() override;

  World(const World&) = delete;
  World& operator=(const World&) = delete;

  // ---- TS 的公开字段（`readonly`）----
  WorldDataset dataset;
  WorldCallbacks callbacks;
  std::vector<std::pair<std::u16string, buff::Buff*>> buffs;
  Ground ground;
  Transform transform;
  // `Map<string, Entity>` —— JS 的 `Map` 是**插入序**，覆盖同键时位置不变 ⇒ 端口用 vector。
  std::vector<std::pair<std::u16string, Entity*>> entity_map;
  std::vector<Entity*> entities;
  std::vector<Entity*> ghosts;
  std::vector<std::pair<std::u16string, Entity*>> puppets;
  // `Set<string>`（插入序 + 判重）。
  std::vector<std::u16string> puppet_teams;
  std::vector<std::pair<std::u16string, collision::Collision>> collisions;
  double pairs_compared = 0.0;
  double render_cost = 0.0;
  bool has_players_alive = false;
  double TU = 1.0;
  double extra_steps = 0.0;
  double extra_step_budget_ms = 16.0;
  std::vector<std::pair<double, double>> ground_weapon_counts;

  IWorldLfw& lfw() const { return *lfw_; }
  IWorldRenderer& renderer() const { return *renderer_; }
  Camera& camera() const { return *camera_; }
  // `Entity` / `Stage` 拿的宿主面（视图）。
  IEntityHost& host() const { return *host_; }
  stage::IStageWorld& stage_world() const { return *stage_view_; }
  state::States* states() const { return states_; }
  // 碰撞那一刀的宿主（`World::step` 的配对 / `collisions_keeper.handle` 都要它）。
  // 首次用到时才建：它会把 12 个 Env 装到 `collision/` 层的全局缝上（同一时刻只应有一个
  // 活的 World 在用 —— 差分台面一次只跑一个用例，见 DESIGN §78.5）。
  WorldCollisionHost& collision_host();

  // ---- `get bg` / `set bg`：换背景时顺带把缩放交给 transform，并 dispose 旧的 ----
  Background* bg() const { return bg_.get(); }
  void set_bg(std::unique_ptr<Background> v);
  // ---- `get stage` / `set stage` ----
  Stage* stage() const { return stage_.get(); }
  void set_stage(std::unique_ptr<Stage> v);

  // ---- `get player_l()` … `get middle()`：转发 `stage` ----
  double player_l() const;
  double player_r() const;
  double left() const;
  double right() const;
  double near_plane() const;
  double far_plane() const;
  double width() const;
  double depth() const;
  const Background::Middle& middle() const;

  bool paused() const { return paused_ == 1.0; }
  void set_paused(bool v);
  void set_paused_value(double v);
  bool fn_locked() const { return fn_locked_ == 1.0; }
  void set_fn_locked(bool v);
  void set_fn_locked_value(double v);
  const std::vector<std::pair<std::u16string, double>>& counts() const { return counts_; }
  double game_time() const { return game_time_.value(); }
  double lifetime() const { return lifetime_; }
  // 差分台面用的只读探针（TS 里是私有字段）。
  double paused_value() const { return paused_; }
  double fn_locked_value() const { return fn_locked_; }
  bool sleeping() const { return sleeping_; }
  bool need_fps() const { return need_FPS_; }
  bool need_ups() const { return need_UPS_; }
  const FPS& fps() const { return fps_; }
  int render_worker_id() const { return render_worker_id_; }
  Ticker* ticker() const { return update_worker_.get(); }
  const std::vector<std::pair<std::u16string, double>>& team_alive_counts() const {
    return team_alive_counts_;
  }
  // `stage_limit` / `stage.world_pause` / `stage.is_stage_finish` …
  bool stage_limit() const;
  bool world_pause() const;
  bool is_stage_finish() const;
  bool is_chapter_finish() const;

  // ---- `ICameraWorld`：TS 里是 `world.stage` / `world.bg` / `world.dataset` 三次属性读 ----
  Value world_stage() override;
  Value world_bg() override;
  Value world_dataset() override;
  // `CollisionCoreEnv::dataset` 用的同一份快照（`world.dataset`）。
  Value dataset_value() const;
  // `new Background(this, data)` 那一处要的 `World*`（`Background` 只前置声明了它）。
  World* world_ptr() { return this; }

  // ---- TS 的方法 ----
  void on_dataset_change(const std::u16string& key, const Value& curr, const Value& prev);  void team_come(const std::u16string& team, double x, double y, double z);
  void team_move(const std::u16string& team);
  void team_stay(const std::u16string& team);
  void team_follow(Entity& target);
  // `add_entities(...entities)`：TS 是变参，端口给单个（调用点都是单个）。
  void add_entities(Entity& e);
  const std::vector<Entity*>& list_entities(const std::u16string& name,
                                            const std::function<bool(Entity&)>& predicate);
  void del_entity(Entity& e);
  void del_entities(const std::vector<Entity*>& list);
  double fps_value() const;
  void stop_render();
  void start_render();
  void stop_update();
  void sleep();
  void awake();
  double base_step_ms();
  // `on_step_error(e)`：`Ticker` 的 `on_step` 里 catch 到异常时调（端口由台面/宿主调）。
  void on_step_error(const std::u16string& message, bool has_errors);
  // ---- `step` / `update_once` / `catch_up` / `start_update` ----
  void step();
  void update_once(double dt);
  void catch_up();
  void start_update();
  // TS 的公开钩子字段（`this.before_update?.()` / `this.after_update?.()`）。
  std::function<void()> before_update;
  std::function<void()> after_update;
  std::vector<double> get_bound(Entity& e);
  const Vector3& restrict(Entity& e);
  void update_ui();
  void change_bg(const Value& bg_id);
  void change_stage(const Value& stage_id);
  void handle_cmds();
  void mark_players_alive(Entity& e, bool is_alive);
  void collect_dead_buff(buff::Buff& b);
  void add_collision(const collision::Collision& c);
  void render_once(double dt);
  void update_camera();
  void spark(double x, double y, double z, const std::u16string& f);
  void etc(double x, double y, double z, const std::u16string& f);
  Value get_bounding(Entity& e, const Value& frame, const Value& info);
  void dispose();
  void add_count(const std::u16string& key, double o);
  void clear();
  void reset_game_time();
  Entity* find_entity(const std::u16string& id) const;
  std::optional<collision::Collision> get_collision(const std::u16string& aid,
                                                   const std::u16string& vid) const;
  bool has_collision(const std::u16string& aid, const std::u16string& vid) const;
  std::vector<collision::Collision> get_collisions(const std::u16string& aid,
                                                   const std::u16string& vid,
                                                   std::vector<collision::Collision> out = {}) const;
  double weapon_section_at(double x) const;
  double random_weapon_x(std::optional<double> exclude_section = std::nullopt);
  double weapon_count_at(double x) const;
  void calc_alives();
  std::u16string game_result(bool refresh);

  // `_gones` / `_alive_players` / `_paused` / … 的只读探针（台面读，TS 里是私有字段）。
  const std::vector<Entity*>& gones() const { return gones_; }
  size_t alive_players_size() const { return alive_players_.size(); }
  const std::vector<buff::Buff*>& dead_buffs() const { return dead_buffs_; }
  double step_error_count() const { return step_error_count_; }
  // TS 的 `this._step_error_count = 0`（`Ticker` 的 `on_step` 成功后复位）。
  void set_step_error_count(double v) { step_error_count_ = v; }

  // `step()` 那一半的挂点（台面写，TS 里是公开字段）。
  void set_need_fps(bool v) { need_FPS_ = v; }
  void set_need_ups(bool v) { need_UPS_ = v; }

 private:
  // `_disposers` 那类「插入序 + 判重」的小表（JS `Map` / `Set` 语义）。
  static Entity* map_get(const std::vector<std::pair<std::u16string, Entity*>>& m,
                         const std::u16string& key);
  static void map_set(std::vector<std::pair<std::u16string, Entity*>>& m, std::u16string key,
                      Entity* value);
  static bool vec_has(const std::vector<std::u16string>& v, const std::u16string& key);

  IWorldLfw* lfw_ = nullptr;
  IWorldRenderer* renderer_ = nullptr;
  state::States* states_ = nullptr;
  std::unique_ptr<IEntityHost> host_;
  std::unique_ptr<stage::IStageWorld> stage_view_;
  std::unique_ptr<Background> bg_;
  std::unique_ptr<Stage> stage_;
  std::unique_ptr<Camera> camera_;
  std::unique_ptr<WorldCollisionHost> collision_host_;
  bool sleeping_ = false;
  Value spark_data_ = Value(NullTag{});
  Value etc_data_ = Value(NullTag{});
  bool need_FPS_ = true;
  bool need_UPS_ = true;
  FPS fps_{0.9};
  double lifetime_ = 0.0;
  int render_worker_id_ = 0;
  // `start_render` 闭包里的 `prev_time` / `fix_radio`（每次调用重新开始）。
  double render_prev_time_ = 0.0;
  double render_fix_radio_ = 1.0;
  std::unique_ptr<Ticker> update_worker_;
  // `new Ticker({step_ms, on_step})` 的那个 options 对象（`World` 持有，`Ticker` 借用）。
  std::unique_ptr<ITickerOptions> update_options_;
  std::map<std::u16string, std::vector<Entity*>> entities_map_;
  std::vector<buff::Buff*> dead_buffs_;
  Times game_time_;
  std::vector<std::pair<std::u16string, double>> counts_;
  std::vector<Entity*> gones_;
  double paused_ = 0.0;
  double fn_locked_ = 0.0;
  std::vector<std::pair<std::u16string, double>> team_alive_counts_;
  std::vector<Entity*> alive_players_;
  NestedMultiMap<std::u16string, std::u16string, collision::Collision> pair_collisions_;
  double step_error_count_ = 0.0;
  double step_error_time_ = 0.0;
  Vector3 restrict_result_;
  std::vector<double> bound_{0.0, 0.0, 0.0, 0.0};
};

}
