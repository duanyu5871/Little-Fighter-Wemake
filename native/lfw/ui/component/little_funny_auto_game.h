#pragma once

#include <string>
#include <vector>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/core/value.h"
#include "lfw/ui/component/action_director.h"
#include "lfw/ui/component/ui_component.h"

namespace lfw {

class Entity;

namespace ui {

// TS `ui/component/LittleFunnyAutoGame.ts`：自动对战演示（`TAGS = ["LittleFunnyAutoGame"]`）。
// 5s 后随机抽两名「BOSS」（1/2 队各一），按双方 `base.ce` 维持士兵数量差；
// 实体死亡回调里轮换左右与补位（死一个补一个、超出回收为 3 队）。
class LittleFunnyAutoGame : public UIComponent {
 public:
  static constexpr const char* TAG = "LittleFunnyAutoGame";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  LittleFunnyAutoGame(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }

  void update(double dt) override;
  void on_resume() override;
  void on_pause() override;

  Entity* add_random_fighter(int lr, const std::u16string& team,
                             const Value& data = Value());

  // 台面观察口（不参与逻辑）。
  Entity* boss(int team) const;
  const std::vector<Entity*>& soldiers(int team) const;
  int lr() const { return _lr; }
  int ox() const { return _ox; }

 private:
  using ValueListener = NoEmitCallbacksT<Value>::Listener;
  using ValuePayloads = NoEmitCallbacksT<Value>::Payloads;

  struct Teams {
    Entity* boss = nullptr;
    std::vector<Entity*> soldiers;
  };

  void on_fighter_dead(const Value& ref);
  void handle_soldiers();

  Value _datas = Value(std::make_shared<Array>());
  // TS 写着 `private _lr: number = 0 | 1` —— 初始值是**按位或**求出的 1（不是 0），
  // `_ox` 同；`on_resume` 只重置 `_lr`，`_ox` 全程只翻面。
  int _lr = 1;
  int _ox = 1;
  Teams _team1;
  Teams _team2;

  struct FighterListenerT : ValueListener {
    explicit FighterListenerT(LittleFunnyAutoGame* o);
  };
  FighterListenerT _fighter_cbs{this};

  ActionDirector _director;
};

}
}
