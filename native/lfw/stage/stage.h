#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/base/fsm.h"
#include "lfw/base/no_emit_callbacks.h"
#include "lfw/bg/background.h"
#include "lfw/core/value.h"
#include "lfw/stage/expressions.h"
#include "lfw/stage/item.h"

namespace lfw {

class MersenneTwister;
class World;

class LFW;

namespace stage {

class Stage;

// `Stage` 用到的实体面（`world.entities` 与 `world.puppets` 的元素）。`Item` 那刀已经为
// 「物件刷出来的实体」开了 `IItemEntity`，这里是同一个实体的另一组面（`Stage` 会直接改
// `hp` / `hp_r` / `mp` / `facing` / 位置），所以另开一条缝。
class IStageEntity {
 public:
  virtual ~IStageEntity() = default;
  virtual Value ref() const = 0;           // `is_fighter(e)` / `is_weapon(e)` 的入参
  virtual const Value& data() const = 0;   // `c.data.base.ce`
  virtual Value team() const = 0;          // `e.team === this.team`
  virtual Value ctrl() const = 0;          // `is_bot_ctrl(e.ctrl)` / `e.ctrl.player_id`
  virtual Value facing() const = 0;
  virtual void set_facing(const Value& v) = 0;
  virtual double hp() const = 0;
  virtual void set_hp(double v) = 0;
  virtual double hp_max() const = 0;
  virtual double hp_r() const = 0;
  virtual void set_hp_r(double v) = 0;
  virtual double mp() const = 0;
  virtual void set_mp(double v) = 0;
  virtual double mp_max() const = 0;
  virtual bool mounted() const = 0;        // `!e.mounted`
  virtual double position_x() const = 0;   // `e.position.x >= this.cam_r`
  virtual void set_position(const Value& x, const Value& y, const Value& z) = 0;
  virtual IStageEntity* bearer() const = 0;  // 武器持有者（`e.bearer`）
};

// `World` 的缝（`World` 那一刀再实现它）。`world.bg` 是**读写**的（`change_bg` 会换掉它）。
class IStageWorld {
 public:
  virtual ~IStageWorld() = default;
  virtual World* world_ptr() const = 0;                     // `new Background(this.world, data)`
  virtual Background* bg() const = 0;                       // `world.bg`
  virtual void set_bg(std::unique_ptr<Background> bg) = 0;  // `world.bg = bg`
  // `world.stage`（`change_bg` / `enter_phase` / `spawn` 三处要读）⇒ 一次取值（打一次日志），
  // 与 TS 的属性读一一对应。
  virtual Stage* stage() const = 0;
  virtual std::vector<IStageEntity*>& entities() = 0;
  virtual std::vector<IStageEntity*>& puppets() = 0;
  virtual void del_entities(const std::vector<IStageEntity*>& es) = 0;
  virtual Value difficulty() const = 0;                     // `world.dataset.difficulty`
  virtual void camera_jump_x(const Value& x) = 0;
};

// `Callbacks<IStageCallbacks>` 的端口替身。TS 的回调第一个/最后一个参数是 `Stage` 本身，
// 而 `Value` 装不下 C++ 对象 ⇒ 给一个专用 payload（同 `FSM` 用 `CallbacksT<FSM*>` 的先例）：
// `stage` 一律是「触发这次回调的 Stage」，`a` / `b` 放那两个值参数
// （`on_phase_changed` 的 `curr`/`prev`、`on_dialogs_changed` 的 `curr`/`prev`）。
struct StageCallbackArgs {
  Stage* stage = nullptr;
  Value a;
  Value b;
};
using StageCallbacks = CallbacksT<StageCallbackArgs>;

// TS `stage/Stage.ts`：一节的舞台（边界 / 阶段 / 物件 / 对话 / 收场）。
class Stage : public IItemHost {
 public:
  static constexpr const char* TAG = "Stage";
  using Warn = std::function<void(const std::u16string&, const std::u16string&)>;

  static void set_warn(Warn warn);
  static void warn(const std::u16string& where, const std::u16string& text);

  Stage(IStageWorld* world, LFW* lfw, Value data);
  ~Stage();

  IStageWorld* world() const { return _world; }
  LFW* lfw() const { return _lfw; }
  const Value& data() const { return _data; }
  const Value& next_stage() const { return _next_stage; }
  StageCallbacks& callbacks() { return _callbacks; }
  FSM& fsm() { return _fsm; }

  // ---- `IItemHost`（`stage/Item` 的宿主面）----
  double far_plane() const override { return _far; }
  double near_plane() const override { return _near; }
  Value team() const override { return Value(_team); }
  bool all_boss_dead() override;
  Value difficulty() const override { return _world->difficulty(); }
  MersenneTwister* mt() override;
  Value datas_find(const Value& oid) override;
  std::shared_ptr<Randoming> datas_randoming_by_group(const Value& oid) override;
  IItemEntity* create_entity_with_bot(const Value& data) override;

  Value title() const;
  bool is_stage_finish() const { return _is_stage_finish; }
  bool is_chapter_finish() const { return _is_chapter_finish; }
  Background* bg() const { return _world->bg(); }
  Value phases() const;
  std::u16string id() const;
  std::u16string name() const;
  double time() const { return _fsm.time(); }
  void set_time(double v) { _fsm.set_time(v); }
  double phase_idx() const { return _phase_idx; }
  const Value& phase() const { return _phase; }
  double dialog_idx() const { return _dialogs.index; }
  Value dialog() const;
  bool world_pause() const;
  bool control_disabled() const;
  bool weapon_rain_disabled() const;

  Background* change_bg(const Value& data);
  void stop_bgm();
  void push_dialogs(const Value& more);
  void next_dialog();
  void clear_dialogs();
  void enter_phase(double idx);
  double ce();
  void spawn(const Value& phase, const Value& obj, double count);
  void kill_all();
  void kill_soliders();
  void kill_boss();
  void kill_others();
  void dispose();
  bool all_fighter_dead();
  bool dialog_cleared() const;
  bool is_phase_end();
  bool is_dialog_end();
  bool check_phase_end();
  bool check_dialog_end();
  bool should_goto_next_stage();
  void update();

  // TS 里这些也是公开字段。
  std::vector<std::unique_ptr<Item>> items;
  Expressions<Stage> dialog_end_tester;
  Expressions<Stage> phase_end_tester;
  double phase_time = 0.0;
  double dialog_time = 0.0;
  double left = 0.0;
  double right = 0.0;
  double width = 0.0;
  double depth = 0.0;
  Background::Middle middle;
  double player_l = 0.0;
  double player_r = 0.0;
  double cam_l = 0.0;
  double cam_r = 0.0;
  double enemy_l = 0.0;
  double enemy_r = 0.0;
  double drink_l = 0.0;
  double drink_r = 0.0;

 private:
  // TS 的 `IDialogState` 是 `{ index, list }` 对象字面量；端口给个小结构（同 `Middle` 的先例）。
  struct DialogState {
    double index = -1.0;
    std::vector<Value> list;
  };

  void set_phase(const Value& phase);
  void play_phase_sounds();
  void apply_phase_bounds();
  Value dialog_state_value(const DialogState& s) const;

  IStageWorld* _world = nullptr;
  LFW* _lfw = nullptr;
  Value _data;
  Value _next_stage;
  std::u16string _team;
  StageCallbacks _callbacks;
  std::vector<std::function<void()>> _disposers;
  double _phase_idx = 0.0;
  Value _phase;
  bool _is_stage_finish = false;
  bool _is_chapter_finish = false;
  DialogState _dialogs;
  std::function<void()> _stop_bgm;
  FSM _fsm;
  // `FSM` 不持有 state ⇒ 由 `Stage` 记账。
  std::vector<std::unique_ptr<IState>> _states;
  std::vector<Item*> _released_items;
  // `near` / `far` 是 `<windows.h>` 的宏名 ⇒ 字段加下划线（`Background` 的 `near_plane()` 同理）。
  double _near = 0.0;
  double _far = 0.0;
};

}
}
