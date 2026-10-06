#include "lfw/bg/layer.h"

#include <cmath>
#include <utility>

#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace {

// `v === void 0`：只认 `undefined`，`null` 不算。
bool is_undefined(const Value& v) { return std::holds_alternative<std::monostate>(v); }

}

Layer::Layer(Background* bg, Value info, double data_index, double loop_index)
    : _bg(bg),
      _info(std::move(info)),
      _data_index(data_index),
      _loop_index(loop_index) {}

bool Layer::is_static() const {
  // `(cc === void 0 || c1 === void 0 || c2 === void 0)` 是**严格**的 `undefined` 判定。
  const bool no_cycle = is_undefined(field_or(_info, u"cc")) ||
                        is_undefined(field_or(_info, u"c1")) ||
                        is_undefined(field_or(_info, u"c2"));
  return no_cycle && !truthy(field_or(_info, u"offsetAnimX")) &&
         !truthy(field_or(_info, u"offsetAnimY")) && truthy(field_or(_info, u"absolute"));
}

void Layer::update(double count) {
  const Value cc = field_or(_info, u"cc");
  const Value c1 = field_or(_info, u"c1");
  const Value c2 = field_or(_info, u"c2");
  if (!is_undefined(cc) && !is_undefined(c1) && !is_undefined(c2)) {
    // `count % cc`：`cc` 是 0 时得 NaN ⇒ 两个比较都假 ⇒ `visible` 变 false。
    const double now = std::fmod(count, to_number(cc));
    _visible = now >= to_number(c1) && now <= to_number(c2);
  } else {
    _visible = true;
  }
}

}
