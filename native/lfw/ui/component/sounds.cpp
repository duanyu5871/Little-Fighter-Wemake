#include "lfw/ui/component/sounds.h"

#include <array>

#include "lfw/defines/defines.h"
#include "lfw/lfw.h"
#include "lfw/ui/uinode.h"

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

// TS `s in Defines.BuiltIn_Sounds` 的前半：枚举**键名**（不是小写值）。
const std::vector<EnumTextEntry>& built_in_names() { return built_in_sounds_entries(); }

// TS `in` 会走原型链 ⇒ `Object.prototype` 上的名字也命中（`toString`/`valueOf`… 都会走
// `play_with_load`，不是 `play_preset`）。
bool prototype_chain_has(const std::u16string& s) {
  static const std::array<const char16_t*, 12> kNames = {
    u"constructor",     u"__defineGetter__",    u"__defineSetter__", u"hasOwnProperty",
    u"__lookupGetter__", u"__lookupSetter__",   u"isPrototypeOf",    u"propertyIsEnumerable",
    u"toLocaleString",  u"toString",            u"valueOf",          u"__proto__",
  };
  for (const char16_t* n : kNames) {
    if (s == n) return true;
  }
  return false;
}

bool is_built_in(const std::u16string& s) {
  for (const EnumTextEntry& e : built_in_names()) {
    if (s == e.name) return true;
  }
  return prototype_chain_has(s);
}

}

const ClazzTag* Sounds::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& Sounds::TAGS() {
  static const std::vector<std::u16string> tags{u"Sounds"};
  return tags;
}

Sounds& Sounds::start() {
  idx = 0.0;
  time = 0.0;
  loop.reset();
  set_enabled(true);
  return *this;
}

Sounds& Sounds::stop() {
  idx = static_cast<double>(seq.size());
  time = 0.0;
  loop.reset();
  set_enabled(false);
  return *this;
}

void Sounds::replay() {
  time = 0.0;
  idx = 0.0;
  set_enabled(true);
  loop.reset();
}

void Sounds::on_start() {
  const Array* const args = as_array(field(info, u"args"));
  const size_t l = args != nullptr ? args->size() : 0;
  double t = 0.0;
  for (size_t i = 0; i < l; i += 2) {
    t += num(i).value_or(0.0);
    const std::u16string s = str(i + 1).value_or(std::u16string());
    seq.push_back(Step{t, s, is_built_in(s)});
  }
}

void Sounds::on_resume() {
  time = 0.0;
  idx = 0.0;
}

void Sounds::update(double dt) {
  UIComponent::update(dt);
  const double l = static_cast<double>(seq.size());
  if (l == 0.0) return;
  if (idx >= l) return;
  if (loop.done()) return;

  time += dt;
  for (; idx < l; idx += 1.0) {
    const Step& step = seq[static_cast<size_t>(idx)];
    if (step.t > time) break;
    if (step.builtin) {
      lfw().host().sounds_play_with_load(Value(step.s));
    } else {
      lfw().host().sounds_play_preset(Value(step.s), Value(), Value(), Value());
    }
  }

  if (idx >= l) {
    if (loop.continue_()) {
      time = 0.0;
      idx = 0.0;
    } else {
      set_enabled(false);
    }
  }
}

}
}
