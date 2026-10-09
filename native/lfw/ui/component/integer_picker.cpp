#include "lfw/ui/component/integer_picker.h"

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "lfw/ui/uinode.h"
#include "lfw/ui/value_spread.h"
#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {
namespace ui {

namespace {

constexpr double kMin = -9007199254740991.0;
constexpr double kMax = 9007199254740991.0;

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

}

const ClazzTag* IntegerPicker::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& IntegerPicker::TAGS() {
  static const std::vector<std::u16string> tags{u"IntegerPicker"};
  return tags;
}

void IntegerPicker::init() {
  const double v1 = props_holder.num(u"min").value_or(kMin);
  const double v2 = props_holder.num(u"max").value_or(kMax);
  const std::optional<double> v3 = props_holder.num(u"val");
  set_min(lfw::min(v1, v2));
  set_max(lfw::max(v1, v2));
  set_val(v3.has_value() ? *v3 : _min);
  const std::optional<std::vector<std::u16string>> triggers = props_holder.strs(u"triggers");
  if (triggers.has_value()) {
    for (const std::u16string& t : *triggers) _triggers.insert(t);
  } else {
    _triggers.insert(u"click");
  }
}

void IntegerPicker::on_click(LFWPointerEvent& e) {
  if (_triggers.count(u"click") == 0) return;
  switch (static_cast<int>(e.button)) {
    case 0: goto_next(); break;
    case 2: goto_prev(); break;
  }
}

void IntegerPicker::on_key_down(LFWKeyEvent& e) {
  const bool no_focused = _triggers.count(u"no-focused") != 0;
  if (!node.focused() && !no_focused) return;
  const bool need_aj = _triggers.count(u"aj") != 0;
  const bool need_ud = _triggers.count(u"ud") != 0;
  const bool need_lr = _triggers.count(u"lr") != 0;
  const std::u16string& gk = e.game_key;
  if (gk == u"j") {
    if (need_aj) goto_prev();
  } else if (gk == u"a") {
    if (need_aj) goto_next();
  } else if (gk == u"U") {
    if (need_ud) goto_prev();
  } else if (gk == u"D") {
    if (need_ud) goto_next();
  } else if (gk == u"L") {
    if (need_lr) goto_prev();
  } else if (gk == u"R") {
    if (need_lr) goto_next();
  }
}

void IntegerPicker::goto_next() {
  const double v = _val >= _max ? _min : _val + 1;
  set_val(v);
}

void IntegerPicker::goto_prev() {
  const double v = _val <= _min ? _max : _val - 1;
  set_val(v);
}

void IntegerPicker::set_min(double v) {
  if (v > _max) {
    set_max(v);
    return;
  }
  if (std::isnan(v)) {
    v = kMin;
  } else {
    v = lfw::max(ceil(v), kMin);
  }
  const double p = _min;
  if (p == v) return;
  _min = v;
  IntegerPickerCallbackArgs a;
  a.curr = _min;
  a.prev = p;
  a.self = this;
  callbacks.call(u"on_min_changed", {a});
  set_val(_val);
}

void IntegerPicker::set_max(double v) {
  if (v < _min) {
    set_min(v);
    return;
  }
  if (std::isnan(v)) {
    v = kMin;
  } else {
    v = lfw::min(floor(v), kMax);
  }
  const double p = _max;
  if (p == v) return;
  _max = v;
  IntegerPickerCallbackArgs a;
  a.curr = _max;
  a.prev = p;
  a.self = this;
  callbacks.call(u"on_max_changed", {a});
  set_val(_val);
}

void IntegerPicker::set_val(double v) {
  if (std::isnan(v)) {
    v = _min;
  } else {
    v = clamp(round(v), _min, _max);
  }
  const double p = _val;
  if (p == v) return;
  _val = v;
  IntegerPickerCallbackArgs a;
  a.curr = _val;
  a.prev = p;
  a.self = this;
  callbacks.call(u"on_val_changed", {a});
  const Value* const style = field_of(node.text(), u"style");
  node.set_text(number_to_string(v),
                style != nullptr && !is_nullish(*style) ? *style : Value(std::make_shared<Object>()));
}

}
}
