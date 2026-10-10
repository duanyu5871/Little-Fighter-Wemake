#include "lfw/ui/component/player_ctrl_type.h"

#include <cmath>

#include "lfw/defines/ctrl_device.h"
#include "lfw/lfw.h"
#include "lfw/player_info.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"

namespace lfw {
namespace ui {

namespace {

const Value& field(const Value& obj, const char16_t* key) {
  static const Value kUndef;
  const Object* const o = as_object(obj);
  if (o == nullptr) return kUndef;
  const Value* const p = o->get(std::u16string(key));
  return p != nullptr ? *p : kUndef;
}

const Value* player_id_of(UINode& v) {
  const Value* const values = field_of(v.data(), u"values");
  const Value* const pid = values != nullptr ? field_of(*values, u"player_id") : nullptr;
  return pid != nullptr && truthy(*pid) ? pid : nullptr;
}

}

const ClazzTag* PlayerCtrlType::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PlayerCtrlType::TAGS() {
  static const std::vector<std::u16string> tags{u"PlayerCtrlType"};
  return tags;
}

std::u16string PlayerCtrlType::player_id() {
  const Array* const args = as_array(field(info, u"args"));
  if (args != nullptr && args->size() > 0 && truthy(args->at(0))) return to_string(args->at(0));
  UINode* const p = node.find_parent([](UINode& v) { return player_id_of(v) != nullptr; });
  if (p != nullptr) {
    const Value* const pid = player_id_of(*p);
    return pid != nullptr ? to_string(*pid) : std::u16string();
  }
  return std::u16string();
}

PlayerInfo* PlayerCtrlType::player() {
  const std::u16string id = player_id();
  for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
    if (kv.first == id) return kv.second.get();
  }
  return nullptr;
}

void PlayerCtrlType::on_resume() {
  PlayerInfo* const p = player();
  if (p == nullptr) return;
  if (!_remover) {
    _remover = p->callbacks.on(u"on_ctrl_changed", [this](const Callbacks::Payloads& pl) {
      if (!pl.empty()) on_ctrl_changed(pl[0]);
    });
  }
  on_ctrl_changed(p->ctrl());
}

void PlayerCtrlType::on_pause() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
}

void PlayerCtrlType::on_click(LFWPointerEvent& e) {
  PlayerInfo* const p = player();
  if (p == nullptr) return;
  node.set_focused(true);
  e.stop_immediate_propagation();
  const double cur = to_number(p->ctrl());
  const double ctrl = std::fmod(e.button == 2.0 ? cur + 5.0 : cur + 1.0, 6.0);
  if (ctrl == static_cast<double>(CtrlDevice::TouchScreen)) {
    for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
      PlayerInfo* const other = kv.second.get();
      if (other == p) continue;
      if (to_number(other->ctrl()) != static_cast<double>(CtrlDevice::TouchScreen)) continue;
      other->set_ctrl(Value(static_cast<double>(CtrlDevice::Keyboard)), true);
      other->save();
    }
  }
  p->set_ctrl(Value(ctrl), true);
  p->save();
}

void PlayerCtrlType::on_ctrl_changed(const Value& c) {
  const double ctrl = to_number(c);
  const std::vector<UINode*>& kids = node.children();
  for (size_t i = 0; i < kids.size(); ++i) {
    kids[i]->set_visible(static_cast<double>(i) == ctrl);
  }
}

}
}
