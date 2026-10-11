#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/entity/entity.h"
#include "lfw/ui/component/sum_info.h"
#include "lfw/ui/component/ui_component.h"
#include "lfw/utils/times.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

// TS `ui/component/SummaryLogic.ts`（TAGS = ["SummaryLogic"]）：战况汇总。
// `fighters` / `players` / `teams` 在 TS 里是 `Map`（插入序）⇒ 端口用插入序的
// vector<pair>（同 `world.entity_map` 的口径）；`losing_teams` 照抄 `Set` 的插入序 + 判重。
class SummaryLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "SummaryLogic";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  explicit SummaryLogic(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }

  std::vector<std::pair<std::u16string, FighterSumInfo>> fighters;
  std::vector<std::pair<std::u16string, PlayerSumInfo>> players;
  std::vector<std::pair<std::u16string, TeamSumInfo>> teams;
  std::vector<std::u16string> losing_teams;

  TeamSumInfo& team_sum(const std::u16string& team);
  FighterSumInfo& fighter_sum(Entity& e);
  PlayerSumInfo& player_sum(Entity& e);

  bool is_losing(const std::u16string& team) const;
  void add_losing(const std::u16string& team);
  void del_losing(const std::u16string& team);

  // 台面观察口（不参与逻辑）：存活监听表大小。
  size_t cb_map_count() const { return _summary_cb_map.size(); }

  void on_fighter_add(Entity& e);
  void on_fighter_del(Entity& e);

  void on_start() override;
  void on_stop() override;
  void update(double dt) override;

  // 回调处理（监听器转发进来；`ref` 是实体引用值）。
  void handle_dead(const Value& ref);
  void handle_hp_changed(const Value& ref, const Value& value, const Value& prev);
  void handle_mp_changed(const Value& ref, const Value& value, const Value& prev);
  void handle_reserve_changed(const Value& ref, const Value& value, const Value& prev);
  void handle_damage_sum_changed(Entity& e, const Value& value, const Value& prev);
  void handle_kill_sum_changed(Entity& e, const Value& value, const Value& prev);
  void handle_picking_sum_changed(Entity& e, const Value& value, const Value& prev);

  Entity* entity_of(const Value& ref);
  static std::u16string data_id_of(const Entity& e);

 private:
  using ValueListener = NoEmitCallbacksT<Value>::Listener;
  using ValuePayloads = NoEmitCallbacksT<Value>::Payloads;
  using WorldListener = NoEmitCallbacksT<WorldCallbackArgs>::Listener;
  using WorldPayloads = NoEmitCallbacksT<WorldCallbackArgs>::Payloads;

  struct WorldListenerT : WorldListener {
    explicit WorldListenerT(SummaryLogic* o);
  };
  struct FighterListenerT : ValueListener {
    explicit FighterListenerT(SummaryLogic* o);
  };
  struct SummaryListenerT : ValueListener {
    SummaryListenerT(SummaryLogic* o, Entity* e);
  };

  WorldListenerT _world_cb{this};
  FighterListenerT _fighter_cb{this};
  std::map<std::u16string, std::unique_ptr<SummaryListenerT>> _summary_cb_map;
  // TS `_summary_cb_map.set` 覆盖时旧回调对象仍留在 summary callbacks 里（泄漏但继续被调）
  // ⇒ 端口把它移到这条保活队列（行为一致，不悬垂）。
  std::vector<std::unique_ptr<SummaryListenerT>> _summary_cb_trash;
  Times _refresh_timer{0.0, 300.0};
  std::vector<std::u16string> _temps;
};

}
}
