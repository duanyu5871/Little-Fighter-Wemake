#include "lfw/ui/component/components_player.h"

#include <memory>

#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"

namespace lfw {
namespace ui {

namespace {

// `string[] | undefined`：非数组/非字符串项一律跳过（TS 靠 schema 校验，端口宽松）。
std::vector<std::u16string> strs_of(const Value* const v) {
  std::vector<std::u16string> out;
  const Array* const a = v != nullptr ? as_array(*v) : nullptr;
  if (a == nullptr) return out;
  for (size_t i = 0; i < a->size(); ++i) {
    if (const std::u16string* const s = std::get_if<std::u16string>(&a->at(i))) out.push_back(*s);
  }
  return out;
}

std::shared_ptr<Object> str_arr_prop() {
  auto p = std::make_shared<Object>();
  p->set(u"type", Value(u"array"));
  auto items = std::make_shared<Object>();
  items->set(u"type", Value(u"string"));
  p->set(u"items", Value(items));
  p->set(u"nullable", Value(true));
  return p;
}

std::shared_ptr<Object> bool_prop(const char16_t* key) {
  auto p = std::make_shared<Object>();
  p->set(u"key", Value(std::u16string(key)));
  p->set(u"type", Value(u"boolean"));
  p->set(u"nullable", Value(true));
  return p;
}

bool truthy_value(const Value* const v) { return v != nullptr && truthy(*v); }

}

const ClazzTag* ComponentsPlayer::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& ComponentsPlayer::TAGS() {
  static const std::vector<std::u16string> tags{u"ComponentsPlayer"};
  return tags;
}

const std::u16string& ComponentsPlayer::props_tag() const {
  static const std::u16string tag = u"ComponentsPlayer";
  return tag;
}

const Value& ComponentsPlayer::PROPS() {
  static const Value props = [] {
    auto o = std::make_shared<Object>();
    auto comps = str_arr_prop();
    comps->set(u"key", Value(u"children"));
    o->set(u"components", Value(comps));
    o->set(u"local", Value(bool_prop(u"local")));
    o->set(u"recursive", Value(bool_prop(u"local")));
    return Value(o);
  }();
  return props;
}

const Value& ComponentsPlayer::props_meta() const { return PROPS(); }

void ComponentsPlayer::on_start() {
  const Value* const p = props();
  if (p == nullptr) return;
  const std::vector<std::u16string> ids = strs_of(field_of(*p, u"components"));
  const bool local = truthy_value(field_of(*p, u"local"));
  const bool recursive = truthy_value(field_of(*p, u"recursive"));
  UINode& search_node = local ? node : node.root();
  if (ids.empty()) return;
  components.assign(ids.size(), nullptr);
  search_node.traversal_components([&](UIComponent& c, int depth) {
    if (!recursive && depth != 0) return false;
    if (!c.is_playable()) return false;
    for (size_t i = 0; i < ids.size(); ++i) {
      if (c.id != ids[i]) continue;
      if (components[i] != nullptr) continue;
      if (&c == this) continue;
      components[i] = &c;
    }
    return false;
  });
}

void ComponentsPlayer::start() {
  for (UIComponent* const c : components) {
    if (c != nullptr) c->start();
  }
  set_enabled(true);
}

void ComponentsPlayer::stop() {
  for (UIComponent* const c : components) {
    if (c != nullptr) c->stop();
  }
  set_enabled(false);
}

void ComponentsPlayer::replay() {
  for (UIComponent* const c : components) {
    if (c != nullptr) c->replay();
  }
  set_enabled(true);
}

}
}
