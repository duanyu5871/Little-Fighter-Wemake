#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lfw/base/expression.h"
#include "lfw/buff/buff.h"
#include "lfw/collision/collision.h"
#include "lfw/collision/keeper.h"
#include "lfw/core/value.h"
#include "lfw/entity/entity_collision_view.h"

namespace lfw {

class Entity;
class World;

// `src/LFW` 的 `collision/*` 直接引用 `world.*` / `lfw.*` / 实体属性；端口的
// `collision/` 层把这些都留成了 Env 缝（82 条）。本类是**唯一**的接线点：把 82 条缝接到
// 真实的 `World` / `Entity` / 宿主 `lfw` 面上，并给 `collision/` 层提供两个 `Entity` 视图
// （`EntityHandlerView` / `EntityWeaponView` / `EntityActionView`）。
//
// 保真要点：
// 1. `CollisionActor` 是**快照**（`collision/` 层看不见 `Entity`）⇒ 宿主负责 `Entity` →
//    `CollisionActor` 的投影，字段对应见 `actor_of` 上的注释（TS 是直接读活实体）。
// 2. 两条缝（`victim_get_v_rest` / `attacker_is_ally`）签名里没有对方实体，TS 那边是
//    `victim.get_v_rest(c.aid)` / `attacker.is_ally(victim)` ⇒ 宿主要知道「当前判定的那一对」。
//    它由本类的 `collision_get` / `collision_test` 包装器登记（`collision/` 层的入口只有这两个）。
// 3. 需要「活实体」的缝（`find_entity` / `get_bounding` / `victim_add_v_rest` …）一律按 id 现查
//    `world.find_entity`，不缓存实体指针。
class WorldCollisionHost : public ICollisionViewHost {
 public:
  explicit WorldCollisionHost(World& world);
  ~WorldCollisionHost();

  WorldCollisionHost(const WorldCollisionHost&) = delete;
  WorldCollisionHost& operator=(const WorldCollisionHost&) = delete;

  World& world() const { return *_world; }
  const collision::CollisionCoreEnv& core() const { return _core; }
  const collision::CollisionKeeper& keeper() const { return collision::collisions_keeper(); }

  // `collision_get(a, b)` / `collision_test(c)`：登记当前那一对后转调 `collision/` 层。
  collision::Collision* collision_get(Entity& a, Entity& v);
  bool collision_test(collision::Collision& c);
  // `collisions_keeper.handle(c)`：装好 `c.env` / `c.core` 再转调。
  void handle(collision::Collision& c);
  // `acquire_collision` 每次给新对象（见 `acquire_collision` 的注释）⇒ `World::step`
  // 开头清碰撞表时整批释放。
  void reset_collisions();

  // `Entity` → `CollisionActor` 快照（TS 侧是那个活实体本身）。
  collision::CollisionActor actor_of(Entity& e) const;
  collision::CollisionActor actor_of_id(const std::u16string& id) const;

  // `ICollisionViewHost`
  EntityHandlerView* handler_view(Entity* e) override;
  EntityWeaponView* weapon_view(Entity* e) override;
  EntityActionView* action_view(Entity* e) override;
  buff::IBuffEntity* buff_view(Entity* e) override;
  Entity* entity_by_id(const std::u16string& id) override;
  double mt_range(double min, double max) override;
  void mt_mark(const std::u16string& mark) override;

 private:
  void bind();
  void bind_core();
  void bind_keeper();
  void bind_handlers();
  void bind_action();
  void bind_handlers2();
  void bind_handlers3();
  void bind_handlers4();
  void bind_fall();
  void bind_nbdy_normal();
  void bind_nbd_defend();
  void bind_weapon_is_hit();
  void bind_ball_frozen();
  void bind_healing();
  void bind_buff_env();

  // `game/` 之外那一层的两个入口：`collision/*` 的 Env 只有「id 或 `Collision&`」，
  // 而 TS 是直接对活实体动手 ⇒ 这几个小工具做 id → `Entity`（找不到给 `nullptr`）。
  Entity* entity_of_collision_a(const collision::Collision& c) const;
  Entity* entity_of_collision_v(const collision::Collision& c) const;
  // `is_fall` / `is_armor_work` / `calc_itr_velocity` / `calc_stiffness` 在端口里收的是
  // **`Value` 形状**的碰撞（那一刀是用用例数据喂的）⇒ 宿主按 TS 的读法投影一次。
  Value entity_helpers_value(Entity& e) const;
  Value collision_helpers_value(const collision::Collision& c) const;
  Value world_helpers_value() const;
  // `action.tester` / `bdy.__tester`：端口存源串（DESIGN §64.2），宿主按需编译。
  bool tester_run(const Value& tester, collision::Collision& c);
  Value tester_debug(const Value& tester);

  collision::Collision& acquire_collision();
  buff::Buff* find_buff(const std::u16string& id) const;
  buff::Buff* create_buff(const std::u16string& kind, const std::u16string& id);

  World* _world = nullptr;
  // 当前判定的那一对（见类注释第 2 条）。
  Entity* _cur_a = nullptr;
  Entity* _cur_v = nullptr;

  collision::CollisionCoreEnv _core;
  collision::KeeperEnv _keeper;
  collision::HandlersEnv _handlers;
  collision::ActionEnv _action;
  collision::Handlers2Env _handlers2;
  collision::Handlers3Env _handlers3;
  collision::Handlers4Env _handlers4;
  collision::FallEnv _fall;
  collision::NbdyNormalEnv _nbdy_normal;
  collision::NbdDefendEnv _nbd_defend;
  collision::WeaponIsHitEnv _weapon_is_hit;
  collision::BallFrozenEnv _ball_frozen;
  collision::HealingEnv _healing;
  buff::BuffEnv _buff_env;

  std::map<Entity*, std::unique_ptr<EntityHandlerView>> _handler_views;
  std::map<Entity*, std::unique_ptr<EntityWeaponView>> _weapon_views;
  std::map<Entity*, std::unique_ptr<EntityActionView>> _action_views;

  // `lfw.acquire_collision()`：TS 的对象池 `Graves`（`LFW.ts:891`）**没有任何**
  // `recycle_collision` 调用者 ⇒ 池永远是空的 ⇒ 每次都是新对象。端口按同样的行为给「每次
  // 新对象」，但用固定大小的环槽承载（引用要活到调用者拷走为止）。
  std::vector<std::unique_ptr<collision::Collision>> _collisions;
  size_t _collision_at = 0;

  std::map<std::u16string, Expression<collision::Collision>> _testers;
};

}
