#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "lfw/collision/collision.h"
#include "lfw/core/value.h"
#include "lfw/defines/bdy_kind.h"
#include "lfw/defines/entity_enum.h"
#include "lfw/defines/itr_kind.h"
#include "lfw/defines/state_enum.h"
#include "lfw/utils/container_help/nested_map.h"

namespace lfw {
namespace collision {

struct KeeperEntry {
  std::u16string fn;
  bool has_a_state = false;
  bool has_v_state = false;
  std::vector<StateEnum> a_state;
  std::vector<StateEnum> v_state;
};

struct HandlerConfig {
  std::vector<EntityEnum> a_type;
  std::vector<ItrKind> itr;
  std::vector<EntityEnum> v_type;
  std::vector<BdyKind> bdy;
  std::u16string handler;
  std::vector<StateEnum> a_state;
  std::vector<StateEnum> v_state;
  bool has_a_state = false;
  bool has_v_state = false;
};

struct KeeperEnv {
  std::function<double()> attacker_state;
  std::function<double()> victim_state;
  // `victim.data`（`handle` 末尾读 `victim.data.base.hit_sounds`）。缝里没有实体
  // （见 `handlers2.h` 那批缝的同一处境）⇒ 宿主按「当前判定的那一对」回答。
  std::function<Value()> victim_data;
  std::function<void(const std::u16string& fn, Collision& c)> call_handler;  std::function<bool(CollisionActor& first, CollisionActor& second, const Value& itr)>
      ball_frozen;
  std::function<void(const std::u16string& type, const Value& action, Collision& c)> run_action;
  std::function<void(Collision& c)> victim_push_collided;
  std::function<void(Collision& c)> attacker_push_collision;
  std::function<void(const Value& sounds)> victim_play_sound;
};

const KeeperEnv& keeper_env();
void set_keeper_env(const KeeperEnv& env);

class CollisionKeeper {
 public:
  void add(const std::vector<EntityEnum>& a_type_list, const std::vector<ItrKind>& itr_kind_list,
           const std::vector<EntityEnum>& v_type_list, const std::vector<BdyKind>& bdy_kind_list,
           const std::u16string& fn, const std::vector<StateEnum>* a_state_list,
           const std::vector<StateEnum>* v_state_list);

  void register_configs(const std::vector<HandlerConfig>& configs);

  bool load_handlers(Collision& collision) const;

  void handle(Collision& collision) const;

 private:
  NestedMap<double, double, std::vector<KeeperEntry>> _pair_map;
};

const CollisionKeeper& collisions_keeper();

}
}
