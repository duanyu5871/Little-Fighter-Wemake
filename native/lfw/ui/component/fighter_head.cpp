#include "lfw/ui/component/fighter_head.h"

#include "lfw/defines/defines.h"
#include "lfw/entity/entity.h"
#include "lfw/lfw.h"
#include "lfw/player_info.h"
#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"

namespace lfw {
namespace ui {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

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

const ClazzTag* FighterHead::class_tag() {
  static const ClazzTag tag{Picture::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& FighterHead::TAGS() {
  static const std::vector<std::u16string> tags{u"FighterHead"};
  return tags;
}

std::u16string FighterHead::player_id() {
  const Array* const args = as_array(field(info, u"args"));
  if (args != nullptr && args->size() > 0 && truthy(args->at(0))) return to_string(args->at(0));
  UINode* const p = node.find_parent([](UINode& v) { return player_id_of(v) != nullptr; });
  if (p != nullptr) {
    const Value* const pid = player_id_of(*p);
    return pid != nullptr ? to_string(*pid) : std::u16string();
  }
  return std::u16string();
}

PlayerInfo* FighterHead::player() {
  const std::u16string id = player_id();
  for (const std::pair<std::u16string, std::unique_ptr<PlayerInfo>>& kv : lfw().players()) {
    if (kv.first == id) return kv.second.get();
  }
  return nullptr;
}

std::u16string FighterHead::head() {
  PlayerInfo* const p = player();
  const Entity* const fighter = p != nullptr ? p->fighter() : nullptr;
  if (fighter == nullptr) return std::u16string(built_in_imgs::kRFACE);
  const Value* const base = field_of(fighter->data(), u"base");
  const Value* const head_v = base != nullptr ? field_of(*base, u"head") : nullptr;
  if (head_v == nullptr || is_nullish(*head_v)) return std::u16string(built_in_imgs::kRFACE);
  return to_string(*head_v);
}

void FighterHead::on_start() {
  UIComponent::on_start();
  set_width(120);
  set_height(120);
}

void FighterHead::handle_changed() { set_src(head()); }

}
}
