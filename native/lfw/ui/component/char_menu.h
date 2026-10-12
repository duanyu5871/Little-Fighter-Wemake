#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lfw/base/fsm.h"
#include "lfw/helper/randoming.h"
#include "lfw/ui/component/ui_component.h"
#include "lfw/ui/ui_event.h"

namespace lfw {

class LFW;
class PlayerInfo;

namespace ui {

class CharMenuHead;
class FighterName;
class GamePrepareLogic;
class PlayerName;
class PlayerTeamName;
class UINode;

// TS `ui/component/CharMenu/CharMenuState.ts`。
enum class CharMenuState {
  PlayerSel,
  CountingDown,
  ComNumSel,
  ComSel,
  GameSetting,
};

std::u16string char_menu_state_str(CharMenuState s);

// TS `SlotStep.ts`。
enum class SlotStep {
  FighterSel = 0,
  TeamSel = 1,
  Ready = 2,
};

// TS `SlotState.ts`（`fighter: IEntityData | null`；端口用 `NullTag` 表示 `null`）。
struct SlotState {
  Value fighter = Value(NullTag{});
  bool random = true;
  SlotStep step = SlotStep::FighterSel;
  std::u16string team;
};

// TS `ISlotPack.ts`。
struct ISlotPack {
  CharMenuHead* head = nullptr;
  PlayerName* player_name = nullptr;
  FighterName* fighter_name = nullptr;
  PlayerTeamName* team_name = nullptr;
};

class CharMenuLogic;

// TS `CharMenuState_Base.ts`（`ICharMenuState = IState<CharMenuState> & { on_key_down? }`）。
class CharMenuState_Base : public IState {
 public:
  CharMenuState_Base(CharMenuState key, CharMenuLogic& owner);

  Value key() const override;
  virtual void on_key_down(LFWKeyEvent& e) { (void)e; }
  std::optional<Value> update(double dt) override {
    (void)dt;
    return std::nullopt;
  }
  void enter() override {}
  void leave() override {}

  CharMenuState state_key;

 protected:
  CharMenuLogic& owner;
  LFW& lfw;
};

class CharMenuState_PlayerSel : public CharMenuState_Base {
 public:
  explicit CharMenuState_PlayerSel(CharMenuLogic& owner);

  void on_key_down(LFWKeyEvent& e) override;
  void enter() override;
  std::optional<Value> update(double dt) override;
};

class CharMenuState_CountingDown : public CharMenuState_Base {
 public:
  explicit CharMenuState_CountingDown(CharMenuLogic& owner);

  void enter() override;
  void on_key_down(LFWKeyEvent& e) override;
  std::optional<Value> update(double dt) override;
  void leave() override;

 private:
  double _count_down = 5000.0;
};

class CharMenuState_ComNumSel : public CharMenuState_Base {
 public:
  explicit CharMenuState_ComNumSel(CharMenuLogic& owner);

  void enter() override;
  std::optional<Value> update(double dt) override;
  void leave() override;
  void on_key_down(LFWKeyEvent& e) override;

 private:
  UINode* how_many_coms = nullptr;
  double com_num = -1.0;
  std::vector<UINode*> btns;
};

class CharMenuState_ComSel : public CharMenuState_Base {
 public:
  explicit CharMenuState_ComSel(CharMenuLogic& owner);

  bool all_com_ready();

  void enter() override;
  void on_key_down(LFWKeyEvent& e) override;
  std::optional<Value> update(double dt) override;
};

class CharMenuState_GameSetting : public CharMenuState_Base {
 public:
  explicit CharMenuState_GameSetting(CharMenuLogic& owner);

  void enter() override;
  void leave() override;

 private:
  UINode* game_settings_menu = nullptr;
};

// TS `ui/component/CharMenu/CharMenuLogic.ts`。
class CharMenuLogic : public UIComponent {
 public:
  static constexpr const char* TAG = "CharMenuLogic";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();
  static const Value& PROPS();

  CharMenuLogic(UINode& layout, const std::u16string& f_name, const Value& info);
  ~CharMenuLogic() override;

  const ClazzTag* clazz() const override { return class_tag(); }
  const std::u16string& props_tag() const override;
  const Value& props_meta() const override { return PROPS(); }
  FSM* fsm() override { return &_fsm; }
  FSM& fsm_ref() { return _fsm; }

  std::vector<std::pair<PlayerInfo*, SlotState>> prev_players;
  std::vector<std::pair<PlayerInfo*, SlotState>> players;
  double com_num = 0.0;
  std::vector<ISlotPack> slots;

  double max_player();
  void set_max_player(double v);
  double min_player();
  void set_min_player(double v);
  double max_coms();
  double min_coms();
  std::vector<std::u16string> teams();
  void set_teams(std::vector<std::u16string> v);
  std::optional<std::vector<std::u16string>> oids();
  void set_oids(const std::optional<std::vector<std::u16string>>& v);

  std::vector<Value> filter_oids(const std::vector<Value>& list);
  void rebuild_randoming();
  void ensure_randoming();
  // `this._randoming?.get() ?? null`。
  Value randoming_next();
  std::vector<Value> fighters();

  void on_start() override;
  void on_resume() override;
  void on_stop() override;
  void on_key_down(LFWKeyEvent& e) override;
  void update(double dt) override;

  void reset();
  void update_random();
  void cancel_counting_down(PlayerInfo* player);
  void update_slots();
  void press_a(PlayerInfo* player);
  bool is_player_sel();
  void press_j(PlayerInfo* player);
  void press_lr(PlayerInfo* player, double dir);
  void pick_lr(double dir, std::optional<double> index = std::nullopt);
  void press_u(PlayerInfo* player);
  void handle_fighters_changed();

  std::optional<std::pair<PlayerInfo*, SlotState*>> last_player();
  std::optional<std::pair<PlayerInfo*, SlotState*>> last_com();
  void add_com();

  SlotState* find_state(std::vector<std::pair<PlayerInfo*, SlotState>>& map, PlayerInfo* player);
  bool map_has(PlayerInfo* player);
  void map_erase(std::vector<std::pair<PlayerInfo*, SlotState>>& map, PlayerInfo* player);
  Randoming* randoming() { return _randoming.get(); }

 private:
  void subscribe_lfw();
  void unsubscribe_lfw();
  void do_auto_join();
  bool group_has(const Value& data, const char16_t* group);

  std::unique_ptr<Randoming> _randoming;
  bool _randoming_ready = false;
  size_t _randoming_fighters = static_cast<size_t>(-1);
  std::u16string _player_names_key;
  std::vector<std::function<void()>> _removers;
  FSM _fsm;
  std::vector<std::unique_ptr<CharMenuState_Base>> _states;
};

}
}
