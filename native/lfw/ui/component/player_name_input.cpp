#include "lfw/ui/component/player_name_input.h"

#include <memory>
#include <string>

#include "lfw/lfw.h"
#include "lfw/ui/uinode.h"

namespace lfw {
namespace ui {

const ClazzTag* PlayerNameInput::class_tag() {
  static const ClazzTag tag{TextInput::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PlayerNameInput::TAGS() {
  static const std::vector<std::u16string> tags{u"PlayerNameInput"};
  return tags;
}

std::optional<double> PlayerNameInput::max_length() {
  return props_holder.num(u"maxLength").value_or(10.0);
}

std::optional<std::u16string> PlayerNameInput::default_value() {
  return props_holder.str(u"defaultValue");
}

std::u16string PlayerNameInput::player_id() {
  const Value v = node.get_value(u"player_id", true);
  const std::u16string* const s = std::get_if<std::u16string>(&v);
  return s != nullptr ? *s : std::u16string();
}

void PlayerNameInput::on_start() {
  const std::u16string pid = player_id();
  for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
    if (kv.first != pid) continue;
    _player = kv.second.get();
    break;
  }
  if (_player != nullptr && !_remover) {
    _remover = _player->callbacks.on(
        u"on_name_changed", [this](const Callbacks::Payloads& p) { on_name_changed(p); });
  }
}

void PlayerNameInput::on_stop() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
  if (_player != nullptr) {
    _player->set_name(Value(text()), true);
    _player->save();
  }
}

void PlayerNameInput::on_resume() {
  set_text(_player != nullptr ? to_string(_player->name()) : std::u16string());
}

void PlayerNameInput::on_blur() {
  if (_player != nullptr) {
    _player->set_name(Value(text()), true);
    _player->save();
  }
}

void PlayerNameInput::on_name_changed(const Callbacks::Payloads& p) {
  if (!p.empty()) _text = to_string(p[0]);
}

}
}
