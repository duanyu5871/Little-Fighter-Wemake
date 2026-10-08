#include "lfw/ui/component/player_key_text.h"

#include <memory>
#include <string>
#include <utility>

#include "lfw/core/js_string.h"
#include "lfw/defines/defines_data.h"
#include "lfw/lfw.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"

namespace lfw {
namespace ui {

namespace {

const Value* values_field(const Value& data, const char16_t* field_name) {
  const Value* const values = field_of(data, u"values");
  if (values == nullptr) return nullptr;
  return field_of(*values, field_name);
}

}

const ClazzTag* PlayerKeyText::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PlayerKeyText::TAGS() {
  static const std::vector<std::u16string> tags{u"PlayerKeyText"};
  return tags;
}

std::u16string PlayerKeyText::pick(size_t idx, const char16_t* field_name) {
  const Value* const args_v = field_of(info, u"args");
  const Array* const args = args_v != nullptr ? as_array(*args_v) : nullptr;
  if (args != nullptr && idx < args->size()) {
    const std::u16string* const s = std::get_if<std::u16string>(&args->at(idx));
    if (s != nullptr && !s->empty()) return *s;
  }
  UINode* const parent = node.find_parent([field_name](UINode& v) {
    const Value* const f = values_field(v.data(), field_name);
    return f != nullptr && truthy(*f);
  });
  if (parent != nullptr) {
    const Value* const f = values_field(parent->data(), field_name);
    if (f != nullptr) return to_string(*f);
  }
  return std::u16string();
}

std::u16string PlayerKeyText::player_id() { return pick(0, u"player_id"); }

std::u16string PlayerKeyText::key_name() { return pick(1, u"key_name"); }

PlayerInfo* PlayerKeyText::player() {
  const std::u16string pid = player_id();
  for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
    if (kv.first == pid) return kv.second.get();
  }
  return nullptr;
}

std::u16string PlayerKeyText::key_code() {
  PlayerInfo* const p = player();
  if (p == nullptr) return u"None";
  const Object* const keys = as_object(p->keys());
  const Value* const raw = keys != nullptr ? keys->get(key_name()) : nullptr;
  if (raw == nullptr || !std::holds_alternative<std::u16string>(*raw)) return u"None";
  const std::u16string kc = to_upper_case(std::get<std::u16string>(*raw));
  std::u16string out;
  const Value* const table = defines::find(u"Defines.SHORT_KEY_CODES");
  const Object* const map = table != nullptr ? as_object(*table) : nullptr;
  const Value* const mapped = map != nullptr ? map->get(kc) : nullptr;
  if (mapped != nullptr && std::holds_alternative<std::u16string>(*mapped)) {
    out = std::get<std::u16string>(*mapped);
  }
  if (out.empty()) out = kc;
  if (out.empty()) out = u"None";
  return out;
}

void PlayerKeyText::on_resume() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
  PlayerInfo* const p = player();
  if (p != nullptr) {
    _remover = p->callbacks.on(u"on_key_changed", [this](const Callbacks::Payloads&) {
      on_key_changed();
    });
  }
  on_key_changed();
}

void PlayerKeyText::on_pause() {
  if (_remover) {
    _remover();
    _remover = nullptr;
  }
}

void PlayerKeyText::on_key_changed() { node.set_text(key_code()); }

}
}
