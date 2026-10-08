#pragma once

#include <functional>
#include <string>

#include "lfw/base/no_emit_callbacks.h"
#include "lfw/player_info.h"
#include "lfw/ui/component/text_input.h"

namespace lfw {
namespace ui {

// TS `ui/component/PlayerNameInput.ts`（TAGS = ["PlayerNameInput"]）：文本输入框绑定玩家名。
// `player_id` 从节点 `values.player_id`（`get_value(..., true)` 会向上找）取。
class PlayerNameInput : public TextInput {
 public:
  static constexpr const char* TAG = "PlayerNameInput";
  static const ClazzTag* class_tag();
  static const std::vector<std::u16string>& TAGS();

  using TextInput::TextInput;

  const ClazzTag* clazz() const override { return class_tag(); }

  std::optional<double> max_length() override;
  std::optional<std::u16string> default_value() override;

  void on_start() override;
  void on_stop() override;
  void on_resume() override;
  void on_blur() override;
  void on_name_changed(const Callbacks::Payloads& p);

  std::u16string player_id();
  PlayerInfo* player() const { return _player; }

 protected:
  PlayerInfo* _player = nullptr;
  std::function<void()> _remover;
};

}
}
