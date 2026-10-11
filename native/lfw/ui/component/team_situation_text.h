#pragma once

#include <optional>
#include <string>
#include <vector>

#include "lfw/core/value.h"
#include "lfw/ui/component/sum_info.h"
#include "lfw/ui/component/ui_component.h"
#include "lfw/ui/style.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

class SummaryLogic;

// TS `ui/component/TeamSituationText.ts`（TAGS = ["TeamSituationText"]）：
// `args[0]` 指定队伍则固定；否则订阅世界 `on_stage_change` 跟随当前关卡的队伍。
// 文本形如 `Man: 2　HP: 900`（有候补再追 `　Reserve: N`）。
class TeamSituationText : public UIComponent {
 public:
  static constexpr const char* TAG = "TeamSituationText";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  explicit TeamSituationText(UINode& layout, const std::u16string& f_name, const Value& info);

  const ClazzTag* clazz() const override { return class_tag(); }

  void on_start() override;
  void on_stop() override;
  void on_resume() override;
  void update(double dt) override;

  void set_team(const std::u16string& team);
  void rr();

 protected:
  std::optional<std::u16string> _team;
  Style _style;
  SummaryLogic* _sum_logic = nullptr;
  std::u16string _sum_team;
  std::u16string _text = u" ";

 private:
  using WorldListener = NoEmitCallbacksT<WorldCallbackArgs>::Listener;
  using WorldPayloads = NoEmitCallbacksT<WorldCallbackArgs>::Payloads;

  struct StageListener : WorldListener {
    explicit StageListener(TeamSituationText* o);
  };

  StageListener _world_cb{this};
};

}
}
