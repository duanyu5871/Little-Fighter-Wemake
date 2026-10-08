#include "lfw/ui/component/fn_keys_counts.h"

#include "lfw/core/js_string.h"
#include "lfw/ui/instance_ref.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

const ClazzTag* FnKeysCounts::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& FnKeysCounts::TAGS() {
  static const std::vector<std::u16string> tags{u"FnKeysCounts"};
  return tags;
}

const std::u16string& FnKeysCounts::props_tag() const {
  static const std::u16string tag = u"FnKeysCounts";
  return tag;
}

const Value& FnKeysCounts::props_meta() const {
  static const Value meta = [] {
    auto prop = [] {
      auto p = std::make_shared<Object>();
      p->set(u"type", Value(u"$cls:UINode"));
      return p;
    };
    auto o = std::make_shared<Object>();
    o->set(u"f6", Value(prop()));
    o->set(u"f7", Value(prop()));
    o->set(u"f8", Value(prop()));
    o->set(u"f9", Value(prop()));
    o->set(u"f10", Value(prop()));
    o->set(u"fn_key_counts", Value(prop()));
    o->set(u"fn_key_locked", Value(prop()));
    return Value(o);
  }();
  return meta;
}

UINode* FnKeysCounts::node_prop(const std::u16string& name) {
  if (!ensure_props()) return nullptr;
  return ref_to_node(props_holder.prop_instance(name));
}

void FnKeysCounts::on_start() {
  if (!_remove_locked) {
    _remove_locked = world().callbacks.on(
        u"on_fn_locked_change", [this](const WorldCallbacks::Payloads& p) {
          on_fn_locked_change(!p.empty() && p[0].num != 0);
        });
  }
  if (!_remove_counts) {
    _remove_counts = world().callbacks.on(u"on_counts",
                                          [this](const WorldCallbacks::Payloads&) { on_counts(); });
  }
}

void FnKeysCounts::on_stop() {
  if (_remove_locked) {
    _remove_locked();
    _remove_locked = nullptr;
  }
  if (_remove_counts) {
    _remove_counts();
    _remove_counts = nullptr;
  }
}

void FnKeysCounts::on_fn_locked_change(bool locked) {
  UINode* const lck = node_prop(u"fn_key_locked");
  UINode* const cnt = node_prop(u"fn_key_counts");
  if (locked) {
    if (lck != nullptr) lck->set_visible(true);
    if (cnt != nullptr) cnt->set_visible(false);
  } else {
    if (lck != nullptr) lck->set_visible(false);
    if (cnt != nullptr) cnt->set_visible(true);
  }
}

void FnKeysCounts::on_counts() {
  UINode* const cnt = node_prop(u"fn_key_counts");
  if (cnt != nullptr) cnt->set_visible(true);
  const std::vector<std::pair<std::u16string, double>>& all = world().counts();
  const char16_t* const names[] = {u"f6", u"f7", u"f8", u"f9", u"f10"};
  for (const char16_t* const name : names) {
    UINode* const n = node_prop(name);
    if (n == nullptr) continue;
    double count = 0;
    for (const std::pair<std::u16string, double>& e : all) {
      if (e.first == name) {
        count = e.second;
        break;
      }
    }
    n->set_text(number_to_string(count));
  }
}

}
}
